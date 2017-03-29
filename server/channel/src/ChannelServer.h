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
#include <RegisteredChannel.h>
#include <RegisteredWorld.h>

// channel Includes
#include "AccountManager.h"
#include "ActionManager.h"
#include "CharacterManager.h"
#include "ChatManager.h"
#include "DefinitionManager.h"
#include "ServerDataManager.h"
#include "SkillManager.h"
#include "ZoneManager.h"

namespace channel
{

typedef uint64_t ServerTime;
typedef ServerTime (*GET_SERVER_TIME)();

/**
 * Channel server that handles client packets in game.
 */
class ChannelServer : public libcomp::BaseServer
{
public:
    /**
     * Create a new channel server.
     * @param szProgram First command line argument for the application.
     * @param config Pointer to a casted ChannelConfig that will contain properties
     *   every server has in addition to channel specific ones.
     * @param configPath File path to the location of the config to be loaded.
     */
    ChannelServer(const char *szProgram, std::shared_ptr<
        objects::ServerConfig> config, const libcomp::String& configPath);

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
    virtual bool Initialize();

    /**
     * Get the current time relative to the server.
     * @return Current time relative to the server
     */
    static ServerTime GetServerTime();

    /**
     * Get the RegisteredChannel.
     * @return Pointer to the RegisteredChannel
     */
    const std::shared_ptr<objects::RegisteredChannel> GetRegisteredChannel();

    /**
     * Get the RegisteredWorld.
     * @return Pointer to the RegisteredWorld
     */
    std::shared_ptr<objects::RegisteredWorld> GetRegisteredWorld();

    /**
     * Set the RegisteredWorld.
     * @param registeredWorld Pointer to the RegisteredWorld
     */
    void RegisterWorld(const std::shared_ptr<
        objects::RegisteredWorld>& registeredWorld);

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
     * @param channelID Channel ID from the world to register with
     * @return true on success, false on failure
     */
    bool RegisterServer(uint8_t channelID);

    /**
     * Get the connection manager for the server.
     * @return Pointer to the connection manager.
     */
    std::shared_ptr<ManagerConnection> GetManagerConnection() const;

    /**
     * Get a pointer to the account manager.
     * @return Pointer to the AccountManager
     */
    AccountManager* GetAccountManager() const;

    /**
     * Get a pointer to the action manager.
     * @return Pointer to the ActionManager
     */
    ActionManager* GetActionManager() const;

    /**
     * Get a pointer to the character manager.
     * @return Pointer to the CharacterManager
     */
    CharacterManager* GetCharacterManager() const;

    /**
     * Get a pointer to the chat manager.
     * @return Pointer to the ChatManager
     */
    ChatManager* GetChatManager() const;

    /**
     * Get a pointer to the skill manager.
     * @return Pointer to the SkillManager
     */
    SkillManager* GetSkillManager() const;

    /**
     * Get a pointer to the zone manager.
     * @return Pointer to the ZoneManager
     */
    ZoneManager* GetZoneManager() const;

    /**
     * Get a pointer to the definition manager.
     * @return Pointer to the DefinitionManager
     */
    libcomp::DefinitionManager* GetDefinitionManager() const;

    /**
     * Get a pointer to the server data manager.
     * @return Pointer to the ServerDataManager
     */
    libcomp::ServerDataManager* GetServerDataManager() const;

    /**
     * Increments and returns the next available entity ID.
     * @return Next game entity ID for the channel
     */
    int32_t GetNextEntityID();

    /**
     * Increments and returns the next available object ID.
     * @return Next object ID for the channel
     */
    int64_t GetNextObjectID();

protected:
    /**
     * Create a connection to a newly active socket.
     * @param socket A new socket connection.
     * @return Pointer to the newly created connection
     */
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /**
     * Get the current time relative to the server using the
     * C++ standard steady_clock.
     * @return Current time relative to the server
     */
    static ServerTime GetServerTimeSteady();

    /**
     * Get the current time relative to the server using the
     * C++ standard high_resolution_clock.
     * @return Current time relative to the server
     */
    static ServerTime GetServerTimeHighResolution();

    /// Function pointer to the most accurate time detection code
    /// available for the current machine.
    static GET_SERVER_TIME sGetServerTime;

    /// Pointer to the manager in charge of connection messages.
    std::shared_ptr<ManagerConnection> mManagerConnection;

    /// Pointer to the RegisteredWorld.
    std::shared_ptr<objects::RegisteredWorld> mRegisteredWorld;

    /// A shared pointer to the world database used by the server.
    std::shared_ptr<libcomp::Database> mWorldDatabase;

    /// A shared pointer to the main database used by the server.
    std::shared_ptr<libcomp::Database> mLobbyDatabase;

    /// Pointer to the RegisteredChannel.
    std::shared_ptr<objects::RegisteredChannel> mRegisteredChannel;

    /// Pointer to the account manager.
    AccountManager *mAccountManager;

    /// Pointer to the action manager.
    ActionManager *mActionManager;

    /// Poiner to the character manager.
    CharacterManager *mCharacterManager;

    /// Pointer to the Chat Manager.
    ChatManager *mChatManager;

    /// Pointer to the Skill Manager.
    SkillManager *mSkillManager;

    /// Pointer to the Zone Manager.
    ZoneManager *mZoneManager;

    /// Pointer to the Definition Manager.
    libcomp::DefinitionManager *mDefinitionManager;

    /// Pointer to the Server Data Manager.
    libcomp::ServerDataManager *mServerDataManager;

    /// Highest entity ID currently assigned
    int32_t mMaxEntityID;

    /// Highest unique object ID currently assigned
    int64_t mMaxObjectID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELSERVER_H
