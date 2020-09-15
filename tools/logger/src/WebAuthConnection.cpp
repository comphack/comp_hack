/**
 * @file tools/logger/src/WebAuthConnection.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the class used to control a connection to the web auth
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

#include "WebAuthConnection.h"

#include "LoggerServer.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QDateTime>
#include <QRegExp>
#include <QUrl>
#include <iostream>

// Stop ignoring warnings
#include <PopIgnore.h>

#ifndef COMP_LOGGER_HEADLESS

// Ignore warnings
#include <PopIgnore.h>

// Qt Includes
#include <QMessageBox>

// Stop ignoring warnings
#include <PushIgnore.h>

#endif  // COMP_LOGGER_HEADLESS

WebAuthConnection::WebAuthConnection(LoggerServer* server,
                                     qintptr socketDescriptor,
                                     uint32_t clientID, QObject* p)
    : QThread(p),
      mServer(server),
      mClientSocket(0),
      mServerSocket(0),
      mSocketDescriptor(socketDescriptor),
      mClientID(clientID) {}

WebAuthConnection::~WebAuthConnection() {}

void WebAuthConnection::run() {
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
  logMessage(tr("Client %1 connected to the web auth server from %2")
                 .arg(mClientID)
                 .arg(mClientAddress));

  // Start the thread.
  exec();
}

void WebAuthConnection::sslErrors(const QList<QSslError>& errors) {
  // Log each SSL error.
  foreach (QSslError err, errors) {
    logMessage(tr("Client %1 experienced the following SSL error: %2")
                   .arg(mClientID)
                   .arg(err.errorString()));
  }
}

void WebAuthConnection::peerVerifyError(const QSslError& err) {
  // Log the peer verification error.
  logMessage(tr("Client %1 experienced the following error: %2")
                 .arg(mClientID)
                 .arg(err.errorString()));
}

QString WebAuthConnection::timestamp() const {
  // Generate a timestamp.
  return QDateTime::currentDateTime().toString(Qt::ISODate);
}

void WebAuthConnection::clientLost() {
  // Add log message about client disconnect.
  logMessage(
      tr("Client %1 disconnected from the web auth server").arg(mClientID));

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

void WebAuthConnection::clientReady() {
  // If the connection to the target server has not been made, make it.
  if (!mServerSocket) {
    // Get the URL of the server.
    QString webAuth = mServer->jpWebAuth();

    // Set the hostname of the server.
    mHost = QUrl(webAuth).host();

    // Create the socket and turn off peer verification.
    mServerSocket = new QSslSocket;
    mServerSocket->setPeerVerifyMode(QSslSocket::VerifyNone);

    // Connect the needed signals for the server socket.
    connect(mServerSocket, SIGNAL(readyRead()), this, SLOT(serverReady()),
            Qt::DirectConnection);
    connect(mServerSocket, SIGNAL(disconnected()), this, SLOT(serverLost()),
            Qt::DirectConnection);
    connect(mServerSocket, SIGNAL(sslErrors(const QList<QSslError>&)), this,
            SLOT(sslErrors(const QList<QSslError>&)), Qt::DirectConnection);
    connect(mServerSocket, SIGNAL(peerVerifyError(const QSslError&)), this,
            SLOT(peerVerifyError(const QSslError&)), Qt::DirectConnection);

    if (webAuth.length() >= 5 && webAuth.left(5) == "https") {
      // Check for SSL support.
      if (!QSslSocket::supportsSsl()) {
        logMessage(
            tr("Failed to create the SSL connection because there is no SSL "
               "support compiled into the application."));
      }

      // Connect and negotiate an SSL connection.
      mServerSocket->setProtocol(QSsl::TlsV1_0);
      mServerSocket->connectToHostEncrypted(mHost,
                                            (quint16)QUrl(webAuth).port(443));
      mServerSocket->waitForEncrypted();
    } else {
      // Connect to the server using standard HTTP.
      mServerSocket->connectToHost(mHost, (quint16)QUrl(webAuth).port(80));
    }
  }

  // Buffer the new data.
  mBuffer.append(mClientSocket->readAll());

  // Parse the buffer as request data if the header has been parsed.
  if (!mReqLine.isEmpty()) return parseRequest();

  // Calculate the size of the header (or -1 for an incomplete header).
  int headerSize = mBuffer.indexOf("\r\n\r\n");

  // If we don't have a complete header, keep waiting.
  if (headerSize <= 0) return;

  // Copy the headers into strings.
  QStringList headers =
      QString::fromLatin1(mBuffer.left(headerSize)).split("\r\n");

  // Remove the header from the buffer.
  mBuffer = mBuffer.mid(headerSize + 4);

  // Copy the request line.
  mReqLine = headers.takeFirst();

  // Regular expression to parse the request line with.
  QRegExp reqLineMatcher("(GET|POST) ([^\\s]+) HTTP/1.1");

  // Get the path from the target server URL.
  QString path = QUrl(mServer->jpWebAuth()).path();

  // Make sure the path ends with a trailing slash.
  if (path.right(1) != "/") path += "/";

  // Replace the path if the target server resides in a sub-path.
  if (path != "/" && reqLineMatcher.exactMatch(mReqLine)) {
    // If the path is the root simply use the given path. If the path
    // contains a trailing slash, remove it before making the replacement.
    if (reqLineMatcher.cap(2) == "/") {
      mReqLine = tr("%1 %2 HTTP/1.1").arg(reqLineMatcher.cap(1)).arg(path);
    } else if (reqLineMatcher.cap(2).left(1) == "/") {
      mReqLine = tr("%1 %2 HTTP/1.1")
                     .arg(reqLineMatcher.cap(1))
                     .arg(path + reqLineMatcher.cap(2).mid(1));
    } else {
      mReqLine = tr("%1 %2 HTTP/1.1")
                     .arg(reqLineMatcher.cap(1))
                     .arg(path + reqLineMatcher.cap(2));
    }
  }

  // Regular expression to match an HTTP header.
  QRegExp headerMatcher("([^\\:]+)\\:(.+)");

  // Split each header into it's key and value.
  foreach (QString header, headers) {
    // If the parsing failed, close the connection.
    if (!headerMatcher.exactMatch(header)) {
      mClientSocket->disconnect();

      return;
    }

    // Read the key and convert it to lower case.
    QString key = headerMatcher.cap(1).trimmed().toLower();

    // Add the key to the list.
    mHeaderKeys.append(key);

    // Add the value to the map.
    mHeaders[key] = header;
  }

  // Patch the headers (replace the hostname).
  if (mHeaders.contains("host"))
    mHeaders["host"] = QString("Host: %1").arg(mHost);

  // Parse any request (post) data that may be required.
  parseRequest();
}

void WebAuthConnection::parseRequest() {
  // Default to a content length of zero.
  int contentSize = 0;

  // If the content length header is found, set the content length.
  if (mHeaders.contains("content-length")) {
    // Regular expression for parsing.
    QRegExp headerMatcher("([^\\:]+)\\:(.+)");

    // If there was a parse error, close the connection.
    if (!headerMatcher.exactMatch(mHeaders.value("content-length"))) {
      mClientSocket->disconnect();

      return;
    }

    // Convert the content size to an integer.
    contentSize = headerMatcher.cap(2).trimmed().toInt();

    // If the content length isn't valid, close the connection.
    if (contentSize < 1) {
      mClientSocket->disconnect();

      return;
    }
  }

  // If we don't have it all yet, keep waiting.
  if (mBuffer.size() < contentSize) return;

  // Where to copy the headers to.
  QStringList headers;

  // Retrieve each hader and add it to the list.
  foreach (QString key, mHeaderKeys)
    headers.append(mHeaders.value(key));

  // Send the HTTP headers to the target server.
  mServerSocket->write(tr("%1\r\n%2\r\n\r\n")
                           .arg(mReqLine)
                           .arg(headers.join("\r\n"))
                           .toLatin1());

  // If there is request content (post dat), send that to the server.
  if (contentSize) {
    // Get the request content from the buffer.
    QByteArray content = mBuffer.left(contentSize);

#ifndef COMP_LOGGER_HEADLESS
    // This is an example of how you can obtain the login information
    // for an account by simply using a HTTPS proxy. Find the PASS post
    // variable.
    if (content.indexOf("PASS") >= 0) {
      // Slit the request content into paris of keys and values.
      QStringList pairs = QString::fromLatin1(content).split("&");

      // Regular expression for parsing the POST data.
      QRegExp pairMatcher("([^\\=]+)\\=(.*)");

      // Variable to store the post data in.
      QMap<QString, QString> post;

      // For each key-value pair, parse and add it to the map.
      foreach (QString pair, pairs) {
        // If the string doesn't match the expected format, skip it.
        if (!pairMatcher.exactMatch(pair)) continue;

        // Add the pair to the map.
        post[QUrl::fromPercentEncoding(
            pairMatcher.cap(1).trimmed().toLatin1())] =
            QUrl::fromPercentEncoding(pairMatcher.cap(2).trimmed().toLatin1());
      }

      // Add the username and password of the client to the log.
      logMessage(
          tr("Client %1 username: %2").arg(mClientID).arg(post.value("ID")));
      logMessage(
          tr("Client %1 password: %2").arg(mClientID).arg(post.value("PASS")));
    }
#endif  // COMP_LOGGER_HEADLESS

    // Send the content to the target server.
    mServerSocket->write(content);

    // Remove the content from the buffer.
    mBuffer = mBuffer.mid(contentSize);
  }

  // Clear the variables for the next request.
  mReqLine.clear();
  mHeaderKeys.clear();
  mHeaders.clear();
}

void WebAuthConnection::logMessage(const QString& msg) {
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

void WebAuthConnection::serverLost() {
  // If the server has disconnected us, disconnect the client.
  mClientSocket->disconnect();
}

void WebAuthConnection::serverReady() {
  // If the server has no new data, check again later.
  if (mServerSocket->bytesAvailable() <= 0) return;

  // Send the data directly to the client.
  mClientSocket->write(mServerSocket->readAll());
}
