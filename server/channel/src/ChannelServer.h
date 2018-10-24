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
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include "WorldClock.h"

namespace libcomp
{
class DefinitionManager;
class ServerDataManager;
}

namespace objects
{
class Character;
class WorldSharedConfig;
}

namespace channel
{

typedef uint64_t ServerTime;
typedef ServerTime (*GET_SERVER_TIME)();

class AccountManager;
class ActionManager;
class AIManager;
class ChannelSyncManager;
class CharacterManager;
class ChatManager;
class EventManager;
class FusionManager;
class MatchManager;
class SkillManager;
class TokuseiManager;
class ZoneManager;

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
     * Call the Shutdown function on each worker.  This should be called
     * only before preparing to stop the application.
     */
    virtual void Shutdown();

    /**
     * This is called before Run() ends giving a derived class the chance to
     * do additional cleanup.
     */
    virtual void Cleanup();

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
     * Get the world clock time of the server. This is thread safe and checks
     * to make sure it does not calculate more than is needed.
     * @return Current world clock time
     */
    const WorldClock GetWorldClockTime();

    /**
     * Set a custom time offset for the world clock (in seconds)
     * @param offset Custom time offset (in seconds)
     */
    void SetTimeOffset(uint32_t offset);

    /**
     * Get the RegisteredChannel.
     * @return Pointer to the RegisteredChannel
     */
    const std::shared_ptr<objects::RegisteredChannel> GetRegisteredChannel();

    /**
     * Get the current channel ID from the RegisteredChannel
     * @return Current channel ID
     */
    uint8_t GetChannelID();

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
     * Get a pointer to the AI manager.
     * @return Pointer to the AIManager
     */
    AIManager* GetAIManager() const;

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
     * Get a pointer to the fusion manager.
     * @return Pointer to the FusionManager
     */
    FusionManager* GetFusionManager() const;

    /**
     * Get a pointer to the match manager.
     * @return Pointer to the MatchManager
     */
    MatchManager* GetMatchManager() const;

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
     * Get a pointer to the data sync manager.
     * @return Pointer to the ChannelSyncManager
     */
    ChannelSyncManager* GetChannelSyncManager() const;

    /**
     * Get a pointer to the tokusei manager.
     * @return Pointer to the TokuseiManager
     */
    TokuseiManager* GetTokuseiManager() const;

    /**
     * Get the world server supplied shared config settings.
     * @return Pointer to the world shared config
     */
    std::shared_ptr<objects::WorldSharedConfig> GetWorldSharedConfig() const;

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
     * Generates server game ticks.
     */
    void StartGameTick();

    /**
    * Sends an announcement to each client connected to world
    * @param client Client that sent announcement packet to channel
    * @param message Content of message that will be announced
    * @param type Type of message to send
    *  0) Red ticker message
    *  1) White ticker message
    *  2) Blue ticker message
    *  3) Purple ticker message
    *  4) COMP shop description
    * @param broadcast If true, the packet will be broadcasted to everyone in
    *  the current zone
    * @return true if the message was successfully sent, false otherwise.
    */
    bool SendSystemMessage(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        libcomp::String message, int8_t type, bool broadcast);

    /**
     * Get the system time deadline for all punitive attributes
     * which matches midnight of the next Monday. Punitive atributes
     * are used for time restricted actions such as participation in
     * the "invoke" events.
     * @return System time respresenting the punitive attribute deadline
     */
    int32_t GetPAttributeDeadline();

    /**
     * Get the default character creation object map.
     * @return Default character creation object map
     */
    PersistentObjectMap GetDefaultCharacterObjectMap() const;

    /**
     * Schedule recurring actions that continue to run until the server shuts
     * down. This does not need to run until the channel has successfully
     * registered with the world.
     */
    void ScheduleRecurringActions();

    /**
     * Register a timed event to occur when the world clock is updated to
     * pass that time
     * @param time Time to register
     * @param type Type identifier to register the time with. This allows
     *  multiple sources to register the same time and have the server
     *  clean up as needed upon removal.
     * @param remove true if the time should be removed, false if it should
     *  should be registered
     * @return true if the time was registered properly, false if a failure
     *  ocurred
     */
    bool RegisterClockEvent(WorldClockTime time, uint8_t type, bool remove);

    /**
     * Clock event handler that is called once every second to update the
     * the world time. If the registered "next time" is hit, the time sources
     * will be notified to recalculate updates.
     */
    void HandleClockEvents();

    /**
     * Update and notify all currently connected players of demon quests
     * becoming available for the next day. By default this is scheduled to
     * execute at midnight UTC.
     */
    void HandleDemonQuestReset();

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
     * Get the number of seconds until midnight of the next day. Useful
     * for scheduling timed events.
     * @return Number of seconds until midnight of the next day
     */
    uint32_t GetTimeUntilMidnight();

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

    /**
     * Recalculate the next time the world clock will fire an event on.
     * This will be stored as a system timestamp for easy comparison.
     */
    void RecalcNextWorldEventTime();

    /// Timestamp ordered map of prepared Execute messages and timestamps
    /// associated to when they should be queued following a server tick
    std::map<ServerTime,
        std::list<libcomp::Message::Execute*>> mScheduledWork;

    /// Map of world clock times to the type of event that will
    /// occur at that time. Types include:
    /// 1) Spawn activation/deactivation
    /// 2) Tokusei active timespans
    /// 3) Zone event trigger
    /// 4) Global zone event trigger
    std::map<WorldClockTime, std::set<uint8_t>> mWorldClockEvents;

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

    /// Map of default character creation state objects
    PersistentObjectMap mDefaultCharacterObjectMap;

    /// Pointer to the account manager.
    AccountManager *mAccountManager;

    /// Pointer to the action manager.
    ActionManager *mActionManager;

    /// Pointer to the AI manager.
    AIManager *mAIManager;

    /// Poiner to the character manager.
    CharacterManager *mCharacterManager;

    /// Pointer to the Chat Manager.
    ChatManager *mChatManager;

    /// Pointer to the Event Manager.
    EventManager *mEventManager;

    /// Pointer to the Fusion Manager.
    FusionManager *mFusionManager;

    /// Pointer to the Match Manager.
    MatchManager *mMatchManager;

    /// Pointer to the Skill Manager.
    SkillManager *mSkillManager;

    /// Pointer to the Zone Manager.
    ZoneManager *mZoneManager;

    /// Pointer to the Definition Manager.
    libcomp::DefinitionManager *mDefinitionManager;

    /// Pointer to the Server Data Manager.
    libcomp::ServerDataManager *mServerDataManager;

    /// Data sync manager for the server.
    ChannelSyncManager* mSyncManager;

    /// Tokusei manager for the server.
    TokuseiManager* mTokuseiManager;

    /// Server world clock
    WorldClock mWorldClock;

    /// World clock time of the last time zone events processed
    WorldClockTime mLastEventTrigger;

    /// System time representation of the next world clock
    /// event time that the clock needs to react to
    uint32_t mNextEventTime;

    /// true if the sources of each time registered should
    /// should be updated based on the last clock update
    bool mRecalcTimeDependents;

    /// Highest entity ID currently assigned
    int32_t mMaxEntityID;

    /// Highest unique object ID currently assigned
    int64_t mMaxObjectID;

    /// Thread that queues up tick messages after a delay.
    std::thread mTickThread;

    /// Server lock for shared resources
    std::mutex mLock;

    /// Server lock for server time calculation
    std::mutex mTimeLock;

    /// If the tick thread should continue running.
    volatile bool mTickRunning;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELSERVER_H
