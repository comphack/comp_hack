/**
 * @file server/lobby/src/packets/AccountLogin.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Parser to handle the response for retrieving a channel for the
 * client to log into.
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
#include <Decrypt.h>
#include <InternalConnection.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>

// lobby Includes
#include "LobbyServer.h"

using namespace lobby;

void UpdateAccountLogin(std::shared_ptr<LobbyServer> server,
    const std::shared_ptr<objects::AccountLogin> login)
{
    auto cid = login->GetCID();
    auto worldID = login->GetWorldID();
    auto channelID = login->GetChannelID();
    if(0 > worldID || 0 > channelID)
    {
        LOG_ERROR("Invalid channel or world ID received for AccountLogin.\n");
        return;
    }

    auto world = server->GetWorldByID((uint8_t)worldID);
    if(nullptr == world)
    {
        return;
    }

    //Should be the same one we passed in
    auto account = login->GetAccount().Get(world->GetWorldDatabase());
    if(nullptr == account)
    {
        return;
    }

    auto channel = world->GetChannelByID((uint8_t)channelID);
    if(nullptr == account || nullptr == channel)
    {
        LOG_ERROR("Unknown channel ID returned from the world.\n");
        return;
    }

    auto username = account->GetUsername();
    auto accountManager = server->GetAccountManager();

    int8_t currentWorldID;
    if(!accountManager->IsLoggedIn(username, currentWorldID))
    {
        return;
    }

    auto clientConnection = server->GetManagerConnection()->GetClientConnection(
        account->GetUsername());
    if(nullptr != clientConnection && currentWorldID == -1)
    {
        LOG_DEBUG(libcomp::String("Login character with ID %1 into world %2, channel %3\n"
            ).Arg(cid).Arg(worldID).Arg(channelID));

        libcomp::Packet reply;
        reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_START_GAME);

        // Some session key.
        reply.WriteU32Little(login->GetSessionKey());

        // Server address.
        reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            libcomp::String("%1:%2").Arg(channel->GetIP()).Arg(channel->GetPort()), true);

        // Character ID.
        reply.WriteU8(cid);

        clientConnection->SendPacket(reply);
    }
    
    //Always refresh the connection
    if(!accountManager->LogoutUser(username, currentWorldID))
    {
        return;
    }
    accountManager->LoginUser(account->GetUsername(), login);
}

bool Parsers::AccountLogin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    auto response = std::shared_ptr<objects::AccountLogin>(new objects::AccountLogin);
    if(!response->LoadPacket(p))
    {
        LOG_ERROR("Invalid response received for AccountLogin.\n");
        return false;
    }

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    server->QueueWork(UpdateAccountLogin, server, response);

    return true;
}
