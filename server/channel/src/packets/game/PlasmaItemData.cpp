/**
 * @file server/channel/src/packets/game/PlasmaItemData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for item information corresonding
 *  to a plasma point they have looted.
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
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerDataManager.h>

// objects Includes
#include <DropSet.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <LootBox.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "PlasmaState.h"

using namespace channel;

bool Parsers::PlasmaItemData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 5)
    {
        return false;
    }

    int32_t plasmaID = p.ReadS32Little();
    int8_t pointID = p.ReadS8();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto serverDataManager = server->GetServerDataManager();

    auto zone = cState->GetZone();
    auto pState = std::dynamic_pointer_cast<PlasmaState>(zone->GetEntity(plasmaID));
    auto point = pState ? pState->GetPoint((uint32_t)pointID) : nullptr;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PLASMA_ITEM_DATA);
    reply.WriteS32Little(plasmaID);
    reply.WriteS8(pointID);

    if(point)
    {
        bool success = true;

        auto loot = point->GetLoot();
        if(!loot)
        {
            // Generate the loot
            auto dropSet = serverDataManager->GetDropSetData(pState
                ->GetEntity()->GetDropSetID());
            if(dropSet)
            {
                loot = std::make_shared<objects::LootBox>();
                loot->SetType(objects::LootBox::Type_t::PLASMA);

                float maccaRate = (float)cState->GetCorrectValue(
                    CorrectTbl::RATE_MACCA) / 100.f;
                float magRate = (float)cState->GetCorrectValue(
                    CorrectTbl::RATE_MAG) / 100.f;

                characterManager->CreateLootFromDrops(loot, dropSet->GetDrops(),
                    cState->GetLUCK(), true, maccaRate, magRate);

                success = pState->SetLoot((uint32_t)pointID, state->GetWorldCID(),
                    loot);
            }
        }

        reply.WriteS32Little(success ? 0 : -1);

        if(success)
        {
            reply.WriteFloat(state->ToClientTime(loot->GetLootTime()));

            for(auto lItem : loot->GetLoot())
            {
                if(lItem && lItem->GetCount() > 0)
                {
                    reply.WriteU32Little(lItem->GetType());
                    reply.WriteU16Little(lItem->GetCount());
                }
                else
                {
                    reply.WriteU32Little(static_cast<uint32_t>(-1));
                    reply.WriteU16Little(0);
                }
            }
        }
    }
    else
    {
        reply.WriteS32Little(-1);
    }

    client->SendPacket(reply);

    return true;
}
