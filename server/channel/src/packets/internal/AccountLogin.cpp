/**
 * @file server/channel/src/packets/internal/AccountLogin.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle retrieving a channel for the client to log into.
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
#include <ChannelLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "ManagerConnection.h"
#include "ZoneManager.h"

using namespace channel;

void HandleLoginResponse(AccountManager* accountManager,
    const std::shared_ptr<channel::ChannelClientConnection> client)
{
    accountManager->HandleLoginResponse(client);
}

bool Parsers::AccountLogin::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(0 == p.Left())
    {
        LOG_ERROR("Invalid response received for AccountLogin.\n");
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());

    int8_t responseCode = p.ReadS8();
    if(responseCode == 1)
    {
        // No error
        objects::AccountLogin response;
        if(!response.LoadPacket(p, false))
        {
            LOG_ERROR("Invalid response received for AccountLogin.\n");
            return false;
        }

        std::shared_ptr<objects::ChannelLogin> channelLogin;
        if(p.ReadU8() == 1)
        {
            // Channel login also exists
            channelLogin = std::make_shared<objects::ChannelLogin>();
            if(!channelLogin->LoadPacket(p, false))
            {
                LOG_ERROR("Invalid ChannelLogin response received for"
                    " AccountLogin.\n");
                return false;
            }
        }

        auto worldDB = server->GetWorldDatabase();

        // This user should already be cached since its the same one we
        // passed in
        auto account = response.GetAccount().Get(server->GetLobbyDatabase());
        if(nullptr == account)
        {
            LOG_ERROR("Unknown account returned from AccountLogin"
                " response.\n");
            return true;
        }

        // Reload the character just in case
        auto character = response.GetCharacterLogin() != nullptr
            ? response.GetCharacterLogin()->GetCharacter().Get(worldDB, true)
            : nullptr;
        if(nullptr == character)
        {
            LOG_ERROR("Invalid character returned from AccountLogin"
                " response.\n");
            return true;
        }

        auto username = account->GetUsername();
        auto client = server->GetManagerConnection()
            ->GetClientConnection(username);
        if(nullptr == client)
        {
            // Already disconnected, nevermind
            return true;
        }

        auto login = client->GetClientState()->GetAccountLogin();
        login->SetSessionID(response.GetSessionID());
        login->SetCharacterLogin(response.GetCharacterLogin());

        // Respond to this in the handler
        client->GetClientState()->SetChannelLogin(channelLogin);

        server->QueueWork(HandleLoginResponse, server->GetAccountManager(),
            client);
    }
    else if(responseCode == 2)
    {
        // World is requesting information about which channel to log
        // a player into from the lobby
        auto accountLogin = std::make_shared<objects::AccountLogin>();
        if(!accountLogin->LoadPacket(p, false))
        {
            // Nothing we could send the world back would make sense so
            // let it time out whatever it thinks its doing
            return true;
        }

        auto charLogin = accountLogin->GetCharacterLogin();

        auto channelLogin = std::make_shared<objects::ChannelLogin>();
        channelLogin->SetWorldCID(charLogin->GetWorldCID());

        auto character = charLogin->GetCharacter().Get(server
            ->GetWorldDatabase(), true);
        if(character)
        {
            uint32_t zoneID = 0, dynamicMapID = 0;
            int8_t channelID = -1;
            float x = 0.f, y = 0.f, rot = 0.f;
            if(server->GetZoneManager()->GetLoginZone(character, zoneID,
                dynamicMapID, channelID, x, y, rot))
            {
                channelLogin->SetToZoneID(zoneID);
                channelLogin->SetToDynamicMapID(dynamicMapID);
                channelLogin->SetToChannel(channelID);
            }
        }

        // Send the info back to the world
        libcomp::Packet reply;
        reply.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);
        reply.WriteU8(1); // Information response
        accountLogin->SavePacket(reply, false);
        channelLogin->SavePacket(reply, false);

        server->GetManagerConnection()->GetWorldConnection()
            ->SendPacket(reply);
    }
    else
    {
        // Failure, disconnect the client if they're here
        auto username = p.ReadString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
        auto client = server->GetManagerConnection()
            ->GetClientConnection(username);
        if(nullptr != client)
        {
            client->Close();
        }
    }

    return true;
}
