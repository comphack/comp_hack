/**
 * @file server/world/src/packets/AccountLogin.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle retrieving a channel for the client to log into.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// object Includes
#include <AccountLogin.h>
#include <ChannelLogin.h>

// world Includes
#include "AccountManager.h"
#include "WorldServer.h"

using namespace world;

bool Parsers::AccountLogin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto server = std::dynamic_pointer_cast<WorldServer>(
        pPacketManager->GetServer());
    auto iConnection = std::dynamic_pointer_cast<
        libcomp::InternalConnection>(connection);

    if(connection == server->GetLobbyConnection())
    {
        // The lobby is requesting a channel to log into
        auto login = std::make_shared<objects::AccountLogin>();
        if(!login->LoadPacket(p, false))
        {
            return false;
        }

        server->QueueWork([](std::shared_ptr<WorldServer> pServer,
            std::shared_ptr<objects::AccountLogin> pLogin)
            {
                pServer->GetAccountManager()->HandleLobbyLogin(pLogin);
            }, server, login);
    }
    else
    {
        if(p.ReadU8() == 0)
        {
            // The channel is requesting session info
            libcomp::String username = p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
            uint32_t sessionKey = p.ReadU32();

            server->QueueWork([](std::shared_ptr<WorldServer> pServer,
                const std::shared_ptr<libcomp::InternalConnection>& pChannel,
                uint32_t pSessionKey, const libcomp::String pUsername)
                {
                    pServer->GetAccountManager()->HandleChannelLogin(pChannel,
                        pSessionKey, pUsername);
                }, server, iConnection, sessionKey, username);
        }
        else
        {
            // The channel is supplying requested first login info
            auto login = std::make_shared<objects::AccountLogin>();
            if(!login->LoadPacket(p, false))
            {
                return false;
            }

            auto channelLogin = std::make_shared<objects::ChannelLogin>();
            if(!channelLogin->LoadPacket(p, false))
            {
                return false;
            }

            server->QueueWork([](std::shared_ptr<WorldServer> pServer,
                std::shared_ptr<objects::AccountLogin> pLogin,
                std::shared_ptr<objects::ChannelLogin> pChannelLogin)
                {
                    pServer->GetAccountManager()->CompleteLobbyLogin(pLogin,
                        pChannelLogin);
                }, server, login, channelLogin);
        }
    }

    return true;
}
