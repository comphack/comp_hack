/**
 * @file server/lobby/src/ManagerPacketClient.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Manager to handle client lobby packets.
 *
 * This file is part of the lobby Server (lobby).
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

#include "ManagerPacketClient.h"

// libcomp Includes
#include <PacketCodes.h>

// lobby Includes
#include "PacketParser.h"
#include "Packets.h"

using namespace lobby;

ManagerPacketClient::ManagerPacketClient(const std::shared_ptr<libcomp::BaseServer>& server)
    : ManagerPacket(server)
{
    mPacketParsers[PACKET_LOGIN] = std::shared_ptr<PacketParser>(
        new Parsers::Login());
    mPacketParsers[PACKET_AUTH] = std::shared_ptr<PacketParser>(
        new Parsers::Auth());
    mPacketParsers[PACKET_START_GAME] = std::shared_ptr<PacketParser>(
        new Parsers::StartGame());
    mPacketParsers[PACKET_CHARACTER_LIST] = std::shared_ptr<PacketParser>(
        new Parsers::CharacterList());
    mPacketParsers[PACKET_WORLD_LIST] = std::shared_ptr<PacketParser>(
        new Parsers::WorldList());
    mPacketParsers[PACKET_CREATE_CHARACTER] = std::shared_ptr<PacketParser>(
        new Parsers::CreateCharacter());
    mPacketParsers[PACKET_DELETE_CHARACTER] = std::shared_ptr<PacketParser>(
        new Parsers::DeleteCharacter());
    mPacketParsers[PACKET_QUERY_PURCHASE_TICKET] = std::shared_ptr<PacketParser>(
        new Parsers::QueryPurchaseTicket());
    mPacketParsers[PACKET_PURCHASE_TICKET] = std::shared_ptr<PacketParser>(
        new Parsers::PurchaseTicket());
}

ManagerPacketClient::~ManagerPacketClient()
{
}