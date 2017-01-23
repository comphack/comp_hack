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
    mStartTime(0)
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

    return static_cast<ClientTime>((time - mStartTime) / 1000000.0f);
}

ServerTime ClientState::ToServerTime(ClientTime time) const
{
    return static_cast<ServerTime>(((ServerTime)time * 1000000.0f) + mStartTime);
}
