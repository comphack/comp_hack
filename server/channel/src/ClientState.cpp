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

// channel Includes
#include "ChannelServer.h"

using namespace channel;

ClientState::ClientState() : objects::ClientStateObject(),
    mCharacterState(std::shared_ptr<CharacterState>(new CharacterState)),
    mDemonState(std::shared_ptr<DemonState>(new DemonState)),
    mStartTime(0), mNextActivatedAbilityID(1)
{
}

libcomp::Convert::Encoding_t ClientState::GetClientStringEncoding()
{
    /// @todo: Return UTF-8 for US Client
    return libcomp::Convert::Encoding_t::ENCODING_CP932;
}

ClientState::~ClientState()
{
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

int64_t ClientState::GetObjectID(const libobjgen::UUID& uuid) const
{
    auto uuidStr = uuid.ToString();

    auto iter = mObjectIDs.find(uuidStr);
    if(iter != mObjectIDs.end())
    {
        return iter->second;
    }

    return 0;
}

const libobjgen::UUID ClientState::GetObjectUUID(int64_t objectID) const
{
    auto iter = mObjectUUIDs.find(objectID);
    if(iter != mObjectUUIDs.end())
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

uint8_t ClientState::GetNextActivatedAbilityID()
{
    std::lock_guard<std::mutex> lock(mLock);
    uint8_t next = mNextActivatedAbilityID;

    mNextActivatedAbilityID = (uint8_t)((mNextActivatedAbilityID + 1) % 128);
    if(mNextActivatedAbilityID == 0)
    {
        mNextActivatedAbilityID = 1;
    }

    return next;
}

const libobjgen::UUID ClientState::GetAccountUID() const
{
    return mCharacterState->Ready() ?
        mCharacterState->GetEntity()->GetAccount().GetUUID() : NULLUUID;
}

bool ClientState::Ready()
{
    return GetAuthenticated() && mCharacterState->Ready();
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
