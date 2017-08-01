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
#include "ActiveEntityState.h"

// objects Includes
#include <Character.h>
#include <ClientStateObject.h>
#include <Demon.h>
#include <PartyCharacter.h>

namespace libcomp
{
class Packet;
}

namespace channel
{

typedef float ClientTime;
typedef uint64_t ServerTime;
typedef ActiveEntityStateImp<objects::Character> CharacterState;
typedef ActiveEntityStateImp<objects::Demon> DemonState;

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
     * Clean up the client state, removing it from the registry
     * if it exists there.
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
     * Get the entity state associated to an entity ID for this client.
     * @param entityID Entity ID associated to this client to retrieve
     * @return Pointer to the matching entity state, null if no match
     *  exists
     */
    std::shared_ptr<ActiveEntityState> GetEntityState(int32_t entityID);

    /**
     * Registers the client state with the static entity map for access by
     * other clients.
     * @return true if the state was registered properly, otherwise false
     */
    bool Register();

    /**
     * Get the object ID associated to a UUID associated to the client.
     * If a zero is returned, the UUID is not registered.
     * @param uuid UUID to retrieve the corresponding object ID from
     * @return Object ID associated to a UUID
     */
    int64_t GetObjectID(const libobjgen::UUID& uuid);

    /**
     * Get the UUID associated to an object ID associated to the client.
     * If a null UUID is returned, the object ID is not registered.
     * @param objectID Object ID to retrieve the corresponding UUID from
     * @return UUID associated to an object ID
     */
    const libobjgen::UUID GetObjectUUID(int64_t objectID);

    /**
     * Get the local object ID associated to a UUID associated to the
     * client. If a zero is returned, the UUID is not registered.
     * @param uuid UUID to retrieve the corresponding object ID from
     * @return Local object ID associated to a UUID
     */
    int32_t GetLocalObjectID(const libobjgen::UUID& uuid);

    /**
     * Get the UUID associated to an object ID associated to the client.
     * If a null UUID is returned, the object ID is not registered.
     * @param objectID Object ID to retrieve the corresponding UUID from
     * @return UUID associated to an object ID contextual to the client
     */
    const libobjgen::UUID GetLocalObjectUUID(int32_t objectID);

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
     * Get the UID of the account associated to the client.
     * @return UID of the account associated to the client
     */
    const libobjgen::UUID GetAccountUID() const;

    /**
     * Get the current world CID associated to the logged in
     * character.
     * @return Current character's world CID
     */
    int32_t GetWorldCID() const;

    /**
     * Get the current party ID associated to the logged in
     * character.
     * @return Current character's party ID
     */
    uint32_t GetPartyID() const;

    /**
     * Get the current clan ID associated to the logged in
     * character.
     * @return Current character's clan ID
     */
    int32_t GetClanID() const;

    /**
     * Get a current party character representation from the
     *.associated CharacterState.
     * @param includeDemon true if the demon state is set
     *  on the member variable of the party character
     * @return Pointer to a party character representation
     */
    std::shared_ptr<objects::PartyCharacter> GetPartyCharacter(
        bool includeDemon) const;

    /**
     * Get a current party demon representation from the
     * associated DemonState.
     * @return Pointer to a party demon representation
     */
    std::shared_ptr<objects::PartyMember> GetPartyDemon() const;

    /**
     * Populate the supplied packet with an internal CharacterLogin
     * packet containing character information.
     * @param p Packet to populate
     */
    void GetPartyCharacterPacket(libcomp::Packet& p) const;

    /**
     * Populate the supplied packet with an internal CharacterLogin
     * packet containing partner demon information.
     * @param p Packet to populate
     */
    void GetPartyDemonPacket(libcomp::Packet& p) const;

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

    /**
     * Get the client state associated to the supplied entity ID.
     * @param id Entity ID or world ID associated to the client
     *  state to retrieve
     * @param worldID true if the ID is from the world, false if it is a
     *  local entity ID
     * @return Pointer to the client state associated to the ID or
     *  nullptr if it does not exist
     */
    static ClientState* GetEntityClientState(int32_t id,
        bool worldID = false);

private:
    /// Static registry of all client states sorted as world (true) or
    /// local entity IDs (false) and their respective IDs
    static std::unordered_map<bool,
        std::unordered_map<int32_t, ClientState*>> sEntityClients;

    /// Static lock for shared static resources
    static std::mutex sLock;

    /// State of the character associated to the client
    std::shared_ptr<CharacterState> mCharacterState;

    /// State of the active demon associated to the client which will
    /// be set to an empty Demon pointer when one is not summoned
    std::shared_ptr<DemonState> mDemonState;

    /// Map of UUIDs to game client object IDs
    std::unordered_map<libcomp::String, int64_t> mObjectIDs;

    /// Map of game client object IDs to UUIDs
    std::unordered_map<int64_t, libobjgen::UUID> mObjectUUIDs;

    /// Map of UUIDs to game client object IDs
    /// The IDs listed here are only relevant to this client
    std::unordered_map<libcomp::String, int32_t> mLocalObjectIDs;

    /// Map of game client object IDs to UUIDs
    /// The IDs listed here are only relevant to this client
    std::unordered_map<int32_t, libobjgen::UUID> mLocalObjectUUIDs;

    /// Current time of the server set upon starting the client
    /// communication.
    ServerTime mStartTime;

    /// Next available activated ability ID
    uint8_t mNextActivatedAbilityID;

    /// Next available local object ID
    int32_t mNextLocalObjectID;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CLIENTSTATE_H
