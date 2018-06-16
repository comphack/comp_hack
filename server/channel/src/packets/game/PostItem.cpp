/**
 * @file server/channel/src/packets/game/PostItem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to move an Post item into the player's
 *  inventory.
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
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <AccountWorldData.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiShopProductData.h>
#include <PostItem.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::PostItem::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto lobbyDB = server->GetLobbyDatabase();

    int32_t postID = p.ReadS32Little();
    int32_t targetSlot = p.ReadS32Little();
    (void)targetSlot;

    auto itemUUID = state->GetLocalObjectUUID(postID);

    bool success = false;
    if(!itemUUID.IsNull())
    {
        auto postItem = libcomp::PersistentObject::LoadObjectByUUID<objects::PostItem>(lobbyDB,
            itemUUID);
        auto productData = postItem
            ? definitionManager->GetShopProductData(postItem->GetType()) : nullptr;

        if(productData)
        {
            auto characterManager = server->GetCharacterManager();

            // Since the character and lobby are on different servers, we cannot
            // batch this up into a single transaction. The loss of a CP item from
            // the post is more damaging to the player than a server caused duplication
            // so only delete the post item after the item has been created.
            std::unordered_map<uint32_t, uint32_t> items;
            items[productData->GetItem()] = productData->GetStack();
            success = characterManager->AddRemoveItems(client, items, true);
            if(success && !postItem->Delete(lobbyDB))
            {
                // If this fails we don't have a good way to recover, disconnect
                // the player so they don't get into an invalid state
                LOG_ERROR("Post item retrieval failed to save.\n");
                state->SetLogoutSave(true);
                client->Close();
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_POST_ITEM);
    reply.WriteS32Little(postID);
    reply.WriteS32Little(success ? 0 : -1);

    client->SendPacket(reply);

    return true;
}
