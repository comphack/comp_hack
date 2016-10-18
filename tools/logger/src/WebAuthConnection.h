/**
 * @file tools/logger/src/WebAuthConnection.h
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the class used to control a connection to the web auth
 * server.
 *
 * Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_LOGGER_SRC_WEBAUTHCONNECTION_H
#define TOOLS_LOGGER_SRC_WEBAUTHCONNECTION_H

#include <PushIgnore.h>
#include <QByteArray>
#include <QStringList>
#include <QThread>
#include <QSslError>
#include <QSslSocket>
#include <QTcpSocket>
#include <PopIgnore.h>

#include <stdint.h>

class LoggerServer;

/**
 * Proxy connection between the logger and the web authentication server.
 */
class WebAuthConnection : public QThread
{
    Q_OBJECT

public:
    /**
     * Create a new web authentication connection.
     * @param server @ref LoggerServer this connection was made from.
     * @param socketDescriptor Netowrk socket for the connection.
     * @param clientID ID to identify this client in the log.
     * @param parent Parent object that this object belongs to.
     * Should remain 0.
     */
    WebAuthConnection(LoggerServer *server, qintptr socketDescriptor,
        uint32_t clientID, QObject *parent = 0);

    /**
     * Delete the connection.
     */
    virtual ~WebAuthConnection();

protected slots:
    /**
     * This method is called when the client closes the connection. The
     * connection to the web auth server will be closed and the connection
     * object will be deleted.
     */
    void clientLost();

    /**
     * This method is called when new data has arrived from the client. The
     * data will be parsed and then sent to the web auth server.
     */
    void clientReady();

    /**
     * This method is called when the web auth server closes the connection.
     * The connection to the client will be closed and the connection object
     * will be deleted.
     */
    void serverLost();

    /**
     * This method is called when new data has arrived from the web auth
     * server. The data will be parsed and then sent to the client.
     */
    void serverReady();

    /**
     * Parse an HTTP request.
     */
    void parseRequest();

    /**
     * Handle any SSL errors that have occured.
     * @param errors List of SSL errors.
     */
    void sslErrors(const QList<QSslError>& errors);

    /**
     * Handle any SSL peer verification errors.
     * @param error Peer verification error.
     */
    void peerVerifyError(const QSslError& error);

protected:
    /**
     * This method is called when the connection thread starts executing.
     */
    virtual void run();

    /**
     * Generate a timestamp to be used in the log.
     * @returns The timestamp string.
     */
    QString timestamp() const;

    /**
     * Log a message to the console and GUI.
     * @param msg Message to log.
     */
    void logMessage(const QString& msg);

private:
    /// Logger server this connection belongs to.
    LoggerServer *mServer;

    /// Host name of the target server.
    QString mHost;

    /// Buffer of the most recent HTTP request.
    QByteArray mBuffer;

    /// Request line of the most recent HTTP request.
    QString mReqLine;

    /// List of HTTP headers in the order they occured.
    QStringList mHeaderKeys;

    /// Map of the HTTP headers and their values.
    QHash<QString, QString> mHeaders;

    /// Connection to the client.
    QTcpSocket *mClientSocket;

    /// Connection to the target server.
    QSslSocket *mServerSocket;

    /// IP address of the client connection.
    QString mClientAddress;

    /// Socket descriptior of the client connection.
    qintptr mSocketDescriptor;

    /// Unique channel ID of this client connection.
    uint32_t mClientID;
};

#endif // TOOLS_LOGGER_SRC_WEBAUTHCONNECTION_H
