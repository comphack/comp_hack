/**
 * @file server/channel/src/ClientState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a client connection.
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

#ifndef SERVER_CHANNEL_SRC_CLIENTSTATE_H
#define SERVER_CHANNEL_SRC_CLIENTSTATE_H

// channel Includes
#include "CharacterState.h"
#include "DemonState.h"

// objects Includes
#include <ClientStateObject.h>

namespace channel
{

typedef float ClientTime;
typedef uint64_t ServerTime;

/**
 * Contains the state of a game client currently connected to the
 * channel.
 */
class ClientState : public objects::ClientStateObject
{
public:
    /**
     * Create a new client state.
     */
    ClientState();

    /**
     * Clean up the client state.
     */
    virtual ~ClientState();

    /**
     * Get the string encoding to use for this client.
     * @return Encoding to use for strings
     */
    libcomp::Convert::Encoding_t GetClientStringEncoding();

    /**
     * Get the state of the character associated to the client.
     * @return Pointer to the CharacterState
     */
    std::shared_ptr<CharacterState> GetCharacterState();

    /**
     * Get the state of the active demon associated to the client.
     * If there is no active demon, a state will still be returned
     * but no demon will be set.
     * @return Pointer to the DemonState
     */
    std::shared_ptr<DemonState> GetDemonState();

    /**
     * Get the object ID associated a UUID associated to the client.
     * If a zero is returned, the UUID is not registered.
     * @param uuid UUID to retrieve the corresponding object ID from
     * @return Object ID associated to a UUID
     */
    int64_t GetObjectID(const libobjgen::UUID& uuid) const;

    /**
     * Get the UUID associated to an object ID associated to the client.
     * If a null UUID is returned, the object ID is not registered.
     * @param objectID Object ID to retrieve the corresponding UUID from
     * @return UUID associated to an object ID
     */
    const libobjgen::UUID GetObjectUUID(int64_t objectID) const;

    /**
     * Set the object ID associated a UUID associated to the client.
     * @param uuid UUID to set the corresponding object ID for
     * @param objectID Object ID to map to the UUID
     * @return true if the UUID was not already registered, false
     *  if it was
     */
    bool SetObjectID(const libobjgen::UUID& uuid, int64_t objectID);

    /**
     * Get the next activated ability ID from 1 to 128.
     * @return The next activated ability ID for the client
     */
    uint8_t GetNextActivatedAbilityID();

    /**
     * Check if the client state has everything needed to start
     * being used.
     * @return true if the state is ready to use, otherwise false
     */
    bool Ready();

    /**
     * Handle any actions needed when the game client pings the
     * server with a sync request.  If the start time has not been
     * set, it will be set here.
     */
    void SyncReceived();

    /**
     * Convert time relative to the server to time relative to the
     * game client.
     * @param time Time relative to the server
     * @return Time relative to the client
     */
    ClientTime ToClientTime(ServerTime time) const;

    /**
     * Convert time relative to the game client to time relative
     * to the server.
     * @param time Time relative to the client
     * @return Time relative to the server
     */
    ServerTime ToServerTime(ClientTime time) const;

private:
    /// State of the character associated to the client
    std::shared_ptr<CharacterState> mCharacterState;

    /// State of the active demon associated to the client which will
    /// be set to an empty Demon pointer when one is not summoned
    std::shared_ptr<DemonState> mDemonState;

    /// Map of UUIDs to game client object IDs
    std::unordered_map<libcomp::String, int64_t> mObjectIDs;

    /// Map of game client object IDs to UUIDs
    std::unordered_map<int64_t, libobjgen::UUID> mObjectUUIDs;

    /// Current time of the server set upon starting the client
    /// communication.
    ServerTime mStartTime;

    uint8_t mNextActivatedAbilityID;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CLIENTSTATE_H
