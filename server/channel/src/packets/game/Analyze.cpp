/**
 * @file server/channel/src/packets/game/Analyze.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to analyze another player character.
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
#include <Item.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::Analyze::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 6)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    int32_t targetEntityID = p.ReadS32Little();
    uint16_t unknown = p.ReadU16Little();

    auto targetState = ClientState::GetEntityClientState(targetEntityID);
    auto targetChar = targetState ? targetState->GetCharacterState()->GetEntity() : nullptr;
    
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EQUIPMENT_ANALYZE);
    reply.WriteS32Little(targetEntityID);
    reply.WriteU16Little(unknown);
    for(int8_t slot = 0; slot < 15; slot++)
    {
        auto equip = targetChar
            ? targetChar->GetEquippedItems((size_t)slot).Get() : nullptr;

        if(equip)
        {
            reply.WriteS16Little(equip->GetTarot());
            reply.WriteS16Little(equip->GetSoul());

            for(auto modSlot : equip->GetModSlots())
            {
                reply.WriteU16Little(modSlot);
            }

            reply.WriteU32Little(equip->GetType());
            reply.WriteU32Little(equip->GetType());

            for(auto bonus : equip->GetFuseBonuses())
            {
                reply.WriteS8(bonus);
            }
        }
        else
        {
            reply.WriteBlank(14);
            reply.WriteU32Little(static_cast<uint32_t>(-1));
            reply.WriteU32Little(static_cast<uint32_t>(-1));
            reply.WriteBlank(3);
        }
    }

    connection->SendPacket(reply);

    return true;
}
