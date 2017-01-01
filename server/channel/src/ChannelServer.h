/**
 * @file server/channel/src/ChannelServer.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel server class.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef SERVER_CHANNEL_SRC_CHANNELSERVER_H
#define SERVER_CHANNEL_SRC_CHANNELSERVER_H

// libcomp Includes
#include <InternalConnection.h>
#include <BaseServer.h>
#include <ManagerConnection.h>
#include <Worker.h>

// object Includes
#include <RegisteredServer.h>

namespace channel
{

/**
 * Channel server that handles client packets in game.
 */
class ChannelServer : public libcomp::BaseServer
{
public:
    /**
     * Create a new channel server.
     * @param config Pointer to a casted ChannelConfig that will contain properties
     *   every server has in addition to channel specific ones.
     * @param configPath File path to the location of the config to be loaded.
     */
    ChannelServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath);

    /**
     * Clean up the server.
     */
    virtual ~ChannelServer();

    /**
     * Initialize the database connection and do anything else that can fail
     * to execute that needs to be handled outside of a constructor.  This
     * calls the BaseServer version as well to perform shared init steps.
     * @param self Pointer to this server to be used as a reference in
     *   packet handling code.
     * @return true on success, false on failure
     */
    virtual bool Initialize(std::weak_ptr<BaseServer>& self);
    
    /**
     * Get the channel's RegisteredServer.
     * @return Pointer to the RegisteredServer
     */
    const std::shared_ptr<objects::RegisteredServer> GetRegisteredServer();

    /**
     * Get the world's RegisteredServer.
     * @return Pointer to the RegisteredServer
     */
    std::shared_ptr<objects::RegisteredServer> GetWorldRegisteredServer();
    
    /**
     * Set the world's RegisteredServer.
     * @param registeredServer Pointer to the world's RegisteredServer
     */
    void SetWorldRegisteredServer(const std::shared_ptr<
        objects::RegisteredServer>& registeredServer);
    
    /**
     * Get the world database.
     * @return Pointer to the world's database
     */
    std::shared_ptr<libcomp::Database> GetWorldDatabase() const;

    /**
     * Set the world database.
     * @param database Pointer to the world's database
     */
    void SetWorldDatabase(const std::shared_ptr<libcomp::Database>& database);
    
    /**
     * Get the lobby database.
     * @return Pointer to the lobby's database
     */
    std::shared_ptr<libcomp::Database> GetLobbyDatabase() const;

    /**
     * Set the lobby database.
     * @param database Pointer to the lobby's database
     */
    void SetLobbyDatabase(const std::shared_ptr<libcomp::Database>& database);

    /**
     * Register the channel with the lobby database.
     * @return true on success, false on failure
     */
    bool RegisterServer();

protected:
    /**
     * Create a connection to a newly active socket.
     * @param socket A new socket connection.
     * @return Pointer to the newly created connection
     */
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /// Pointer to the manager in charge of connection messages.
    std::shared_ptr<ManagerConnection> mManagerConnection;

    /// Pointer to the world's RegisteredServer.
    std::shared_ptr<objects::RegisteredServer> mWorldRegisteredServer;

    /// A shared pointer to the world database used by the server.
    std::shared_ptr<libcomp::Database> mWorldDatabase;

    /// A shared pointer to the main database used by the server.
    std::shared_ptr<libcomp::Database> mLobbyDatabase;

    /// Pointer to the channel's RegisteredServer.
    std::shared_ptr<objects::RegisteredServer> mRegisteredServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELSERVER_H
