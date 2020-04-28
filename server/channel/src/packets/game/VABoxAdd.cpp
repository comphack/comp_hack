/**
 * @file server/channel/src/packets/game/VABoxAdd.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to create a VA item and add it to
 *  the closet.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

// object Includes
#include <Item.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiUseRestrictionsData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::VABoxAdd::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 12)
    {
        return false;
    }

    int32_t unused = p.ReadS32Little(); // Always 0
    int64_t itemID = p.ReadS64Little();
    (void)unused;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    auto itemDef = item ? server->GetDefinitionManager()
        ->GetItemData(item->GetType()) : nullptr;

    int8_t slot = -1;

    bool failure = item == nullptr || itemDef == nullptr ||
        itemDef->GetBasic()->GetEquipType() ==
            objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE;
    if(!failure)
    {
        auto restr = itemDef->GetRestriction();
        failure = restr->GetGender() != GENDER_NA &&
            cState->GetGender() != restr->GetGender();
    }

    if(!failure)
    {
        for(size_t i = 0; i < 50; i++)
        {
            if(character->GetVACloset(i) == 0)
            {
                slot = (int8_t)i;
                break;
            }
        }

        if(slot == -1)
        {
            failure = true;
        }
    }

    if(!failure)
    {
        bool removed = false;
        for(uint32_t removeItemType : SVR_CONST.VA_ADD_ITEMS)
        {
            std::unordered_map<uint32_t, uint32_t> remove;
            remove[removeItemType] = 1;

            if(characterManager->AddRemoveItems(client, remove, false))
            {
                removed = true;
                break;
            }
        }

        if(removed)
        {
            character->SetVACloset((size_t)slot, item->GetType());
        }
        else
        {
            failure = true;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VA_BOX_ADD);
    reply.WriteS32Little(failure ? -1 : 0);
    reply.WriteS32Little(0);    // Unknown
    reply.WriteS8(slot);
    reply.WriteU32Little(item ? item->GetType() : 0);

    client->SendPacket(reply);

    if(!failure)
    {
        server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());
    }

    return true;
}
