/**
 * @file server/channel/src/packets/internal/AccountLogout.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle a logout request sent from the world.
 *
 * This file is part of the Channel Server (channel).
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
#include <Character.h>
#include <CharacterLogin.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::AccountLogout::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    int32_t cid = p.ReadS32Little();
    LogoutPacketAction_t action = (LogoutPacketAction_t)p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = server->GetManagerConnection()->GetEntityClient(cid, true);
    if(client == nullptr)
    {
        // Not logged in, stop here
        return true;
    }

    // Manually perform the logout then respond to the client
    server->GetAccountManager()->Logout(client, true);
    client->GetClientState()->SetLogoutSave(false);

    bool normalDisconnect = true;
    if(action == LogoutPacketAction_t::LOGOUT_CHANNEL_SWITCH)
    {
        int8_t channelID = p.ReadS8();
        uint32_t sessionKey = p.ReadU32Little();

        std::shared_ptr<objects::RegisteredChannel> channel;
        for(auto c : server->GetAllRegisteredChannels())
        {
            if(c->GetID() == channelID)
            {
                channel = c;
                break;
            }
        }

        if(channel)
        {
            libcomp::Packet request;
            request.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_LOGOUT);
            request.WriteU32Little(
                (uint32_t)LogoutPacketAction_t::LOGOUT_CHANNEL_SWITCH);
            request.WriteU32Little(sessionKey);
            request.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                libcomp::String("%1:%2").Arg(channel->GetIP()).Arg(
                    channel->GetPort()), true);
            client->SendPacket(request);

            normalDisconnect = false;
        }
    }

    if(normalDisconnect)
    {
        server->GetAccountManager()->RequestDisconnect(client);
    }

    return true;
}
