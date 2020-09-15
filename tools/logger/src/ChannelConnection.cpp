/**
 * @file tools/logger/src/ChannelConnection.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the class used to control a connection to the channel
 * server.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ChannelConnection.h"

#include <Crypto.h>

// Ignore warnings
#include <PushIgnore.h>

#include <QDateTime>
#include <QDir>
#include <QFile>

// Stop ignoring warnings
#include <PopIgnore.h>

// Standard C++ Includes
#include <iostream>

#include "LoggerServer.h"

#ifdef Q_OS_WIN32
#include <wincrypt.h>
#include <windows.h>
#endif  // Q_OS_WIN32

// Connection magic sent by the client to the server requesting to start the
// encryption process.
static uint8_t connectMagic[8] = {0x00, 0x00, 0x00, 0x01,
                                  0x00, 0x00, 0x00, 0x08};

static const uint32_t FORMAT_MAGIC = 0x4B434148;  // HACK
static const uint32_t FORMAT_VER = 0x00010100;    // Major, Minor, Patch (1.1.0)

#ifdef Q_OS_WIN32
#include <windows.h>

uint64_t microtime() {
  const uint64_t DELTA_EPOCH_IN_MICROSECS = 11644473600000000ULL;

  FILETIME ft;

  ZeroMemory(&ft, sizeof(ft));
  GetSystemTimeAsFileTime(&ft);

  return (((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime) /
             10 -
         DELTA_EPOCH_IN_MICROSECS;
}
#else  // Q_OS_WIN32
#include <sys/time.h>

uint64_t microtime() {
  struct timeval now;
  gettimeofday(&now, 0);

  return ((uint64_t)now.tv_sec * 1000000ULL) + (uint64_t)now.tv_usec;
}
#endif  // Q_OS_WIN32

ChannelConnection::ChannelConnection(LoggerServer* server,
                                     qintptr socketDescriptor,
                                     uint32_t clientID, QObject* p)
    : QThread(p),
      mServer(server),
      mClientState(NotConnected),
      mServerState(NotConnected),
      mClientLoginPacket(0),
      mClientLoginPacketSize(0),
      mClientSocket(0),
      mServerSocket(0),
      mSocketDescriptor(socketDescriptor),
      mClientID(clientID) {}

ChannelConnection::~ChannelConnection() {
  // Delete the client login packet if it exists.
  delete mClientLoginPacket;
  mClientLoginPacket = 0;
}

void ChannelConnection::run() {
  // Setup the encryption data that will be passed to the client
  // when the client connects to the server.
  mClientCryptData.base = "2";
  mClientCryptData.prime =
      "f488fd584e49dbcd20b49de49107366b336c380d451d0f7c88b31c7c5b2d8ef6f3c923c0"
      "43f0a55b188d8ebb558cb85d38d334fd7c175743a31d186cde33212cb52aff3ce1b12940"
      "18118d7c84a70a72d686c40319c807297aca950cd9969fabd00a509b0246d3083d66a45d"
      "419f9c7cbd894b221926baaba25ec355e92f78c7";
  mClientCryptData.secret = libcomp::Crypto::GenerateRandom();
  mClientCryptData.serverPublic =
      libcomp::Crypto::GenDiffieHellman(mClientCryptData.base,
                                        mClientCryptData.prime,
                                        mClientCryptData.secret)
          .RightJustified(256, '0');

  // Generate the packet that will be sent to the client when it sends
  // the hello packet.
  mKeyExchangePacket.Clear();
  mKeyExchangePacket.WriteBlank(4);
  mKeyExchangePacket.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                                      mClientCryptData.base);
  mKeyExchangePacket.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                                      mClientCryptData.prime);
  mKeyExchangePacket.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                                      mClientCryptData.serverPublic);

  // Create a socket for the client connection.
  mClientSocket = new QTcpSocket;

  // Connect the needed signals for the client socket.
  connect(mClientSocket, SIGNAL(readyRead()), this, SLOT(clientReady()),
          Qt::DirectConnection);
  connect(mClientSocket, SIGNAL(disconnected()), this, SLOT(clientLost()),
          Qt::DirectConnection);

  // Set the client socket descriptor and open the socket.
  mClientSocket->setSocketDescriptor(mSocketDescriptor);
  mClientSocket->open(QIODevice::ReadWrite);

  // Retrieve the address of the client (for logging).
  mClientAddress = mClientSocket->peerAddress().toString();

  // Add log message about client connection.
  logMessage(tr("Client %1 connected to the channel server from %2")
                 .arg(mClientID)
                 .arg(mClientAddress));

  // Set the state of the connection.
  mClientState = Connected;

  // Only open the log file if logging is enabled.
  if (mServer->isChannelLogEnabled()) {
    // Get the current time.
    QDateTime time = QDateTime::currentDateTime();
    time_t stamp = time.toTime_t();

    // Generate the name of the capture file.
    QString filename = tr("%1.hack").arg(time.toString("yyyyMMddhhmmss"));

    // Get the full path of the capture file to be created.
    QString path = QDir(mServer->capturePath()).absoluteFilePath(filename);

    // Calculate the length of the client address string.
    uint32_t addrlen = static_cast<uint32_t>(mClientAddress.toUtf8().size());

    // Write the header to the log file.
    mCaptureLog.setFileName(path);
    mCaptureLog.open(QIODevice::WriteOnly);
    mCaptureLog.write((char*)&FORMAT_MAGIC, 4);
    mCaptureLog.write((char*)&FORMAT_VER, 4);
    mCaptureLog.write((char*)&stamp, 8);
    mCaptureLog.write((char*)&addrlen, 4);
    mCaptureLog.write(mClientAddress.toUtf8().constData(), addrlen);
  }

  // Start the thread.
  exec();
}

QString ChannelConnection::timestamp() const {
  // Generate a timestamp.
  return QDateTime::currentDateTime().toString(Qt::ISODate);
}

void ChannelConnection::clientLost() {
  // If the client is not connected anymore, ignore.
  if (mClientState == NotConnected) return;

  // Set the client as disconnected so this function is not run again.
  mClientState = NotConnected;

  // Add log message about client disconnect.
  logMessage(
      tr("Client %1 disconnected from the channel server").arg(mClientID));

  // Close the log file.
  mCaptureLog.close();

  // If the login packet still exists, delete it.
  if (mClientLoginPacket) {
    delete mClientLoginPacket;
    mClientLoginPacket = 0;
  }

  // Disconnect the socket.
  mClientSocket->disconnectFromHost();

  // Wait for the socket to disconnect.
  if (mClientSocket->state() != QAbstractSocket::UnconnectedState)
    mClientSocket->waitForDisconnected();

  // Block signals from the socket and delete it.
  mClientSocket->blockSignals(true);
  mClientSocket->deleteLater();
  mClientSocket = 0;

  // If we ever connected to the target server, do the same for the server
  // connection.
  if (mServerSocket) {
    // Disconnect the socket.
    mServerSocket->disconnectFromHost();

    // Wait for the socket to disconnect.
    if (mServerSocket->state() != QAbstractSocket::UnconnectedState)
      mServerSocket->waitForDisconnected();

    // Block signals from the socket and delete it.
    mServerSocket->blockSignals(true);
    mServerSocket->deleteLater();
    mServerSocket = 0;
  }

  // We are done, exit and delete the thread.
  exit(0);
}

void ChannelConnection::encryptPacket(const BF_KEY* key, libcomp::Packet& p) {
  // Skip over the sizes.
  p.Seek(8);

  // Buffer to store the current block of data.
  uint32_t buffer[2];

  // Encrypt each block of data.
  for (uint32_t i = 8; i < p.Size(); i += 8) {
    // Read the unencrypted data from the packet into the buffer.
    p.ReadArray(buffer, 8);

    // Move the packet pointer back.
    p.Rewind(8);

    // Encrypt the data in the buffer.
    BF_encrypt(buffer, key);

    // Write the encrypted data back into the packet.
    p.WriteArray(buffer, 8);
  }

  // Seek back to the beginning of the packet.
  p.Rewind();
}

void ChannelConnection::decryptPacket(const BF_KEY* key, libcomp::Packet& p) {
  // Skip over the sizes.
  p.Seek(8);

  // Buffer to store the current block of data.
  uint32_t buffer[2];

  for (uint32_t i = 8; i < p.Size(); i += 8) {
    // Read the encrypted data from the packet into the buffer.
    p.ReadArray(buffer, 8);

    // Move the packet pointer back.
    p.Rewind(8);

    // Decrypt the data in the buffer.
    BF_decrypt(buffer, key);

    // Write the unencrypted data back into the packet.
    p.WriteArray(buffer, 8);
  }

  // Seek back to the beginning of the packet.
  p.Rewind();
}

void ChannelConnection::clientReady() {
  // Determine how many bytes are avaliable for reading from the socket.
  uint32_t avail = (uint32_t)mClientSocket->bytesAvailable();

  // If we don't have at least have 8 bytes for the packet sizes,
  // return from the function and wait for more data.
  if (avail < 8) return;

  // This is a special case, this packet doesn't have sizes before it.
  // If the exchange has started, process the exchange packet.
  if (mClientState == ExchangeStarted) {
    // If we don't have all of the packet yet, return from the function
    // and wait for more data.
    if (avail < 260) return;

    // Packet object to store the packet data in.
    libcomp::Packet p;

    // Read in the packet.
    mClientSocket->read(p.Direct(260), 260);

    // Read in the client's public key.
    mClientCryptData.clientPublic =
        p.ReadString32Big(libcomp::Convert::ENCODING_UTF8);

    // Set the client state to encrypted.
    mClientState = Encrypted;

    // Calculate the final shared encryption key.
    mClientCryptData.sharedKey = libcomp::Crypto::GenDiffieHellman(
        mClientCryptData.clientPublic, mClientCryptData.prime,
        mClientCryptData.secret);
    mClientCryptData.keys = QByteArray::fromHex(mClientCryptData.sharedKey.C());

    // Set the shared encryption key.
    BF_set_key(&mClientCryptData.key, 8,
               (uchar*)mClientCryptData.keys.constData());

    // Adjust how much data is avaliable.
    avail -= 260;

    // If there isn't enough data to read another packet,
    // return from the function and wait for more data.
    if (avail < 8) return;
  }

  // Packet object to store the packet data in.
  libcomp::Packet p;

  // Read in the sizes (without removing them from the socket buffer).
  mClientSocket->peek(p.Direct(8), 8);

  // Calculate the padded and real size of the packet.
  uint32_t paddedSize = p.ReadU32Big();
  uint32_t realSize = p.ReadU32Big();

  // Check for connect magic.
  if (paddedSize == 1 && realSize == 8) {
    // Remove the magic from the buffer.
    mClientSocket->read(p.Direct(8), 8);

    // This makes sure we only send it once.
    if (mClientState == Connected) {
      mClientSocket->write(mKeyExchangePacket.Data(),
                           mKeyExchangePacket.Size());
      mClientSocket->flush();

      mClientState = ExchangeStarted;
    }

    // Read again if there is another packet (not that there should be).
    if (avail >= 16) clientReady();

    return;
  }

  // If the client isn't encrypted yet, return (this should never happen).
  if (mClientState != Encrypted) return;

  // If the entire packet isn't buffered, return and wait for more data.
  if (avail < (paddedSize + 8)) return;

  // Go back and get ready to read in the packet.
  p.Rewind();

  // Read in the packet.
  mClientSocket->read(p.Direct(paddedSize + 8), paddedSize + 8);

  // Decrypt the packet.
  decryptPacket(&mClientCryptData.key, p);

  // Log the packet.
  logPacket(p, 0);

  // If the client hasn't read the first packet yet, read it now.
  if (mServerState == NotConnected) {
    // Calculate how much data is padding.
    uint32_t padding = paddedSize - realSize;

    // Skip over the data we don't care about.
    p.Seek(30);

    // Variable for the channel key.
    uint32_t channelKey;

    // If there is only one string, use the old login method.
    // If there is more data in the packet, assume the new atlus method.
    if (p.Left() == (6 + padding + p.PeekU16Little())) {
      // Read the username.
      mUsername = p.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

      // Read the channel key.
      channelKey = p.ReadU32Little();
    } else {
      // Read the authentication string.
      libcomp::String authKey =
          p.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

      // Read the channel key.
      channelKey = p.ReadU32Little();

      // Read the username.
      mUsername = p.ReadString16Little(libcomp::Convert::ENCODING_UTF8);
    }

    // Find the original address of the channel server.
    QString addr = mServer->retrieveChannelKey(channelKey).trimmed();

    // Split the address into the IP and port.
    QStringList addrPair = addr.split(':');

    // Set the IP address.
    addr = addrPair.at(0);

    // Set the port.
    QString port = addrPair.at(1);

    // Delete any old packet if it exists (should never happen).
    if (mClientLoginPacket) delete mClientLoginPacket;

    // Set the size of the packet.
    mClientLoginPacketSize = paddedSize + 8;

    // Allocate space for the login packet.
    mClientLoginPacket = new uint8_t[mClientLoginPacketSize];

    // Rewind to the beginning of the packet.
    p.Rewind();

    // Write the packet into the buffer.
    p.ReadArray(mClientLoginPacket, mClientLoginPacketSize);

    // Convert the port to an int value.
    int portNum = port.toInt();

    // Create the server socket.
    mServerSocket = new QTcpSocket;

    // Connect the needed signals to the server socket.
    connect(mServerSocket, SIGNAL(readyRead()), this, SLOT(serverReady()),
            Qt::DirectConnection);
    connect(mServerSocket, SIGNAL(connected()), this, SLOT(sendClientHello()),
            Qt::DirectConnection);
    connect(mServerSocket, SIGNAL(disconnected()), this, SLOT(serverLost()),
            Qt::DirectConnection);

    // Connect to the server.
    mServerSocket->connectToHost(addr, (quint16)portNum);

    // Set the server state.
    mServerState = Connected;

    // If there is more data to read for the client, read it now.
    if (mClientSocket->bytesAvailable() >= 8) clientReady();

    return;
  }

  // If the relay has not connected to the server yet,
  // save the packet for later.
  if (mServerState != Encrypted) {
    // Save the packet.
    mPacketBuffer.append(
        QByteArray(p.Data(), static_cast<int>(paddedSize + 8)));

    // If there is more data to read for the client, read it now.
    if (mClientSocket->bytesAvailable() >= 8) clientReady();

    return;
  }

  // Encrypt the packet.
  encryptPacket(&mServerCryptData.key, p);

  // Send the packet to the server.
  mServerSocket->write((char*)p.Data(), p.Size());
  mServerSocket->flush();

  // If there is more data to read for the client, read it now.
  if (mClientSocket->bytesAvailable() >= 8) clientReady();
}

void ChannelConnection::sendClientHello() {
  // The relay has connected to the server, send the connect magic.
  mServerSocket->write((char*)connectMagic, 8);
  mServerSocket->flush();
}

void ChannelConnection::serverLost() {
  // If the server has disconnected us, disconnect the client.
  clientLost();
}

void ChannelConnection::serverReady() {
  // Determine how many bytes are avaliable for reading from the socket.
  uint32_t avail = (uint32_t)mServerSocket->bytesAvailable();

  // If we don't have at least have 8 bytes for the packet sizes,
  // return from the function and wait for more data.
  if (avail < 8) return;

  // If the server is still exchanging keys, check for the reply.
  if (mServerState != Encrypted) {
    // If we don't have all of the packet yet, return from the function
    // and wait for more data.
    if (avail < 529) return;

    // Exchange the encryption keys.
    exchangeKeys();

    // Adjust how much data is avaliable.
    avail -= 529;

    // If there isn't enough data to read another packet,
    // return from the function and wait for more data.
    if (avail < 8) return;
  }

  // Packet object to store the packet data in.
  libcomp::Packet p;

  // Read in the sizes (without removing them from the socket buffer).
  mServerSocket->peek(p.Direct(8), 8);

  // Calculate the padded and real size of the packet.
  uint32_t paddedSize = p.ReadU32Big();
  uint32_t realSize = p.ReadU32Big();

  // The real size is currently ignored.
  Q_UNUSED(realSize);

  // If the entire packet isn't buffered, return and wait for more data.
  if (avail < (paddedSize + 8)) return;

  // Go back and get ready to read in the packet.
  p.Rewind();

  // Read in the packet.
  mServerSocket->read(p.Direct(paddedSize + 8), paddedSize + 8);

  // Decrypt the packet.
  decryptPacket(&mServerCryptData.key, p);

  // Log the packet.
  logPacket(p, 1);

  // Copy the packet so decompress doesn't overwrite it.
  libcomp::Packet copy;

  // Write the packet.
  copy.WriteArray(p.Data(), p.Size());

  // Rewind to the beginning of the packet.
  copy.Rewind();

  // Check for the server switch command.
  if (packetHasServerSwitch(copy)) {
    // Rewrite the packet.
    rewriteServerSwitchPacket(p);
  }

  // Encrypt the packet.
  encryptPacket(&mClientCryptData.key, p);

  // Send the packet to the client.
  mClientSocket->write((char*)p.Data(), p.Size());
  mClientSocket->flush();

  // If there is more data to read for the server, read it now.
  if (mServerSocket->bytesAvailable() >= 8) serverReady();
}

bool ChannelConnection::packetHasServerSwitch(libcomp::Packet& p) {
  // This is how big a "gzip" packet has to be
  // to read everything before the actual data.
  if (p.Size() < 24) return false;

  // Make sure we are at the right spot.
  p.Rewind();

  // Read the sizes of the packet.
  uint32_t paddedSize = p.ReadU32Big();
  uint32_t realSize = p.ReadU32Big();

  // Calculate how much data is padding.
  uint32_t padding = paddedSize - realSize;

  // Read the gzip "magic".
  uint32_t gzip = p.ReadU32Big();

  // All packets start with this.
  if (gzip != 0x677A6970) return false;

  // Get the uncompressed and compressed size.
  int32_t uncompressed_size = p.ReadS32Little();
  int32_t compressed_size = p.ReadS32Little();

  // Get the compression level.
  uint32_t lv6 = p.ReadU32Big();

  // The compression level should always be the same.
  if (lv6 != 0x6C763600) return false;

  // Make sure we have enough data left to decompress the entire packet.
  if (p.Left() != (static_cast<uint32_t>(compressed_size) + padding))
    return false;

  // If the compressed and uncompressed sizes differ, decompress the packet.
  if (compressed_size != uncompressed_size) {
    // Decompress the packet and get the uncompressed size.
    int32_t sz = p.Decompress(compressed_size);

    // Check the uncompressed size matches the recorded size.
    if (sz != uncompressed_size) return false;

    // There is no padding anymore.
    padding = 0;
  }

  // Default to no switch packet.
  bool hasSwitchPacket = false;

  // Loop through and check each command.
  while (p.Left() > padding) {
    // Make sure there is enough data.
    if (p.Left() < 6) return false;

    // Skip over the big endian size.
    p.Skip(2);

    // Read the command start, size, and code.
    uint32_t cmd_start = p.Tell();
    uint16_t cmd_size = p.ReadU16Little();
    uint16_t code = p.ReadU16Little();

    // With no data, the command size is 4 bytes.
    if (cmd_size < 4) return false;

    // Check there is enough data left.
    if (p.Left() < (uint32_t)(cmd_size - 4)) return false;

    // Read the command.
    libcomp::Packet cmd(p.Data() + cmd_start + 4, cmd_size - 4u);

    // Check for the switch command.
    if (code == 0x0009 && cmd.ReadU32Little() == 14) hasSwitchPacket = true;

    // Move to the next command.
    p.Seek(cmd_start + cmd_size);
  }  // while(p.Left() > padding)

  // Skip the padding.
  p.Skip(padding);

  // Check that the entire packet was read.
  if (p.Left() != 0) return false;

  return hasSwitchPacket;
}

void ChannelConnection::rewriteServerSwitchPacket(libcomp::Packet& p) {
  // This is how big a "gzip" packet has to be
  // to read everything before the actual data.
  if (p.Size() < 24) return;

  // Make sure we are at the right spot.
  p.Rewind();

  // Read the sizes of the packet.
  uint32_t paddedSize = p.ReadU32Big();
  uint32_t realSize = p.ReadU32Big();

  // Calculate how much data is padding.
  uint32_t padding = paddedSize - realSize;
  uint32_t gzip = p.ReadU32Big();

  // All packets start with this.
  if (gzip != 0x677A6970) return;

  // Get the uncompressed and compressed size.
  int32_t uncompressed_size = p.ReadS32Little();
  int32_t compressed_size = p.ReadS32Little();

  // Get the compression level.
  uint32_t lv6 = p.ReadU32Big();

  // The compression level should always be the same.
  if (lv6 != 0x6C763600) return;

  // Make sure we have enough data left to decompress the entire packet.
  if (p.Left() != (static_cast<uint32_t>(compressed_size) + padding)) return;

  // If the compressed and uncompressed sizes differ, decompress the packet.
  if (compressed_size != uncompressed_size) {
    // Decompress the packet and get the uncompressed size.
    int32_t sz = p.Decompress(compressed_size);

    // Check the uncompressed size matches the recorded size.
    if (sz != uncompressed_size) return;

    // There is no padding anymore.
    padding = 0;
  }

  // Create a buffer to write the commands into.
  libcomp::Packet packetQueue;

  // Add the generic header information to the buffer.
  packetQueue.WriteBlank(8);
  packetQueue.WriteArray("gzip", 4);
  packetQueue.WriteBlank(8);
  packetQueue.WriteArray("lv6", 4);

  // Loop through and check each command.
  while (p.Left() > padding) {
    // Make sure there is enough data.
    if (p.Left() < 6) return;

    // Skip over the big endian size.
    p.Skip(2);

    // Read the command start, size, and code.
    uint32_t cmd_start = p.Tell();
    uint16_t cmd_size = p.ReadU16Little();
    uint16_t code = p.ReadU16Little();

    // With no data, the command size is 4 bytes.
    if (cmd_size < 4) return;

    // Check there is enough data left.
    if (p.Left() < (uint32_t)(cmd_size - 4)) return;

    // Read the command.
    libcomp::Packet cmd(p.Data() + cmd_start + 4, cmd_size - 4u);

    // Check for the switch command.
    if (code == 0x0009 && cmd.ReadU32Little() == 14) {
      // Seed over the data we don't care about.
      cmd.Seek(4);

      // Read in the session key.
      uint32_t sessionKey = cmd.ReadU32Little();

      // Read the original address.
      libcomp::String origAddr =
          cmd.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

      // Save the original info.
      mServer->registerChannelKey(sessionKey, origAddr.C());

      // Generate the new address.
      QString addr =
          QString("%1:14666").arg(mClientSocket->localAddress().toString());

      // Log the message.
      logMessage(tr("Sending client to relay..."));
      logMessage(tr("Gave the client: %2").arg(addr));
      logMessage(tr("Original address: %2").arg(origAddr.C()));

      // Adjust the command size.
      uint16_t new_size = (uint16_t)(cmd_size - origAddr.Length() +
                                     static_cast<uint32_t>(addr.length()));

      // Write the new server switch command.
      packetQueue.WriteU16Big(new_size);
      packetQueue.WriteU16Little(new_size);
      packetQueue.WriteU16Little(code);
      packetQueue.WriteU32Little(14);
      packetQueue.WriteU32Little(sessionKey);
      packetQueue.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                                      addr.toUtf8().constData(), true);
    } else {
      // Write the command without modification.
      packetQueue.WriteU16Big(cmd_size);
      packetQueue.WriteU16Little(cmd_size);
      packetQueue.WriteU16Little(code);
      packetQueue.WriteArray(cmd.Data(), cmd.Size());
    }

    // Move to the next command.
    p.Seek(cmd_start + cmd_size);
  }  // while(p.Left() > padding)

  // Skip the padding.
  p.Skip(padding);

  // Check that the entire packet was read.
  if (p.Left() != 0) return;

  // Calculate the new packet size.
  uint32_t packetSize = packetQueue.Size() - 24;

  // Calculate the real and padded size.
  realSize = packetSize + 16;
  paddedSize = realSize - (realSize % 8);

  // Adjust the padded size.
  if (realSize != paddedSize) paddedSize += 8;

  // Rewind and fix the sizes for the packet.
  packetQueue.Rewind();
  packetQueue.WriteU32Big(paddedSize);
  packetQueue.WriteU32Big(realSize);
  packetQueue.Skip(4);
  packetQueue.WriteU32Little(packetSize);  // Uncompressed
  packetQueue.WriteU32Little(packetSize);  // Compressed

  // Compress the packet if the original packet was compressed.
  if (compressed_size != uncompressed_size) {
    // Seek to the data in the packet and compress the packet.
    packetQueue.Seek(24);
    packetQueue.Compress(static_cast<int32_t>(packetSize));

    // If they are equal, this packet might be
    // confused with an uncompressed one.
    if ((packetQueue.Size() - 24) == packetSize) return;

    // Update the compressed size.
    packetSize = packetQueue.Size() - 24;

    // Write the compressed size.
    packetQueue.Seek(16);
    packetQueue.WriteU32Little(packetSize);

    // Update the padded and real size.
    realSize = packetSize + 16;
    paddedSize = realSize - (realSize % 8);

    // Adjust the padded size.
    if (realSize != paddedSize) paddedSize += 8;

    // Write the new sizes.
    packetQueue.Rewind();
    packetQueue.WriteU32Big(paddedSize);
    packetQueue.WriteU32Big(realSize);
  }

  // Add in the padding.
  if (realSize != paddedSize) {
    packetQueue.End();
    packetQueue.WriteBlank(paddedSize - realSize);
  }

  // Rewind to the beginning of the packet.
  packetQueue.Rewind();

  // Replace the packet.
  p.Clear();
  p.Rewind();
  p.WriteArray(packetQueue.Data(), packetQueue.Size());
  p.Rewind();
}

void ChannelConnection::exchangeKeys() {
  libcomp::Packet p;

  // Read in the exchange packet.
  mServerSocket->read(p.Direct(529), 529);

  // Skip over the zero value.
  p.Seek(4);

  // Read in the server encryption data.
  mServerCryptData.base = p.ReadString32Big(libcomp::Convert::ENCODING_UTF8);
  mServerCryptData.prime = p.ReadString32Big(libcomp::Convert::ENCODING_UTF8);
  mServerCryptData.serverPublic =
      p.ReadString32Big(libcomp::Convert::ENCODING_UTF8);

  // If the packet didn't read write, disconnect from the server.
  if (p.Left() != 0) return serverLost();

  // Generate the client public to send to the server.
  mServerCryptData.secret = libcomp::Crypto::GenerateRandom();
  mServerCryptData.clientPublic = libcomp::Crypto::GenDiffieHellman(
      mServerCryptData.base, mServerCryptData.prime, mServerCryptData.secret);

  // Generate the shared secret based on the data from the server.
  mServerCryptData.sharedKey = libcomp::Crypto::GenDiffieHellman(
      mServerCryptData.serverPublic, mServerCryptData.prime,
      mServerCryptData.secret);
  mServerCryptData.keys = QByteArray::fromHex(mServerCryptData.sharedKey.C());

  // Set the shared encryption key.
  BF_set_key(&mServerCryptData.key, 8,
             (uchar*)mServerCryptData.keys.constData());

  // Send the client side of the key exchange.
  {
    libcomp::Packet reply;

    // Write the client public to the packet.
    reply.WriteString32Big(libcomp::Convert::ENCODING_UTF8,
                           mServerCryptData.clientPublic);

    // Send the packet.
    mServerSocket->write((char*)reply.Data(), reply.Size());
    mServerSocket->flush();
  }

  // Send login packet.
  if (mClientLoginPacket) {
    libcomp::Packet reply;

    // Write the login packet.
    reply.WriteArray(mClientLoginPacket, mClientLoginPacketSize);

    // Encrypt the packet.
    encryptPacket(&mServerCryptData.key, reply);

    // Send the packet.
    mServerSocket->write((char*)reply.Data(), reply.Size());
    mServerSocket->flush();
  }

  // Send packets that came in since the hello.
  foreach (QByteArray oldPacket, mPacketBuffer) {
    libcomp::Packet copy;

    // Write the packet.
    copy.WriteArray(oldPacket.constData(),
                    static_cast<uint32_t>(oldPacket.size()));

    // Encrypt the packet.
    encryptPacket(&mServerCryptData.key, copy);

    // Send the packet.
    mServerSocket->write(copy.Data(), copy.Size());
    mServerSocket->flush();
  }

  // Clear the packet buffer.
  mPacketBuffer.clear();

  // Set the server state.
  mServerState = Encrypted;

  // Free the login packet.
  delete mClientLoginPacket;
  mClientLoginPacket = 0;
}

void ChannelConnection::logMessage(const QString& msg) {
  // Prepend the timestamp to the message.
  QString final = tr("%1 %2").arg(timestamp()).arg(msg);

#ifdef COMP_LOGGER_HEADLESS
  // Log the message to standard output.
  std::cout << final.toLocal8Bit().constData() << std::endl;
#else   // COMP_LOGGER_HEADLESS
  // Add the message into the main window.
  mServer->addLogMessage(final);
#endif  // COMP_LOGGER_HEADLESS
}

void ChannelConnection::logPacket(libcomp::Packet& p, uint8_t source) {
  // 0 - Packet came from client.
  // 1 - Packet came from server.

  // Rewind to the beginning of the packet.
  p.Rewind();

  // Read in the size of the packet.
  uint32_t size = p.ReadU32Big() + 8;

  // If there is no size, assume the size is the entire packet.
  if (size == 0) size = p.Size();

  // Get the current time.
  time_t stamp = QDateTime::currentDateTime().toTime_t();

  // Get the current time in microseconds.
  uint64_t micro = microtime();

  // Only bother if the log file is open.
  if (mCaptureLog.isOpen()) {
    // Write the packet to the log.
    mCaptureLog.write((char*)&source, sizeof(source));
    mCaptureLog.write((char*)&stamp, 8);
    mCaptureLog.write((char*)&micro, 8);
    mCaptureLog.write((char*)&size, sizeof(size));
    mCaptureLog.write(p.Data(), size);
  }

  // Generate the packet to send to capgrep.
  QByteArray d;
  d.append((char*)&mClientID, 4);
  d.append((char*)&source, sizeof(source));
  d.append((char*)&stamp, 8);
  d.append((char*)&micro, 8);
  d.append((char*)&size, sizeof(size));
  d.append(p.Data(), static_cast<int>(size));

  // Send the packet to capgrep.
  mServer->addPacket(d);

  // Rewind to the beginning of the packet.
  p.Rewind();
}
