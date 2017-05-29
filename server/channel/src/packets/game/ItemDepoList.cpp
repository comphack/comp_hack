/**
 * @file server/channel/src/packets/game/ItemDepoList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to list the client account's item depositories.
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
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <AccountWorldData.h>
#include <Item.h>
#include <ItemBox.h>

using namespace channel;

bool Parsers::ItemDepoList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 0)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto worldData = state->GetAccountWorldData();

    auto timestamp = ChannelServer::GetServerTimeInSeconds();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_DEPO_LIST);

    reply.WriteS32Little(0);    // Unknown

    auto depoCount = worldData->ItemBoxesCount();
    reply.WriteS32Little((int32_t)depoCount);
    for(size_t i = 0; i < depoCount; i++)
    {
        auto depo = worldData->GetItemBoxes(i);

        // First box is always available
        if(i > 0 && (depo.IsNull() || depo->GetRentalExpiration() == 0))
        {
            reply.WriteS32Little(-1);
            reply.WriteS32Little(0);
            reply.WriteS64Little(0);
            reply.WriteS32Little(0);
        }
        else
        {
            reply.WriteS32Little(ChannelServer::GetExpirationInSeconds(
                depo->GetRentalExpiration(), timestamp));

            int32_t itemCount = 0, mag = 0;
            int64_t macca = 0;

            for(auto item : depo->GetItems())
            {
                if(item.IsNull())
                {
                    continue;
                }

                itemCount++;

                switch(item->GetType())
                {
                    case ITEM_MACCA_NOTE:
                        macca = (int64_t)(macca + item->GetStackSize()
                            * ITEM_MACCA_NOTE_AMOUNT);
                        break;
                    case ITEM_MACCA:
                        macca = (int64_t)(macca + item->GetStackSize());
                        break;
                    case ITEM_MAGNETITE:
                        mag = (int32_t)(mag + item->GetStackSize());
                        break;
                    default:
                        break;
                }
            }

            reply.WriteS32Little(itemCount);
            reply.WriteS64Little(macca);
            reply.WriteS32Little(mag);
        }
    }

    client->SendPacket(reply);

    return true;
}
