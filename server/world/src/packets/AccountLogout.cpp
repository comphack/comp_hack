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
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include <ChannelLogin.h>
#include <Character.h>

// world Includes
#include "AccountManager.h"
#include "CharacterManager.h"
#include "WorldConfig.h"
#include "WorldServer.h"

using namespace world;

bool Parsers::AccountLogout::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    LogoutPacketAction_t action = (LogoutPacketAction_t)p.ReadU32Little();
    libcomp::String username = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
    auto accountManager = server->GetAccountManager();
    
    int8_t channelID;
    if(!accountManager->IsLoggedIn(username, channelID))
    {
        // Nothing to do
        return true;
    }

    auto login = accountManager->GetUserLogin(username);
    auto cLogin = login->GetCharacterLogin();
    if(action == LogoutPacketAction_t::LOGOUT_CHANNEL_SWITCH)
    {
        auto channelLogin = std::make_shared<objects::ChannelLogin>();
        if(!channelLogin->LoadPacket(p))
        {
            LOG_ERROR("Failed to load channel switch info from channel\n");
            return false;
        }

        if(accountManager->SwitchChannel(login, channelLogin))
        {
            libcomp::Packet reply;
            reply.WritePacketCode(
                InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
            reply.WriteS32Little(cLogin->GetWorldCID());
            reply.WriteU32Little(
                (uint32_t)LogoutPacketAction_t::LOGOUT_CHANNEL_SWITCH);
            reply.WriteS8(channelLogin->GetToChannel());
            reply.WriteU32Little(login->GetSessionKey());

            connection->SendPacket(reply);
        }
        else
        {
            server->GetCharacterManager()->RequestChannelDisconnect(
                cLogin->GetWorldCID());
        }
    }
    else if(p.Left() > 0 && p.PeekU8() == 1)
    {
        // Tell the channel to disconnect
        libcomp::Packet reply;
        reply.WritePacketCode(
            InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
        reply.WriteS32Little(cLogin->GetWorldCID());
        reply.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);

        connection->SendPacket(reply);
    }
    else
    {
        if(accountManager->ChannelSwitchPending(username, channelID))
        {
            LOG_DEBUG(libcomp::String("User is switching to channel %1: '%2'\n")
                .Arg(channelID).Arg(username));

            // Tell the lobby a channel switch is happening
            libcomp::Packet lobbyMessage;
            lobbyMessage.WritePacketCode(
                InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
            lobbyMessage.WriteString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, username);
            lobbyMessage.WriteU32Little(
                (uint32_t)LogoutPacketAction_t::LOGOUT_CHANNEL_SWITCH);

            // Make sure the lobby has the new session key and channel
            lobbyMessage.WriteS8(channelID);
            lobbyMessage.WriteU32Little(login->GetSessionKey());

            server->GetLobbyConnection()->SendPacket(lobbyMessage);
        }
        else
        {
            accountManager->LogoutUser(username, channelID);
        }
    }

    return true;
}
