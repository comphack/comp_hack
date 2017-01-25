/**
 * @file server/lobby/src/packets/PurchaseTicket.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to handle the actual purchase of lobby tickets.
 *
 * This file is part of the Lobby Server (lobby).
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

#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// lobby Includes
#include "LobbyServer.h"

// object Includes
#include <Account.h>

using namespace lobby;

bool Parsers::PurchaseTicket::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto lobbyDB = server->GetMainDatabase();
    auto config = std::dynamic_pointer_cast<objects::LobbyConfig>(
        server->GetConfig());
    auto account = state(connection)->GetAccount();
    auto ticketCost = config->GetCharacterTicketCost();

    if(account->GetCP() >= ticketCost)
    {
        account->SetCP(static_cast<uint32_t>(account->GetCP() - ticketCost));
        account->SetTicketCount(static_cast<uint8_t>(account->GetTicketCount() + 1));

        if(!account->Update(lobbyDB))
        {
            LOG_ERROR(libcomp::String("Account purchased a character ticket but could"
                " not be updated: %1").Arg(account->GetUUID().ToString()));
        }
    }
    else
    {
        LOG_ERROR(libcomp::String("Account attempted to purchase a character ticket"
            " without having enough CP available: %1").Arg(account->GetUUID().ToString()));
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyClientPacketCode_t::PACKET_PURCHASE_TICKET_RESPONSE);

    connection->SendPacket(reply);

    return true;
}
