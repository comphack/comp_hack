/**
 * @file server/channel/src/packets/game/BazaarItemUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to update an item in the player's bazaar market.
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

// objects Includes
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <BazaarItem.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::BazaarItemUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 13)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    auto worldData = state->GetAccountWorldData().Get();
    auto bazaarData = worldData->GetBazaarData().Get();

    int8_t slot = p.ReadS8();
    int64_t itemID = p.ReadS64Little();
    int32_t price = p.ReadS32Little();

    bool ok = true;

    auto bItem = bazaarData->GetItems((size_t)slot).Get();
    if(bItem == nullptr || bItem->GetItem().GetUUID() != state->GetObjectUUID(itemID))
    {
        LOG_ERROR("BazaarItemUpdate request encountered with invalid item or"
            " source slot\n");
        ok = false;
    }
    else
    {
        bItem->SetCost((uint32_t)price);

        auto dbChanges = libcomp::DatabaseChangeSet::Create();
        dbChanges->Update(bItem);

        if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
        {
            LOG_ERROR(libcomp::String("BazaarItemUpdate failed to save: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            state->SetLogoutSave(false);
            client->Close();
            return true;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_ITEM_UPDATE);
    reply.WriteS8(slot);
    reply.WriteS64Little(itemID);
    reply.WriteS32Little(price);
    reply.WriteS32Little(ok ? 0 : -1);

    client->SendPacket(reply);

    return true;
}
