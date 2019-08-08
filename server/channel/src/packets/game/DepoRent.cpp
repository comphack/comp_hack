/**
 * @file server/channel/src/packets/game/DepoRent.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to rent a client account item/demon depository.
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
#include <ServerConstants.h>

// object Includes
#include <AccountWorldData.h>
#include <DemonBox.h>
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ClientState.h"

using namespace channel;

bool Parsers::DepoRent::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 16)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto worldData = state->GetAccountWorldData().Get();

    int64_t boxID = p.ReadS64Little();
    int64_t itemID = p.ReadS64Little();

    bool isItemDepo = false;
    int32_t delta = 0;
    bool success = false;

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));
    if(nullptr == item)
    {
        LOG_ERROR("Depo rental failed due to unknown purchase item ID.\n");
    }
    else
    {
        auto itemDayIter = SVR_CONST.DEPO_MAP_ITEM.find(item->GetType());
        auto demonDayIter = SVR_CONST.DEPO_MAP_DEMON.find(item->GetType());

        std::shared_ptr<objects::ItemBox> itemDepo;
        std::shared_ptr<objects::DemonBox> demonDepo;

        bool isNew = false;
        uint32_t dayCount = 0;
        auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
        isItemDepo = itemDayIter != SVR_CONST.DEPO_MAP_ITEM.end();
        if(isItemDepo)
        {
            itemDepo = worldData->GetItemBoxes((size_t)boxID).Get();
            if(itemDepo == nullptr)
            {
                itemDepo = libcomp::PersistentObject::New<objects::ItemBox>();
                itemDepo->SetType(objects::ItemBox::Type_t::ITEM_DEPO);
                itemDepo->SetBoxID(boxID);
                itemDepo->SetAccount(state->GetAccountUID());

                itemDepo->Register(itemDepo);
                worldData->SetItemBoxes((size_t)boxID, itemDepo);

                dbChanges->Insert(itemDepo);
                dbChanges->Update(worldData);
                isNew = true;
            }

            dayCount = itemDayIter->second;

            success = true;
        }
        else if(demonDayIter != SVR_CONST.DEPO_MAP_DEMON.end())
        {
            auto demonBoxID = (int8_t)boxID;

            demonDepo = worldData->GetDemonBoxes((size_t)(demonBoxID-1)).Get();
            if(demonDepo == nullptr)
            {
                demonDepo = libcomp::PersistentObject::New<objects::DemonBox>();
                demonDepo->SetBoxID(demonBoxID);
                demonDepo->SetAccount(state->GetAccountUID());

                demonDepo->Register(demonDepo);
                worldData->SetDemonBoxes((size_t)(demonBoxID-1), demonDepo);

                dbChanges->Insert(demonDepo);
                dbChanges->Update(worldData);
                isNew = true;
            }

            dayCount = demonDayIter->second;

            success = true;
        }
        else
        {
            LOG_ERROR("Depo rental failed due to unknown/invalid purchase item type.\n");
        }

        if(success)
        {
            // Set the expiration timestamp to the specified offset past the current time
            uint32_t timestamp = (uint32_t)time(0);
            delta = (int32_t)(dayCount * 24 * 60 * 60);

            if(isItemDepo)
            {
                if(itemDepo->GetRentalExpiration() > timestamp)
                {
                    // Extend current time
                    delta = delta +
                        (int32_t)(itemDepo->GetRentalExpiration() - timestamp);
                }

                itemDepo->SetRentalExpiration((uint32_t)(timestamp +
                    (uint32_t)delta));

                if(!isNew)
                {
                    dbChanges->Update(itemDepo);
                }
            }
            else
            {
                if(demonDepo->GetRentalExpiration() > timestamp)
                {
                    // Extend current time
                    delta = delta +
                        (int32_t)(demonDepo->GetRentalExpiration() - timestamp);
                }

                demonDepo->SetRentalExpiration((uint32_t)(timestamp +
                    (uint32_t)delta));

                if(!isNew)
                {
                    dbChanges->Update(demonDepo);
                }
            }

            // Update the used item's box
            auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
                libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
            if(itemBox)
            {
                itemBox->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
                dbChanges->Update(itemBox);
                server->GetCharacterManager()->SendItemBoxData(client, itemBox,
                    { (uint16_t)item->GetBoxSlot() });
            }
            dbChanges->Delete(item);

            server->GetWorldDatabase()->QueueChangeSet(dbChanges);
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEPO_RENT);
    reply.WriteS8(isItemDepo ? 0 : 1);
    reply.WriteS64Little(boxID);
    reply.WriteS32Little(success ? 0 : -1);
    reply.WriteS32Little(delta);

    client->SendPacket(reply);

    return true;
}
