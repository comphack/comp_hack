/**
 * @file tools/logger/src/LoggerServer.h
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Server objects to handle each connection type.
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

#ifndef TOOLS_LOGGER_SRC_LOGGERSERVER_H
#define TOOLS_LOGGER_SRC_LOGGERSERVER_H

#include <PushIgnore.h>
#include <QMap>
#include <QString>
#include <QObject>
#include <QTcpServer>
#include <PopIgnore.h>

#include <stdint.h>

class LoggerServer;

/**
 * The lobby server consists of the character creation and selection. This
 * class will handle incoming lobby server connections and create a
 * @ref LobbyConnection object to proxy (and log) the connection between the
 * client and the target lobby server.
 */
class LobbyServer : public QTcpServer
{
    Q_OBJECT

public:
    /**
     * Construct the LobbyServer.
     * @param server Main logger server object controlling all the components.
     * @param parent Parent object. This may be left to it's default value.
     */
    LobbyServer(LoggerServer *server, QObject *parent = 0);

    /**
     * Default deconstructor for the object.
     */
    virtual ~LobbyServer();

protected:
    /**
     * Handle new connection to the lobby server. When the client connects to
     * the logger lobby server, the socket is passed to this method. This
     * method will create a new @ref LobbyConnection for that socket.
     * @param socketDescriptor Operating system specific socket descriptor or
     * handle.
     */
    virtual void incomingConnection(qintptr socketDescriptor);

private:
    /// The logger server that this server belongs to.
    LoggerServer *mServer;

    /// ID to use for the next new client connection.
    uint32_t mNextClientID;
};

/**
 * The channel server consists of the zone the character interacts with. Each
 * Each channel server may contain all zones or a subset of zones requiring
 * the client to switch to a different channel server for certsin zone
 * changes. This class will handle incoming channel server connections and
 * create a @ref ChannelConnection object to proxy (and log) the connection
 * between the client and the target channel server.
 */
class ChannelServer : public QTcpServer
{
    Q_OBJECT

public:
    /**
     * Construct the ChannelServer.
     * @param server Main logger server object controlling all the components.
     * @param parent Parent object. This may be left to it's default value.
     */
    ChannelServer(LoggerServer *server, QObject *parent = 0);

    /**
     * Default deconstructor for the object.
     */
    virtual ~ChannelServer();

protected:
    /**
     * Handle new connection to the channel server. When the client connects to
     * the logger channel server, the socket is passed to this method. This
     * method will create a new @ref ChannelConnection for that socket.
     * @param socketDescriptor Operating system specific socket descriptor or
     * handle.
     */
    virtual void incomingConnection(qintptr socketDescriptor);

private:
    /// The logger server that this server belongs to.
    LoggerServer *mServer;

    /// ID to use for the next new client connection.
    uint32_t mNextClientID;
};

/**
 * The WebAuth server is used to display the login dialog on the JP server. The
 * login dialog is created as an https embedded webpage. This class will handle
 * incoming web connections and create a @ref WebAuthConnection object to proxy
 * (and log) the connection between the client and the target website.
 */
class WebAuthServer : public QTcpServer
{
    Q_OBJECT

public:
    /**
     * Construct the WebAuthServer.
     * @param server Main logger server object controlling all the components.
     * @param parent Parent object. This may be left to it's default value.
     */
    WebAuthServer(LoggerServer *server, QObject *parent = 0);

    /**
     * Default deconstructor for the object.
     */
    virtual ~WebAuthServer();

protected:
    /**
     * Handle new connection to the web auth server. When the client connects
     * to the web auth server, the socket is passed to this method. This
     * method will create a new @ref WebAuthConnection for that socket.
     * @param socketDescriptor Operating system specific socket descriptor or
     * handle.
     */
    virtual void incomingConnection(qintptr socketDescriptor);

private:
    /// The logger server that this server belongs to.
    LoggerServer *mServer;

    /// ID to use for the next new client connection.
    uint32_t mNextClientID;
};

/**
 * The logger server class manages all other server objects and communication
 * between them and the GUI.
 */
class LoggerServer : public QObject
{
    Q_OBJECT

public:
    /**
     * Create a new LoggerServer object.
     * @param parent Parent object. This may be left to it's default value.
     */
    LoggerServer(QObject *parent = 0);

    /**
     * Delete the LoggerServer object.
     */
    virtual ~LoggerServer();

    /**
     * Get the capture directory path.
     * @returns Path to the capture directory.
     */
    QString capturePath() const;

    /**
     * Set the packet capture log path.
     * @param path Capture directory path to set.
     */
    void setCapturePath(const QString& path);

    /**
     * Start the server and listen for new client connections.
     */
    void startServer();

    /**
     * Retrive the original server address for the given channel login key.
     * @param key Channel login key provided by the lobby server.
     * @returns The associated server address or an empty string if the key
     * was not found.
     */
    QString retrieveChannelKey(uint32_t key);

    /**
     * Add a channel login key and it's original address. If the key already
     * exists, the old server address will be replaced with the new one.
     * @param key Channel login key provided by the lobby server.
     * @param addr Address of the target channel server.
     */
    void registerChannelKey(uint32_t key, const QString& addr);

    /**
     * Set the client version for the US lobby server connection. This version
     * is used to determine which client is connecting and which target lobby
     * server should be used.
     * @param ver Version of the US client. The version is represented as an
     * integer. For example, the client version 1.234U would have the integer
     * representation 1.234.
     */
    void setVersionUS(uint32_t ver);

    /**
     * Set the client version for the JP lobby server connection. This version
     * is used to determine which client is connecting and which target lobby
     * server should be used.
     * @param ver Version of the JP client. The version is represented as an
     * integer. For example, the client version 1.234 would have the integer
     * representation 1.234.
     */
    void setVersionJP(uint32_t ver);

    /**
     * Set the address of the target US lobby server. The address should be in
     * the form: "lobby.server.ip.address. The port is assumed to be 10666.
     * @param addr Address of the target US lobby server.
     */
    void setAddressUS(const QString& addr);

    /**
     * Set the address of the target JP lobby server. The address should be in
     * the form: "lobby.server.ip.address. The port is assumed to be 10666.
     * @param addr Address of the target JP lobby server.
     */
    void setAddressJP(const QString& addr);

    /**
     * Set the URL of the target website. The URL should be exactly as it
     * appears in the webaccess.sdat file.
     * @param addr URL of the target website.
     */
    void setWebAuthJP(const QString& url);

    /**
     * Determine if the web authentication feature is enabled.
     * @returns true if the web authentication feature is enabled, false
     * otherwise.
     */
    bool isWebAuthJPEnabled() const;

    /**
     * Enable or disable the web authentication feature.
     * @param enabled true if the web authentication feature should be enabled,
     * false otherwise.
     */
    void setWebAuthJPEnabled(bool enabled);

    /**
     * Determine if lobby connections will save a capture file.
     * @returns true if lobby connections will save a capture file, false
     * otherwise.
     */
    bool isLobbyLogEnabled() const;

    /**
     * Enable or disable the creation of capture files by lobby connections.
     * @param enabled true if lobby connections will save a capture file, false
     * otherwise.
     */
    void setLobbyLogEnabled(bool enabled);

    /**
     * Determine if channel connections will save a capture file.
     * @returns true if channel connections will save a capture file, false
     * otherwise.
     */
    bool isChannelLogEnabled() const;

    /**
     * Enable or disable the creation of capture files by channel connections.
     * @param enabled true if channel connections will save a capture file,
     * false otherwise.
     */
    void setChannelLogEnabled(bool enabled);

    /**
     * Retrieve the expected US client version.
     * @returns Version of the US client.
     */
    uint32_t usVersion() const;

    /**
     * Retrieve the expected JP client version.
     * @returns Version of the JP client.
     */
    uint32_t jpVersion() const;

    /**
     * Retrieve the target US lobby server address.
     * @returns IP address of the target US lobby server.
     */
    QString usAddress() const;

    /**
     * Retrieve the target JP lobby server address.
     * @returns IP address of the target JP lobby server.
     */
    QString jpAddress() const;

    /**
     * Retrieve the target web authentication URL.
     * @returns URL of the target web authentication site.
     */
    QString jpWebAuth() const;

    /**
     * Add a log message to the GUI (if running the GUI version).
     * @param msg Message to log.
     */
    void addLogMessage(const QString& msg);

    /**
     * Send a packet to the packet analyser capgrep. The packet is only sent
     * if there is a valid live mode connection to capgrep.
     * @param p Packet to send to capgrep.
     */
    void addPacket(const QByteArray& p);

signals:
    /**
     * New message to be logged.
     * @param msg Message to be logged.
     */
    void message(const QString& msg);

    /**
     * New packet to be sent to capgrep.
     * @param p Packet to send to capgrep.
     */
    void packet(const QByteArray& p);

private:
    /// Expected US client version.
    uint32_t mVersionUS;

    /// Expected JP client version.
    uint32_t mVersionJP;

    /// Address of the target US lobby server.
    QString mAddressUS;

    /// Address of the target JP lobby server.
    QString mAddressJP;

    /// URL of the target web authentication server.
    QString mWebAuthJP;

    /// Variable to store if the web authentication feature is enabled.
    bool mWebAuthJPEnabled;

    /// Variable to store if lobby connections will save a capture file.
    bool mLobbyLogEnabled;

    /// Variable to store if channel connections will save a capture file.
    bool mChannelLogEnabled;

    /// Path to the directory to store capture files.
    QString mCapturePath;

    /// Lobby server object to manage lobby connections.
    LobbyServer *mLobbyServer;

    /// Channel server object to manage channel connections.
    ChannelServer *mChannelServer;

    /// Web authentication server object to manage web auth connections.
    WebAuthServer *mWebAuthServer;

    /// Mapping of channel keys to their target channel server address.
    QMap<uint32_t, QString> mChannelKeys;
};

#endif // TOOLS_LOGGER_SRC_LOGGERSERVER_H
