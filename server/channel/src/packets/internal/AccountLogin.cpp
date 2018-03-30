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
#include <Character.h>
#include <CharacterLogin.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "ManagerConnection.h"

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

    int8_t errorCode = p.ReadS8();
    if(errorCode == 1)
    {
        // No error
        objects::AccountLogin response;
        if(!response.LoadPacket(p, false))
        {
            LOG_ERROR("Invalid response received for AccountLogin.\n");
            return false;
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

        // The character should be cached as well
        auto character = response.GetCharacterLogin() != nullptr
            ? response.GetCharacterLogin()->GetCharacter().Get(worldDB)
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

        server->QueueWork(HandleLoginResponse, server->GetAccountManager(),
            client);
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
