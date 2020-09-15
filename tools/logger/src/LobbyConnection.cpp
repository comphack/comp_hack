/**
 * @file tools/logger/src/LobbyConnection.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the class used to control a connection to the lobby
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

#include "LobbyConnection.h"

#include <Crypto.h>
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
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

static const uint32_t FORMAT_MAGIC = 0x504D4F43;  // COMP
static const uint32_t FORMAT_VER = 0x00010000;    // Major, Minor, Patch (1.0.0)

LobbyConnection::LobbyConnection(LoggerServer* server, qintptr socketDescriptor,
                                 uint32_t clientID, QObject* p)
    : QThread(p),
      mServer(server),
      mClientState(NotConnected),
      mServerState(NotConnected),
      mClientLoginPacket(0),
      mClientLoginPacketSize(0),
      mClientSocket(0),
      mServerSocket(0),
      mClientVer(0),
      mSocketDescriptor(socketDescriptor),
      mClientID(clientID) {}

LobbyConnection::~LobbyConnection() {
  // Delete the client login packet if it exists.
  delete mClientLoginPacket;
  mClientLoginPacket = 0;
}

void LobbyConnection::run() {
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
  // the hello magic packet.
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
  logMessage(tr("Client %1 connected to the lobby server from %2")
                 .arg(mClientID)
                 .arg(mClientAddress));

  // Set the state of the connection.
  mClientState = Connected;

  // Only open the log file if logging is enabled.
  if (mServer->isLobbyLogEnabled()) {
    // Get the current time.
    QDateTime time = QDateTime::currentDateTime();
    time_t stamp = time.toTime_t();

    // Generate the name of the capture file.
    QString filename = tr("%1.comp").arg(time.toString("yyyyMMddhhmmss"));

    // Get the full path of the capture file to be created.
    QString path = QDir(mServer->capturePath()).absoluteFilePath(filename);

    // Calculate the length of the client address string.
    uint32_t addrlen = static_cast<uint32_t>(mClientAddress.toUtf8().size());

    // Write the header to the log file.
    mCaptureLog.setFileName(path);
    mCaptureLog.open(QIODevice::WriteOnly);
    mCaptureLog.write((char*)&FORMAT_MAGIC, 4);
    mCaptureLog.write((char*)&FORMAT_VER, 4);
    mCaptureLog.write((char*)&stamp, 4);
    mCaptureLog.write((char*)&addrlen, 4);
    mCaptureLog.write(mClientAddress.toUtf8().constData(), addrlen);
  }

  // Start the thread.
  exec();
}

QString LobbyConnection::timestamp() const {
  // Generate a timestamp
  return QDateTime::currentDateTime().toString(Qt::ISODate);
}

void LobbyConnection::clientLost() {
  // If the client is not connected anymore, ignore.
  if (mClientState == NotConnected) return;

  // Set the client as disconnected so this function is not run again.
  mClientState = NotConnected;

  // Add log message about client disconnect.
  logMessage(tr("Client %1 disconnected from the lobby server").arg(mClientID));

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

void LobbyConnection::encryptPacket(const BF_KEY* key, libcomp::Packet& p) {
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

void LobbyConnection::decryptPacket(const BF_KEY* key, libcomp::Packet& p) {
  // Skip over the sizes.
  p.Seek(8);

  // Buffer to store the current block of data.
  uint32_t buffer[2];

  // Decrypt each block of data.
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

void LobbyConnection::encryptCustomPacket(const BF_KEY* key,
                                          libcomp::Packet& p) {
  // Calculate the size of the packet.
  uint32_t extra = (p.Size() - 8) % 8;
  uint32_t size = p.Size() - 8 - extra;

  // Seek back to the beginning of the packet.
  p.Rewind();

  // Write the sizes to the packet.
  p.WriteU32Big(size + (extra ? 8 : 0));
  p.WriteU32Big(size + extra);

  // Buffer to store the current block of data.
  uint32_t buffer[2];

  // Encrypt each block of data.
  for (uint32_t i = 0; i < size; i += 8) {
    // Read the unencrypted data from the packet into the buffer.
    p.ReadArray(buffer, 8);

    // Move the packet pointer back.
    p.Rewind(8);

    // Encrypt the data in the buffer.
    BF_encrypt(buffer, key);

    // Write the encrypted data back into the packet.
    p.WriteArray(buffer, 8);
  }

  // If there is extra data that needs to be padded,
  // pad and encrypt that data.
  if (extra) {
    // Zero the buffer to pad the data.
    memset(buffer, 0, 8);

    // Read the unencrypted data from the packet into the buffer.
    p.ReadArray(buffer, extra);

    // Move the packet pointer back.
    p.Rewind(extra);

    // Encrypt the data in the buffer.
    BF_encrypt(buffer, key);

    // Write the encrypted data back into the packet.
    p.WriteArray(buffer, 8);
  }

  // Seek back to the beginning of the packet.
  p.Rewind();
}

void LobbyConnection::clientReady() {
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

  // Seek past the packet sizes.
  p.Seek(8);

  // Calculate how much data is padding.
  uint32_t padding = paddedSize - realSize;

  // Loop through and check each command.
  while (p.Left() > padding) {
    // Make sure there is enough data.
    if (p.Left() < 6) return clientLost();

    // Skip over the big endian size.
    p.Skip(2);

    // Read the command start, size, and code.
    uint32_t cmd_start = p.Tell();
    uint16_t cmd_size = p.ReadU16Little();
    uint16_t code = p.ReadU16Little();

    // With no data, the command size is 4 bytes.
    if (cmd_size < 4) return clientLost();

    // Check there is enough data left.
    if (p.Left() < (uint32_t)(cmd_size - 4)) return clientLost();

    // If the command is a login packet, parse it
    // and start the server connection.
    if (code == 0x03 || code == 0x1B) {
      // Read the username.
      mUsername = p.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

      // If using an atlus login packet, read the password too.
      if (code == 0x1B) p.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

      // Save the client version.
      mClientVer = p.ReadU32Little();

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
      if (mServer->usVersion() == mClientVer)
        mServerSocket->connectToHost(mServer->usAddress(), 10666);
      else
        mServerSocket->connectToHost(mServer->jpAddress(), 10666);

      // Set the server state.
      mServerState = Connected;

      // If there is more data to read for the client, read it now.
      if (mClientSocket->bytesAvailable() >= 8) clientReady();

      return;
    }

    // Move to the next command.
    p.Seek(cmd_start + cmd_size);
  }  // while(p.Left() > padding)

  // Skip the padding.
  p.Skip(padding);

  // Check that the entire packet was read.
  if (p.Left() != 0) return clientLost();

  // Encrypt the packet.
  encryptPacket(&mServerCryptData.key, p);

  // Send the packet to the server.
  mServerSocket->write((char*)p.Data(), p.Size());
  mServerSocket->flush();

  // If there is more data to read for the client, read it now.
  if (mClientSocket->bytesAvailable() >= 8) clientReady();
}

void LobbyConnection::sendClientHello() {
  // The logger has connected to the server, send the connect magic.
  mServerSocket->write((char*)connectMagic, 8);
  mServerSocket->flush();
}

void LobbyConnection::serverLost() {
  // If the server has disconnected us, disconnect the client.
  clientLost();
}

void LobbyConnection::serverReady() {
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

  // Seek past the packet sizes.
  p.Seek(8);

  // Calculate how much data is padding.
  uint32_t padding = paddedSize - realSize;

  // Loop through and check each command.
  while (p.Left() > padding) {
    // Make sure there is enough data.
    if (p.Left() < 6) return serverLost();

    // Skip over the big endian size.
    p.Skip(2);

    // Read the command start, size, and code.
    uint32_t cmd_start = p.Tell();
    uint16_t cmd_size = p.ReadU16Little();
    uint16_t code = p.ReadU16Little();

    // With no data, the command size is 4 bytes.
    if (cmd_size < 4) return serverLost();

    // Check there is enough data left.
    if (p.Left() < (uint32_t)(cmd_size - 4)) return serverLost();

    // Check for the start game packet.
    if (code == 0x08) {
      // Re-write the packet to go to the logger channel.
      parseStartGamePacket(p);

      // We are sending something else.
      return;
    }

    // Move to the next command.
    p.Seek(cmd_start + cmd_size);
  }  // while(p.Left() > padding)

  // Skip the padding.
  p.Skip(padding);

  // Check that the entire packet was read.
  if (p.Left() != 0) return;

  // Encrypt the packet.
  encryptPacket(&mClientCryptData.key, p);

  // Send the packet to the client.
  mClientSocket->write((char*)p.Data(), p.Size());
  mClientSocket->flush();

  // If there is more data to read for the server, read it now.
  if (mServerSocket->bytesAvailable() >= 8) serverReady();
}

void LobbyConnection::exchangeKeys() {
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
  mServerCryptData.clientPublic =
      libcomp::Crypto::GenDiffieHellman(mServerCryptData.base,
                                        mServerCryptData.prime,
                                        mServerCryptData.secret)
          .RightJustified(256, '0');

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

  // Set the server state.
  mServerState = Encrypted;

  // Free the login packet.
  delete mClientLoginPacket;
  mClientLoginPacket = 0;
}

void LobbyConnection::logMessage(const QString& msg) {
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

void LobbyConnection::logPacket(libcomp::Packet& p, uint8_t source) {
  // 0 - Packet came from client.
  // 1 - Packet came from server.

  // Only bother if the log file is open.
  if (!mCaptureLog.isOpen()) return;

  // Rewind to the beginning of the packet.
  p.Rewind();

  // Read in the size of the packet.
  uint32_t size = p.ReadU32Big() + 8;

  // If there is no size, assume the size is the entire packet.
  if (size == 0) size = p.Size();

  // Get the current time.
  time_t stamp = QDateTime::currentDateTime().toTime_t();

  // Write the packet to the log.
  mCaptureLog.write((char*)&source, sizeof(source));
  mCaptureLog.write((char*)&stamp, 4);
  mCaptureLog.write((char*)&size, sizeof(size));
  mCaptureLog.write(p.Data(), size);

  // Rewind to the beginning of the packet.
  p.Rewind();
}

void LobbyConnection::parseStartGamePacket(libcomp::Packet& p) {
  // Read the session key.
  uint32_t sessionKey = p.ReadU32Little();

  // Read the original address.
  libcomp::String origAddr =
      p.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

  // Save the original info.
  mServer->registerChannelKey(sessionKey, origAddr.C());

  // Generate the address of the logger server.
  QString addr =
      QString("%1:14666").arg(mClientSocket->localAddress().toString());

  // Generate the start game packet.
  libcomp::Packet reply;
  reply.WriteBlank(8);
  reply.WriteU16Big(0x23);
  reply.WriteU16Little(0x23);
  reply.WriteU16Little(0x08);
  reply.WriteU32Little(sessionKey);
  reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                            addr.toUtf8().constData(), true);
  reply.WriteU8(3);

  // Adjust the packet sizes.
  reply.Seek(8);
  reply.WriteU16Big((uint16_t)(reply.Size() - 10));
  reply.WriteU16Little((uint16_t)(reply.Size() - 10));

  // Add a log message that indicates redirecting the client to the logger.
  logMessage(tr("Sending client %1 to the logger...").arg(mClientID));

  // Encrypt the packet.
  encryptCustomPacket(&mClientCryptData.key, reply);

  // Send the packet to the client.
  mClientSocket->write((char*)reply.Data(), reply.Size());
  mClientSocket->flush();
}
