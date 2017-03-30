/**
 * @file tools/logger/src/LoggerServer.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Server objects to handle each connection type.
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

#include "LoggerServer.h"

#include "LobbyConnection.h"
#include "ChannelConnection.h"
#include "WebAuthConnection.h"

#include <QSettings>

#include <iostream>

// Default client versions.
#define CLIENT_VERSION_US (1769)
#define CLIENT_VERSION_JP (1666)

// Default lobby server addresses.
#define LOBBY_ADDRESS_US "127.0.0.1"
#define LOBBY_ADDRESS_JP "127.0.0.1"

// Default web auth URL.
#define WEB_AUTH_URL "https://127.0.0.1/authsv/"

// Ports to use for each server.
#define PORT_LOBBY_SERVER   (10666)
#define PORT_CHANNEL_SERVER (14666)
#define PORT_WEBAUTH_SERVER (10999)

LobbyServer::LobbyServer(LoggerServer *server, QObject *p) : QTcpServer(p),
    mServer(server), mNextClientID(0)
{
}

LobbyServer::~LobbyServer()
{
}

void LobbyServer::incomingConnection(qintptr fd)
{
    // Create the lobby connection, ensure that the object is deleted when the
    // thread exits and start the thread. Pass the connection the next valid
    // client ID and then increment the client ID for the next connection.
    LobbyConnection *conn = new LobbyConnection(mServer, fd,
        mNextClientID++, this);

    connect(conn, SIGNAL(finished()), conn, SLOT(deleteLater()));

    conn->start();
}

ChannelServer::ChannelServer(LoggerServer *server, QObject *p) : QTcpServer(p),
    mServer(server), mNextClientID(0)
{
}

ChannelServer::~ChannelServer()
{
}

void ChannelServer::incomingConnection(qintptr fd)
{
    // Create the channel connection, ensure that the object is deleted when
    // the thread exits and start the thread. Pass the connection the next
    // valid client ID and then increment the client ID for the next
    // connection.
    ChannelConnection *conn = new ChannelConnection(mServer, fd,
        mNextClientID++, this);

    connect(conn, SIGNAL(finished()), conn, SLOT(deleteLater()));

    conn->start();
}

WebAuthServer::WebAuthServer(LoggerServer *server, QObject *p) : QTcpServer(p),
    mServer(server), mNextClientID(0)
{
}

WebAuthServer::~WebAuthServer()
{
}

void WebAuthServer::incomingConnection(qintptr fd)
{
    // Create the web authentication connection, ensure that the object is
    // deleted when the thread exits and start the thread.
    WebAuthConnection *conn = new WebAuthConnection(mServer, fd,
        mNextClientID++, this);

    connect(conn, SIGNAL(finished()), conn, SLOT(deleteLater()));

    conn->start();
}

LoggerServer::LoggerServer(QObject *p) : QObject(p)
{
    // Create each server object (lobby, channel, web auth).
    mLobbyServer = new LobbyServer(this, 0);
    mChannelServer = new ChannelServer(this, 0);
    mWebAuthServer = new WebAuthServer(this, 0);

    QSettings settings;

    // Load the saved values for each setting or the default value if no
    // setting 
    mVersionUS = settings.value("us/version", CLIENT_VERSION_US).toUInt();
    mAddressUS = settings.value("us/address", LOBBY_ADDRESS_US).toString();

    mVersionJP = settings.value("jp/version", CLIENT_VERSION_JP).toUInt();
    mAddressJP = settings.value("jp/address", LOBBY_ADDRESS_JP).toString();

    mWebAuthJP = settings.value("jp/webauth", WEB_AUTH_URL).toString();
    mWebAuthJPEnabled = settings.value("jp/webauthenabled",
        false).toBool();

    mLobbyLogEnabled = settings.value("savelobby", true).toBool();
    mChannelLogEnabled = settings.value("savechannel", true).toBool();
}

LoggerServer::~LoggerServer()
{
    // Delete all the server objects.
    delete mLobbyServer;
    delete mChannelServer;
    delete mWebAuthServer;
}

QString LoggerServer::capturePath() const
{
    // Return the path to the capture directory.
    return mCapturePath;
}

void LoggerServer::setCapturePath(const QString& path)
{
    // Set the path to the capture directory.
    mCapturePath = path;
}

QString LoggerServer::retrieveChannelKey(uint32_t key)
{
    // If the key is not in the map, return an empty string. If the key is
    // found, return it's associated server address.
    if( !mChannelKeys.contains(key) )
        return QString();

    return mChannelKeys.value(key);
}

void LoggerServer::registerChannelKey(uint32_t key, const QString& addr)
{
    // Add the key and it's associated server address to the map. If an address
    // with the key provided already exists, it will be replaced by the new
    // address.
    mChannelKeys[key] = addr;
}

void LoggerServer::startServer()
{
    // Listen on the port for client connections.
    if( !mLobbyServer->listen(QHostAddress::AnyIPv4, PORT_LOBBY_SERVER) )
    {
        QString msg  = QString("Lobby server: failed to listen on port "
            "%1.").arg(PORT_LOBBY_SERVER);

        addLogMessage(msg);

        std::cerr << msg.toLocal8Bit().constData() << std::endl;

        return;
    }

    // Listen on the port for client connections.
    if( !mChannelServer->listen(QHostAddress::AnyIPv4, PORT_CHANNEL_SERVER) )
    {
        QString msg  = QString("Channel server: failed to listen on port "
            "%1.").arg(PORT_CHANNEL_SERVER);

        addLogMessage(msg);

        std::cerr << msg.toLocal8Bit().constData() << std::endl;

        return;
    }

    // Listen on the port for client connections.
    if( !mWebAuthServer->listen(QHostAddress::AnyIPv4, PORT_WEBAUTH_SERVER) )
    {
        QString msg  = QString("WebAuth server: failed to listen on port "
            "%1.").arg(PORT_WEBAUTH_SERVER);

        addLogMessage(msg);

        std::cerr << msg.toLocal8Bit().constData() << std::endl;

        return;
    }

#ifdef COMP_LOGGER_HEADLESS
    std::cout << "Server Ready" << std::endl;
    std::cout << "----------------------------------------"
        << "----------------------------------------" << std::endl;
#else // COMP_LOGGER_HEADLESS
    addLogMessage("Server Ready");
#endif // COMP_LOGGER_HEADLESS
}

void LoggerServer::setVersionUS(uint32_t ver)
{
    // Set the expected US client version and save the setting.
    QSettings().setValue("us/version", ver);

    mVersionUS = ver;
}

void LoggerServer::setVersionJP(uint32_t ver)
{
    // Set the expected JP client version and save the setting.
    QSettings().setValue("jp/version", ver);

    mVersionJP = ver;
}

void LoggerServer::setAddressUS(const QString& addr)
{
    // Set the US lobby server address and save the setting.
    QSettings().setValue("us/address", addr);

    mAddressUS = addr;
}

void LoggerServer::setAddressJP(const QString& addr)
{
    // Set the JP lobby server address and save the setting.
    QSettings().setValue("jp/address", addr);

    mAddressJP = addr;
}

void LoggerServer::setWebAuthJP(const QString& url)
{
    // Set the web authentication sever URL and save the setting.
    QSettings().setValue("jp/webauth", url);

    mWebAuthJP = url;
}

bool LoggerServer::isWebAuthJPEnabled() const
{
    // Return if the web authentication feature is enabled.
    return mWebAuthJPEnabled;
}

void LoggerServer::setWebAuthJPEnabled(bool enabled)
{
    // Set if the web authentication feature is enabled and save the setting.
    QSettings().setValue("jp/webauthenabled", enabled);

    mWebAuthJPEnabled = enabled;
}

bool LoggerServer::isLobbyLogEnabled() const
{
    // Return if the lobby connections should save a capture file.
    return mLobbyLogEnabled;
}

void LoggerServer::setLobbyLogEnabled(bool enabled)
{
    // Set if the lobby connections should save a capture file.
    QSettings().setValue("savelobby", enabled);

    mLobbyLogEnabled = enabled;
}

bool LoggerServer::isChannelLogEnabled() const
{
    // Return if the channel connections should save a capture file.
    return mChannelLogEnabled;
}

void LoggerServer::setChannelLogEnabled(bool enabled)
{
    // Set if the channel connections should save a capture file.
    QSettings().setValue("savechannel", enabled);

    mChannelLogEnabled = enabled;
}

uint32_t LoggerServer::usVersion() const
{
    // Return the expected US client version.
    return mVersionUS;
}

uint32_t LoggerServer::jpVersion() const
{
    // Return the expected JP client version.
    return mVersionJP;
}

QString LoggerServer::usAddress() const
{
    // Return the address of the US lobby server.
    return mAddressUS;
}

QString LoggerServer::jpAddress() const
{
    // Return the address of the JP lobby server.
    return mAddressJP;
}

QString LoggerServer::jpWebAuth() const
{
    // Return the URL of the web authentication server.
    return mWebAuthJP;
}

void LoggerServer::addLogMessage(const QString& msg)
{
    // Emit the log message as a signal so the GUI may respond to it.
    emit message(msg);
}

void LoggerServer::addPacket(const QByteArray& p)
{
    // Emit the packet as a signal so the GUI may respond to it. The live
    // connection to capgrep is handled by the GUI code (because it's only)
    // avaliable in the GUI version. This function or signal could be used to
    // hook events to certain packets for scripting and injection.
    emit packet(p);
}
