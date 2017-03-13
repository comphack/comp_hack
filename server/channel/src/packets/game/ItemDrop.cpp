/**
 * @file server/channel/src/packets/game/ItemDrop.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to throw away an item from an item box.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelClientConnection.h"

using namespace channel;

void DropItem(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection>& client,
    int64_t itemID)
{
    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(
            state->GetObjectUUID(itemID)));

    if(nullptr == item)
    {
        return;
    }

    auto itemBox = item->GetItemBox().Get();
    if(nullptr != itemBox)
    {
        itemBox->SetItems((size_t)item->GetBoxSlot(), NULLUUID);

        auto worldDB = server->GetWorldDatabase();
        if(!itemBox->Update(worldDB) || !item->Delete(worldDB))
        {
            LOG_ERROR(libcomp::String("Save failed during combine stack operation"
                " which may have resulted in invalid item data for character: %1\n")
                .Arg(character->GetUUID().ToString()));
        }
    }
    else
    {
        LOG_ERROR(libcomp::String("Item drop operation failed due to unknown supplied"
            " item ID on character: %1\n").Arg(character->GetUUID().ToString()));
    }
}

bool Parsers::ItemDrop::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    int64_t itemID = p.ReadS64Little();
    auto uuid = client->GetClientState()->GetObjectUUID(itemID);

    if(uuid.IsNull() || nullptr == std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(uuid)))
    {
        return false;
    }

    server->QueueWork(DropItem, server, client, itemID);

    return true;
}
