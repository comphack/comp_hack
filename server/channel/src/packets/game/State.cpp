/**
 * @file server/channel/src/packets/game/State.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to retrieve the player's initial login
 *  state information.
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
#include <ManagerPacket.h>
#include <PacketCodes.h>

// object Includes
#include <AccountLogin.h>
#include <CharacterLogin.h>
#include <ChannelConfig.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChatManager.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ManagerConnection.h"
#include "TokuseiManager.h"

using namespace channel;

void SendStateData(std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto cLogin = state->GetAccountLogin()->GetCharacterLogin();
    auto characterManager = server->GetCharacterManager();
    auto tokuseiManager = server->GetTokuseiManager();

    characterManager->SendCharacterData(client);

    characterManager->SendAutoRecovery(client);

    characterManager->SetStatusIcon(client);

    tokuseiManager->SendCostAdjustments(cState->GetEntityID(),
        client);

    // If we're already in a party, send party member info to rejoin
    // the existing one if possible
    if(cLogin->GetPartyID())
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
        request.WriteS32Little(cLogin->GetWorldCID());

        request.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO
            | (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO);

        auto member = state->GetPartyCharacter(false);
        member->SavePacket(request, true);

        auto partyDemon = state->GetPartyDemon();
        partyDemon->SavePacket(request, true);

        server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
    }

    // Greet the player.
    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(server->GetConfig());
    auto worldSharedConfig = conf->GetWorldSharedConfig();
    auto chatManager = server->GetChatManager();
    auto greetMsg = worldSharedConfig->GetGreetMessage();

    if(!greetMsg.IsEmpty())
    {
        chatManager->SendChatMessage(client,
            ChatType_t::CHAT_SELF, greetMsg);
    }

    chatManager->SendChatMessage(client, ChatType_t::CHAT_SELF,
        "Type @version or @license for more information.");
}

bool Parsers::State::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)p;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(SendStateData, server, client);

    return true;
}
