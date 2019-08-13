/**
 * @file server/channel/src/packets/game/MaterialInsert.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to insert materials into the tank.
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
#include <ServerConstants.h>

// objects Includes
#include <Item.h>
#include <MiTankData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::MaterialInsert::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    int64_t itemID = p.ReadS64Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));
    uint32_t itemType = item ? item->GetType() : 0;

    // Get the material definition and also check that a disassembly trigger exists for
    // the item as each material type has one
    std::shared_ptr<objects::MiTankData> mTank;
    for(auto& pair : definitionManager->GetTankData())
    {
        if(pair.second->GetItemID() == itemType)
        {
            mTank = pair.second;
            break;
        }
    }

    auto triggerDef = definitionManager->GetDisassemblyTriggerData(itemType);

    bool playerHasTank = CharacterManager::HasValuable(character,
        SVR_CONST.VALUABLE_MATERIAL_TANK);

    int32_t inserted = 0;
    bool success = false;
    if(playerHasTank && mTank && triggerDef)
    {
        // Unlike disassembly, direct material insertion will not remove stacks
        // over the delta free for that type
        int32_t maxStack = (int32_t)mTank->GetMaxStack();

        int32_t newStack = character->GetMaterials(itemType) + item->GetStackSize();

        if(newStack > maxStack)
        {
            newStack = (int32_t)maxStack;
        }

        inserted = newStack - character->GetMaterials(itemType);

        if(inserted > 0)
        {
            std::unordered_map<uint32_t, uint32_t> items;
            items[itemType] = (uint32_t)inserted;
            if(characterManager->AddRemoveItems(client, items, false, itemID))
            {
                character->SetMaterials(itemType, (uint16_t)newStack);

                server->GetWorldDatabase()
                    ->QueueUpdate(character, state->GetAccountUID());
            }
        }

        success = true;
    }
    else if(!triggerDef)
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Player '%1' attempted to insert a "
                "non-material item into the material container: %2\n")
                .Arg(state->GetAccountUID().ToString()).Arg(itemType);
        });
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MATERIAL_INSERT);
    reply.WriteS64Little(itemID);
    reply.WriteS32Little(success ? 0 : -1);
    reply.WriteS32Little(inserted);

    client->SendPacket(reply);

    if(success)
    {
        std::set<uint32_t> updates = { itemType };
        characterManager->SendMaterials(client, updates);
    }

    return true;
}
