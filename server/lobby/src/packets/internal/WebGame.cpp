/**
 * @file server/lobby/src/packets/internal/WebGame.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Parser to handle web-game notifications from the world.
 *
 * This file is part of the Lobby Server (lobby).
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

// lobby Includes
#include "AccountManager.h"
#include "LobbyServer.h"
#include "ManagerConnection.h"

using namespace lobby;

bool Parsers::WebGame::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 3)
    {
        return false;
    }

    uint8_t mode = p.ReadU8();
    
    if(p.Left() < (uint32_t)(2 + p.PeekU16Little()))
    {
        return false;
    }

    auto username = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

    auto server = std::dynamic_pointer_cast<LobbyServer>(
        pPacketManager->GetServer());
    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_ADD:
        // Starting a new web-game session
        {
            auto gameSession = std::make_shared<objects::WebGameSession>();
            if(!gameSession->LoadPacket(p, false))
            {
                return false;
            }

            libcomp::Packet reply;
            reply.WritePacketCode(InternalPacketCode_t::PACKET_WEB_GAME);

            if(server->GetAccountManager()->StartWebGameSession(username,
                gameSession))
            {
                // Notify the world that the session is ready to be used
                reply.WriteU8((uint8_t)
                    InternalPacketAction_t::PACKET_ACTION_ADD);
                reply.WriteString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_UTF8, username, true);
                reply.WriteString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_UTF8,
                    gameSession->GetSessionID(), true);
            }
            else
            {
                // Failed to add, request cancellation from world
                reply.WriteU8((uint8_t)
                    InternalPacketAction_t::PACKET_ACTION_REMOVE);
                reply.WriteString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_UTF8, username, true);
            }

            connection->SendPacket(reply);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_REMOVE:
        // Nothing special, just remove session
        server->GetAccountManager()->EndWebGameSession(username);
        break;
    default:
        break;
    }

    return true;
}
