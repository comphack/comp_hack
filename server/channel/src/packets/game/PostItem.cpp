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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <AccountWorldData.h>
#include <Item.h>
#include <ItemBox.h>
#include <PostItem.h>

// channel Includes
#include "ChannelServer.h"

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
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto worldDB = server->GetWorldDatabase();

    int32_t postID = p.ReadS32Little();
    int32_t unknown = p.ReadS32Little();
    (void)unknown;

    // Always reload the post
    auto worldData = objects::AccountWorldData::LoadAccountWorldDataByAccount(worldDB,
        state->GetAccountUID());

    auto itemUUID = state->GetLocalObjectUUID(postID);

    bool success = false;
    if(!itemUUID.IsNull())
    {
        size_t idx = 0;
        std::shared_ptr<objects::PostItem> postItem;
        for(idx = 0; idx < worldData->PostCount(); idx++)
        {
            if(worldData->GetPost(idx).GetUUID() == itemUUID)
            {
                postItem = worldData->GetPost(idx).Get();
                break;
            }
        }

        if(postItem)
        {
            auto inventory = state->GetCharacterState()->GetEntity()
                ->GetItemBoxes(0).Get();
            auto characterManager = server->GetCharacterManager();
            auto newItem = characterManager->GenerateItem(postItem->GetType(), 1);

            int8_t nextSlot = -1;
            for(size_t i = 0; i < 50; i++)
            {
                if(inventory->GetItems(i).IsNull())
                {
                    nextSlot = (int8_t)i;
                    break;
                }
            }
            
            if(nextSlot != -1)
            {
                state->SetObjectID(newItem->GetUUID(),
                    server->GetNextObjectID());

                auto changes = libcomp::DatabaseChangeSet::Create();

                newItem->SetItemBox(inventory);
                newItem->SetBoxSlot(nextSlot);
                inventory->SetItems((size_t)nextSlot, newItem);
                worldData->RemovePost(idx);

                changes->Insert(newItem);
                changes->Update(inventory);
                changes->Update(worldData);
                changes->Delete(postItem);

                if(!worldDB->ProcessChangeSet(changes))
                {
                    LOG_ERROR("Post item retrieval failed to save.\n");
                    state->SetLogoutSave(true);
                    client->Close();
                }
                else
                {
                    success = true;

                    std::list<uint16_t> updatedSlots = { (uint16_t)nextSlot };
                    characterManager->SendItemBoxData(client, inventory,
                        updatedSlots);
                }
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
