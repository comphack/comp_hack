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
#include "EventManager.h"
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
     */
    ChannelServer(const char *szProgram,
        std::shared_ptr<objects::ServerConfig> config,
        std::shared_ptr<libcomp::ServerCommandLineParser> commandLine);

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
     * Get the amount of time left in an expiration relative to the server,
     * in seconds.
     * @return Time until expiration relative to the server, in seconds
     */
    static int32_t GetExpirationInSeconds(uint32_t fixedTime,
        uint32_t relativeTo = 0);

    /**
     * Get the world clock time of the server.
     * @param phase Output param, world clock phase of the server
     * @param hour Output param, world clock hours of the server
     * @param min Output param, world clock minutes of the server
     */
    void GetWorldClockTime(int8_t& phase, int8_t& hour, int8_t& min);

    /**
     * Get the RegisteredChannel.
     * @return Pointer to the RegisteredChannel
     */
    const std::shared_ptr<objects::RegisteredChannel> GetRegisteredChannel();

    /**
     * Get all channels registerd on the channel's world (including
     * itself).
     * @return List of pointers to all of the world's registered channels
     */
    const std::list<std::shared_ptr<objects::RegisteredChannel>>
        GetAllRegisteredChannels();

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
     * Load all of the channel's connected world's RegisteredChannel
     * entries in the database.  This allows other channels to be seen
     * by the current channel for listing existing channels to the client.
     */
    void LoadAllRegisteredChannels();

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
     * Get a pointer to the event manager.
     * @return Pointer to the EventManager
     */
    EventManager* GetEventManager() const;

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

    /**
     * Simulate a server tick, handling events like updating
     * the server time and zone states as well as ansynchronously
     * saving data.
     */
    void Tick();

    /**
    * Sends an announcement to each client connected to world
    * Color can be chosen out of four colors:
    * @param client, client that sent announcement packet to channel
    * @param message, content of message that will be announced
    * @param color, color of the ticker message, red, white, blue, purple
    * @param bool, if true, the packet will be broadcasted to everyone in the current zone
    * @return True if the message was successfully sent, false otherwise.
    */
    bool SendSystemMessage(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        libcomp::String message, int8_t color, bool);

    /**
     * Schedule code work to be queued by the next server tick that occurs
     * following the specified time.
     * @param timestamp ServerTime timestamp that needs to pass for the
     *  specified work to be processed
     * @param f Function (lambda) to execute
     * @param args Arguments to pass to the function when it is executed
     * @return true on success, false on failure
     */
    template<typename Function, typename... Args>
    bool ScheduleWork(ServerTime timestamp, Function&& f, Args&&... args)
    {
        auto msg = new libcomp::Message::ExecuteImpl<Args...>(
            std::forward<Function>(f), std::forward<Args>(args)...);

        std::lock_guard<std::mutex> lock(mLock);
        mScheduledWork[timestamp].push_back(msg);

        return true;
    }

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

    /**
     * Queues up a time based event to insert a system message
     * that triggers a server tick.
     */
    void QueueNextTick();

    /// Function pointer to the most accurate time detection code
    /// available for the current machine.
    static GET_SERVER_TIME sGetServerTime;

    /// Timestamp ordered map of prepared Execute messages and timestamps
    /// associated to when they should be queued following a server tick
    std::map<ServerTime,
        std::list<libcomp::Message::Execute*>> mScheduledWork;

    /// Pointer to the manager in charge of connection messages.
    std::shared_ptr<ManagerConnection> mManagerConnection;

    /// Pointer to the RegisteredWorld.
    std::shared_ptr<objects::RegisteredWorld> mRegisteredWorld;

    /// A shared pointer to the world database used by the server.
    std::shared_ptr<libcomp::Database> mWorldDatabase;

    /// A shared pointer to the main database used by the server.
    std::shared_ptr<libcomp::Database> mLobbyDatabase;

    /// Pointer to the RegisteredChannel for this server.
    std::shared_ptr<objects::RegisteredChannel> mRegisteredChannel;

    /// List of pointers to all RegisteredChannels for the world.
    std::list<std::shared_ptr<objects::RegisteredChannel>> mAllRegisteredChannels;

    /// Pointer to the account manager.
    AccountManager *mAccountManager;

    /// Pointer to the action manager.
    ActionManager *mActionManager;

    /// Poiner to the character manager.
    CharacterManager *mCharacterManager;

    /// Pointer to the Chat Manager.
    ChatManager *mChatManager;

    /// Pointer to the Event Manager.
    EventManager *mEventManager;

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

    /// Thread that queues up tick messages after a delay.
    std::thread mTickThread;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELSERVER_H
