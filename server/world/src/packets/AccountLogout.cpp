/**
 * @file server/world/src/packets/AccountLogout.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle logging out an account.
 *
 * This file is part of the World Server (world).
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

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>

// world Includes
#include "AccountManager.h"
#include "WorldConfig.h"
#include "WorldServer.h"

using namespace world;

void Logout(std::shared_ptr<WorldServer> server,
    const libcomp::String username)
{
    auto accountManager = server->GetAccountManager();

    int8_t channelID;
    if(!accountManager->IsLoggedIn(username, channelID))
    {
        return;
    }

    LOG_DEBUG(libcomp::String("Logging out user: '%1'\n").Arg(username));
    accountManager->LogoutUser(username, channelID);

    auto lobbyConnection = server->GetLobbyConnection();

    //Relay the message  to the lobby
    libcomp::Packet lobbyMessage;
    lobbyMessage.WritePacketCode(
        InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
    lobbyMessage.WriteString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, username);
    lobbyConnection->SendPacket(lobbyMessage);
}

bool Parsers::AccountLogout::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    libcomp::String username = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());

    server->QueueWork(Logout, server, username);

    return true;
}
