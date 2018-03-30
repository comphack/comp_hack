/**
 * @file server/channel/src/packets/game/Analyze.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to analyze another player character or
 *  their partner demon.
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

// object Includes
#include <Item.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::Analyze::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4 && p.Size() != 6)
    {
        return false;
    }

    int32_t targetEntityID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto targetState = ClientState::GetEntityClientState(targetEntityID);
    auto entityState = targetState
        ? targetState->GetEntityState(targetEntityID, false) : nullptr;

    if(p.Size() == 6)
    {
        // Character analyze
        uint16_t equipMask = p.ReadU16Little();

        auto cState = std::dynamic_pointer_cast<CharacterState>(entityState);

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EQUIPMENT_ANALYZE);
        reply.WriteS32Little(targetEntityID);
        reply.WriteU16Little(equipMask);
        for(int8_t slot = 0; slot < 15; slot++)
        {
            // Only return the equipment requested
            if((equipMask & (1 << slot)) == 0) continue;

            auto equip = cState
                ? cState->GetEntity()->GetEquippedItems((size_t)slot).Get() : nullptr;

            characterManager->GetItemDetailPacketData(reply, equip, 0);
        }

        connection->SendPacket(reply);
    }
    else
    {
        // Partner demon analyze
        auto dState = std::dynamic_pointer_cast<DemonState>(entityState);

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ANALYZE_DEMON);
        reply.WriteS32Little(targetEntityID);

        auto d = dState ? dState->GetEntity() : nullptr;
        if(d)
        {
            for(size_t i = 0; i < 8; i++)
            {
                auto skillID = d->GetLearnedSkills(i);
                reply.WriteU32Little(skillID == 0 ? (uint32_t)-1 : skillID);
            }

            for(size_t i = 0; i < 12; i++)
            {
                reply.WriteS8(d->GetReunion(i));
            }

            reply.WriteU8(0);   // Unknown

            //Force Stack?
            for(size_t i = 0; i < 8; i++)
            {
                reply.WriteU16Little(0);
            }

            reply.WriteU8(0);   //Unknown
            reply.WriteU8(0);   //Mitama type

            //Reunion bonuses (12 * 8 ranks)
            for(size_t i = 0; i < 96; i++)
            {
                reply.WriteU8(0);
            }

            //Characteristics panel
            for(size_t i = 0; i < 4; i++)
            {
                reply.WriteU32Little(static_cast<uint32_t>(-1));    //Item type
            }
        }
        else
        {
            reply.WriteBlank(179);
        }

        connection->SendPacket(reply);
    }

    return true;
}
