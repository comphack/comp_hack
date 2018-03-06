/**
 * @file server/world/src/WorldServer.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief World server class.
 *
 * This file is part of the World Server (world).
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

#ifndef SERVER_WORLD_SRC_WORLDSERVER_H
#define SERVER_WORLD_SRC_WORLDSERVER_H

 // Standard C++11 Includes
#include <map>

// libcomp Includes
#include <BaseServer.h>
#include <InternalConnection.h>
#include <ManagerConnection.h>
#include <Worker.h>

// object Includes
#include <RegisteredChannel.h>
#include <RegisteredWorld.h>

// world Includes
#include "AccountManager.h"
#include "CharacterManager.h"
#include "WorldSyncManager.h"

namespace world
{

class WorldServer : public libcomp::BaseServer
{
public:
    /**
     * Create a new world server.
     * @param szProgram First command line argument for the application.
     * @param config Pointer to a casted WorldConfig that will contain properties
     *   every server has in addition to world specific ones.
     */
    WorldServer(const char *szProgram,
        std::shared_ptr<objects::ServerConfig> config,
        std::shared_ptr<libcomp::ServerCommandLineParser> commandLine);

    /**
     * Clean up the server.
     */
    virtual ~WorldServer();

    /**
     * Initialize the database connection and do anything else that can fail
     * to execute that needs to be handled outside of a constructor.  This
     * calls the BaseServer version as well to perform shared init steps.
     * @param self Pointer to this server to be used as a reference in
     *   packet handling code.
     * @return true on success, false on failure
     */
    virtual bool Initialize();

    /**
     * Do any initialize that should happen after the server is listening and
     * fully started.
     */
    virtual void FinishInitialize();

    /**
     * Get the RegisteredWorld.
     * @return Pointer to the RegisteredWorld
     */
    const std::shared_ptr<objects::RegisteredWorld> GetRegisteredWorld() const;

    /**
     * Get the RegisteredChannel of a channel currently connected to
     * by its connection pointer.
     * @param connection Pointer to the channel's connection.
     * @return Pointer to the RegisteredChannel
     */
    std::shared_ptr<objects::RegisteredChannel> GetChannel(
        const std::shared_ptr<libcomp::InternalConnection>& connection) const;

    /**
     * Get channel connection associated to the specified ID.
     * @param channelID ID of the channel to retrieve
     * @return Pointer to the channel connnection
     */
    std::shared_ptr<libcomp::InternalConnection> GetChannelConnectionByID(
        int8_t channelID) const;

    /**
     * Get all channels by connection.
     * @return Map of channels by connection
     */
    std::map<std::shared_ptr<libcomp::InternalConnection>,
        std::shared_ptr<objects::RegisteredChannel>> GetChannels() const;

    /**
     * Get the next channel ID to use for connecting channels.
     * @return Next channel ID, starting at 0
     */
    uint8_t GetNextChannelID() const;

    /**
     * Get the preferred channel to log into for a client in the lobby.
     * @return Pointer to the RegisteredChannel
     */
    std::shared_ptr<objects::RegisteredChannel> GetLoginChannel() const;

    /**
     * Get a pointer to the lobby connection.
     * @return Pointer to the lobby connection
     */
    const std::shared_ptr<libcomp::InternalConnection> GetLobbyConnection() const;

    /**
     * Set the RegisteredChannel of a channel currently being connected to.
     * @param channel Pointer to the RegisteredChannel.
     * @param connection Pointer to the channel's connection.
     */
    void RegisterChannel(const std::shared_ptr<objects::RegisteredChannel>& channel,
        const std::shared_ptr<libcomp::InternalConnection>& connection);

    /**
     * Remove the RegisteredChannel for a connection
     * that is no longer being used.
     * @param connection Pointer to the channel's connection.
     * @return true if the RegisteredChannel existed, false if it did not
     */
    bool RemoveChannel(const std::shared_ptr<libcomp::InternalConnection>& connection);

    /**
     * Get the world database.
     * @return Pointer to the world's database
     */
    std::shared_ptr<libcomp::Database> GetWorldDatabase() const;

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
     * Register the world with the lobby database.
     * @return true on success, false on failure
     */
    bool RegisterServer();

    /**
     * Get the account manager for the server.
     * @return Account manager for the server.
     */
    AccountManager* GetAccountManager();

    /**
     * Get the character manager for the server.
     * @return Character manager for the server.
     */
    CharacterManager* GetCharacterManager() const;

    /**
     * Get a pointer to the data sync manager.
     * @return Pointer to the WorldSyncManager
     */
    WorldSyncManager* GetWorldSyncManager() const;

    /**
     * Build the data-less relay packet from and targetting the supplied
     * world CIDs.
     * @param p Reference param containing the packet to write to
     * @param targetCIDs Optional list of target CIDs. If this is not supplied
     *  it must be inserted elsewhere.
     * @param sourceCID World CID of the source character, 0 means it came
     *  from the world
     * @return Position of the target CIDs section of the packet. This is useful
     *  for when the target CIDs are added later.
     */
    static uint32_t GetRelayPacket(libcomp::Packet& p,
        const std::list<int32_t>& targetCIDs = {}, int32_t sourceCID = 0);

    /**
     * Build the data-less relay packet from and targetting the supplied
     * world CIDs.
     * @param p Reference param containing the packet to write to
     * @param targetCID World CID of the target to send the packet to
     * @param sourceCID World CID of the source character, 0 means it came
     *  from the world
     */
    static void GetRelayPacket(libcomp::Packet& p, int32_t targetCID,
        int32_t sourceCID = 0);

protected:
    /**
     * Create a connection to a newly active socket.
     * @param socket A new socket connection.
     * @return Pointer to the newly created connection
     */
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /// A shared pointer to the world database used by the server.
    std::shared_ptr<libcomp::Database> mDatabase;

    /// A shared pointer to the world database used by the server.
    std::shared_ptr<libcomp::Database> mLobbyDatabase;

    /// Pointer to the RegisteredWorld.
    std::shared_ptr<objects::RegisteredWorld> mRegisteredWorld;

    /// Pointer to the RegisteredChannels by their connections.
    std::map<std::shared_ptr<libcomp::InternalConnection>,
        std::shared_ptr<objects::RegisteredChannel>> mRegisteredChannels;

    /// Pointer to the manager in charge of connection messages.
    std::shared_ptr<ManagerConnection> mManagerConnection;

    /// Account manager for the server.
    AccountManager* mAccountManager;

    /// Character manager for the server.
    CharacterManager* mCharacterManager;

    /// Data sync manager for the server.
    WorldSyncManager* mSyncManager;
};

} // namespace world

#endif // SERVER_WORLD_SRC_WORLDSERVER_H
