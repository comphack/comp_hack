/**
 * @file server/channel/src/packets/game/HotbarSave.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to save a hotbar page. Requests to
 *  save the hotbar happen on logout and are also delayed to the next
 *  5 minute interval that elapses since login after making a change.
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
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Hotbar.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

struct HotbarItemRequest
{
    int8_t Type;
    int64_t ObjectID;
};

void SaveHotbarItems(const std::shared_ptr<ChannelServer> server, 
    const std::shared_ptr<ChannelClientConnection> client,
    size_t page, std::vector<HotbarItemRequest> items)
{
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    auto hotbar = character->GetHotbars(page).Get();

    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        state->GetAccountUID());
    if(nullptr == hotbar)
    {
        hotbar = libcomp::PersistentObject::New<objects::Hotbar>();
        hotbar->SetCharacter(character);
        hotbar->Register(hotbar);
        character->SetHotbars(page, hotbar);

        dbChanges->Update(character);
        dbChanges->Insert(hotbar);
    }
    else
    {
        dbChanges->Update(hotbar);
    }

    for(size_t i = 0; i < 16; i++)
    {
        auto item = items[i];

        // Demons and equipment need to be instance references
        bool instanceRef = item.Type == 3 || item.Type == 5;
        if(instanceRef)
        {
            hotbar->SetItems(i, state->GetObjectUUID(item.ObjectID));
            hotbar->SetItemIDs(i, 0);
        }
        else
        {
            hotbar->SetItems(i, NULLUUID);
            hotbar->SetItemIDs(i, (uint32_t)item.ObjectID);
        }
        hotbar->SetItemTypes(i, item.Type);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_HOTBAR_SAVE);
    reply.WriteS8((int8_t)page);
    reply.WriteS32(0);

    client->SendPacket(reply);

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);
}

bool Parsers::HotbarSave::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 145)
    {
        return false;
    }

    int8_t page = p.ReadS8();

    std::vector<HotbarItemRequest> items;
    for(size_t i = 0; i < 16; i++)
    {
        HotbarItemRequest item;
        item.Type = p.ReadS8();
        item.ObjectID = p.ReadS64Little();
        items.push_back(item);
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    server->QueueWork(SaveHotbarItems, server, client, (size_t)page, items);

    return true;
}
