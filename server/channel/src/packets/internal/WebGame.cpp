/**
 * @file server/channel/src/packets/internal/WebGame.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle web-game notifications from the world.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <WebGameSession.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "EventManager.h"

using namespace channel;

bool Parsers::WebGame::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(p.Size() < 5)
    {
        return false;
    }

    uint8_t mode = p.ReadU8();
    int32_t worldCID = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = server->GetManagerConnection()->GetEntityClient(worldCID,
        true);
    if(!client)
    {
        // Client is no longer connected, quit now
        return true;
    }

    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_ADD:
        {
            if(p.Left() < 2 ||
                (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
            {
                return false;
            }

            auto sessionID = p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

            server->GetEventManager()->StartWebGame(client, sessionID);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_REMOVE:
        server->GetEventManager()->EndWebGame(client, false);
        break;
    default:
        break;
    }

    return true;
}
