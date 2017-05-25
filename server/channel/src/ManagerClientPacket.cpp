/**
 * @file server/channel/src/ManagerClientPacket.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager to handle channel packet logic.
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

#include "ManagerClientPacket.h"

// channel Includes
#include <ChannelClientConnection.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

using namespace channel;

ManagerClientPacket::ManagerClientPacket(std::weak_ptr<libcomp::BaseServer> server)
    : libcomp::ManagerPacket(server)
{
}

ManagerClientPacket::~ManagerClientPacket()
{
}

bool ManagerClientPacket::ValidateConnectionState(const std::shared_ptr<
    libcomp::TcpConnection>& connection, libcomp::CommandCode_t commandCode) const
{
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    bool valid = false;
    switch((ClientToChannelPacketCode_t)commandCode)
    {
        case ClientToChannelPacketCode_t::PACKET_LOGIN:
        case ClientToChannelPacketCode_t::PACKET_KEEP_ALIVE:
            valid = true;
            break;
        case ClientToChannelPacketCode_t::PACKET_AUTH:
            if(!(valid = state->GetLoggedIn()))
            {
                LOG_ERROR("Client connection attempted to authenticate without logging in.\n");
            }
            break;
        default:
            if(!(valid = state->GetAuthenticated() && state->GetLoggedIn()))
            {
                LOG_ERROR("Client connection attempted to handle a request packet"
                    " without authenticating and logging in first.\n");
            }
            break;
    }

    return valid;
}
