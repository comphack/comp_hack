/**
 * @file server/channel/src/ClientState.cpp
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

#include "ClientState.h"

// Standard C++11 Includes
#include <ctime>

// libcomp Includes
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <AccountLogin.h>
#include <CharacterLogin.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

std::unordered_map<bool,
    std::unordered_map<int32_t, ClientState*>> ClientState::sEntityClients;
std::mutex ClientState::sLock;

ClientState::ClientState() : objects::ClientStateObject(),
    mCharacterState(std::shared_ptr<CharacterState>(new CharacterState)),
    mDemonState(std::shared_ptr<DemonState>(new DemonState)),
    mStartTime(0), mNextLocalObjectID(1)
{
}

ClientState::~ClientState()
{
    auto cEntityID = mCharacterState->GetEntityID();
    auto dEntityID = mDemonState->GetEntityID();
    auto worldCID = GetAccountLogin()->GetCharacterLogin()->GetWorldCID();
    if(cEntityID != 0 || dEntityID != 0)
    {
        std::lock_guard<std::mutex> lock(sLock);
        sEntityClients[false].erase(cEntityID);
        sEntityClients[false].erase(dEntityID);
        sEntityClients[true].erase(worldCID);
    }
}

libcomp::Convert::Encoding_t ClientState::GetClientStringEncoding()
{
    /// @todo: Return UTF-8 for US Client
    return libcomp::Convert::Encoding_t::ENCODING_CP932;
}

std::shared_ptr<CharacterState> ClientState::GetCharacterState()
{
    return mCharacterState;
}

std::shared_ptr<DemonState> ClientState::GetDemonState()
{
    return mDemonState;
}

std::shared_ptr<ActiveEntityState> ClientState::GetEntityState(int32_t entityID)
{
    std::list<std::shared_ptr<ActiveEntityState>> states = { mCharacterState, mDemonState };
    for(auto state : states)
    {
        if(state->GetEntityID() == entityID && state->Ready())
        {
            return state;
        }
    }

    return nullptr;
}

bool ClientState::Register()
{
    auto cEntityID = mCharacterState->GetEntityID();
    auto dEntityID = mDemonState->GetEntityID();
    auto worldCID = GetWorldCID();
    if(cEntityID == 0 || dEntityID == 0 || worldCID == 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(sLock);
    if(sEntityClients[false].find(cEntityID) != sEntityClients[false].end() ||
        sEntityClients[false].find(dEntityID) != sEntityClients[false].end())
    {
        return false;
    }

    sEntityClients[false][cEntityID] = this;
    sEntityClients[false][dEntityID] = this;
    sEntityClients[true][worldCID] = this;

    return true;
}

int64_t ClientState::GetObjectID(const libobjgen::UUID& uuid)
{
    std::lock_guard<std::mutex> lock(sLock);
    auto uuidStr = uuid.ToString();

    auto iter = mObjectIDs.find(uuidStr);
    if(iter != mObjectIDs.end())
    {
        return iter->second;
    }

    return 0;
}

const libobjgen::UUID ClientState::GetObjectUUID(int64_t objectID)
{
    std::lock_guard<std::mutex> lock(sLock);
    auto iter = mObjectUUIDs.find(objectID);
    if(iter != mObjectUUIDs.end())
    {
        return iter->second;
    }

    return NULLUUID;
}

int32_t ClientState::GetLocalObjectID(const libobjgen::UUID& uuid)
{
    std::lock_guard<std::mutex> lock(sLock);
    auto uuidStr = uuid.ToString();

    auto iter = mLocalObjectIDs.find(uuidStr);
    if(iter != mLocalObjectIDs.end())
    {
        return iter->second;
    }

    int32_t localID = mNextLocalObjectID++;
    mLocalObjectIDs[uuidStr] = localID;
    mLocalObjectUUIDs[localID] = uuid;

    return localID;
}

const libobjgen::UUID ClientState::GetLocalObjectUUID(int32_t objectID)
{
    std::lock_guard<std::mutex> lock(sLock);
    auto iter = mLocalObjectUUIDs.find(objectID);
    if(iter != mLocalObjectUUIDs.end())
    {
        return iter->second;
    }

    return NULLUUID;
}

bool ClientState::SetObjectID(const libobjgen::UUID& uuid, int64_t objectID)
{
    auto uuidStr = uuid.ToString();

    auto iter = mObjectIDs.find(uuidStr);
    if(iter == mObjectIDs.end())
    {
        mObjectIDs[uuidStr] = objectID;
        mObjectUUIDs[objectID] = uuid;
        return true;
    }

    return false;
}

const libobjgen::UUID ClientState::GetAccountUID() const
{
    return mCharacterState->Ready() ?
        mCharacterState->GetEntity()->GetAccount().GetUUID() : NULLUUID;
}

int32_t ClientState::GetWorldCID() const
{
    return GetAccountLogin()->GetCharacterLogin()->GetWorldCID();
}

uint32_t ClientState::GetPartyID() const
{
    return GetAccountLogin()->GetCharacterLogin()->GetPartyID();
}

int32_t ClientState::GetClanID() const
{
    return GetAccountLogin()->GetCharacterLogin()->GetClanID();
}

std::shared_ptr<objects::PartyCharacter> ClientState::GetPartyCharacter(
    bool includeDemon) const
{
    auto character = mCharacterState->GetEntity();
    auto cStats = character->GetCoreStats();

    auto member = std::make_shared<objects::PartyCharacter>();
    member->SetWorldCID(GetAccountLogin()->GetCharacterLogin()->GetWorldCID());
    member->SetName(character->GetName());
    member->SetLevel((uint8_t)cStats->GetLevel());
    member->SetHP((uint16_t)cStats->GetHP());
    member->SetMaxHP((uint16_t)mCharacterState->GetMaxHP());
    member->SetMP((uint16_t)cStats->GetMP());
    member->SetMaxMP((uint16_t)mCharacterState->GetMaxMP());
    if(includeDemon)
    {
        member->SetDemon(GetPartyDemon());
    }
    else
    {
        member->SetDemon(nullptr);
    }

    return member;
}

std::shared_ptr<objects::PartyMember> ClientState::GetPartyDemon() const
{
    auto demon = mDemonState->GetEntity();

    auto dMember = std::make_shared<objects::PartyMember>();
    if(demon)
    {
        auto dStats = demon->GetCoreStats();
        dMember->SetDemonType(demon->GetType());
        dMember->SetHP((uint16_t)dStats->GetHP());
        dMember->SetMaxHP((uint16_t)mDemonState->GetMaxHP());
        dMember->SetMP((uint16_t)dStats->GetMP());
        dMember->SetMaxMP((uint16_t)mDemonState->GetMaxMP());
    }
    return dMember;
}

void ClientState::GetPartyCharacterPacket(libcomp::Packet& p) const
{
    p.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
    p.WriteS32Little(GetAccountLogin()->GetCharacterLogin()->GetWorldCID());
    p.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO);
    GetPartyCharacter(false)->SavePacket(p, true);
}

void ClientState::GetPartyDemonPacket(libcomp::Packet& p) const
{
    p.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
    p.WriteS32Little(GetAccountLogin()->GetCharacterLogin()->GetWorldCID());
    p.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO);
    GetPartyDemon()->SavePacket(p, true);
}

void ClientState::SyncReceived()
{
    if(mStartTime == 0)
    {
        mStartTime = ChannelServer::GetServerTime();
    }
}

ClientTime ClientState::ToClientTime(ServerTime time) const
{
    if(time <= mStartTime)
    {
        return 0.0f;
    }

    return static_cast<ClientTime>((ClientTime)(time - mStartTime) / 1000000.0f);
}

ServerTime ClientState::ToServerTime(ClientTime time) const
{
    return static_cast<ServerTime>(((ServerTime)time * 1000000) + mStartTime);
}

ClientState* ClientState::GetEntityClientState(int32_t id, bool worldID)
{
    std::lock_guard<std::mutex> lock(sLock);
    std::unordered_map<int32_t, ClientState*>& m = sEntityClients[worldID];
    auto it = m.find(id);
    return it != m.end() ? it->second : nullptr;
}
