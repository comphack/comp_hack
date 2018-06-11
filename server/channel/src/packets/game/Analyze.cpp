/**
 * @file server/channel/src/packets/game/Analyze.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to analyze another player character or
 *  their partner demon (basic details or time trial records).
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

// object Includes
#include <CharacterProgress.h>
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
    auto state = client->GetClientState();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto targetState = ClientState::GetEntityClientState(targetEntityID);
    auto entityState = targetState
        ? targetState->GetEntityState(targetEntityID, false) : nullptr;

    if(!entityState)
    {
        LOG_ERROR(libcomp::String("Attempted to analyze an entity that no"
            " longer exists: %1\n").Arg(state->GetAccountUID().ToString()));
        return true;
    }

    switch(entityState->GetEntityType())
    {
    case EntityType_t::CHARACTER:
        {
            auto cState = std::dynamic_pointer_cast<CharacterState>(entityState);
            auto character = cState ? cState->GetEntity() : nullptr;
            if(p.Size() == 6)
            {
                // Character analyze
                uint16_t equipMask = p.ReadU16Little();

                libcomp::Packet reply;
                reply.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_EQUIPMENT_ANALYZE);
                reply.WriteS32Little(targetEntityID);
                reply.WriteU16Little(equipMask);
                for(int8_t slot = 0; slot < 15; slot++)
                {
                    // Only return the equipment requested
                    if((equipMask & (1 << slot)) == 0) continue;

                    auto equip = character ? character->GetEquippedItems(
                            (size_t)slot).Get() : nullptr;

                    characterManager->GetItemDetailPacketData(reply, equip, 0);
                }

                client->SendPacket(reply);
            }
            else
            {
                // Time trial analyze
                auto progress = character ? character->GetProgress().Get()
                    : nullptr;

                libcomp::Packet reply;
                reply.WritePacketCode(ChannelToClientPacketCode_t::
                    PACKET_ANALYZE_DUNGEON_RECORDS);
                reply.WriteS32Little(targetEntityID);

                if(progress)
                {
                    reply.WriteS8(progress->GetTimeTrialID());

                    reply.WriteS8((int8_t)progress->TimeTrialRecordsCount());
                    for (uint16_t trialTime : progress->GetTimeTrialRecords())
                    {
                        reply.WriteU16Little(trialTime ? trialTime
                            : (uint16_t)-1);
                    }
                }
                else
                {
                    reply.WriteBlank(6);
                }

                client->SendPacket(reply);
            }
        }
        break;
    case EntityType_t::PARTNER_DEMON:
        {
            // Partner demon analyze
            auto dState = std::dynamic_pointer_cast<DemonState>(entityState);

            libcomp::Packet reply;
            reply.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_ANALYZE_DEMON);
            reply.WriteS32Little(targetEntityID);

            auto d = dState ? dState->GetEntity() : nullptr;
            if(d)
            {
                for(size_t i = 0; i < 8; i++)
                {
                    auto skillID = d->GetLearnedSkills(i);
                    reply.WriteU32Little(skillID == 0 ? (uint32_t)-1 : skillID);
                }

                for(int8_t reunionRank : d->GetReunion())
                {
                    reply.WriteS8(reunionRank);
                }

                reply.WriteU8(0);   // Unknown

                for(uint16_t forceStack : d->GetForceStack())
                {
                    reply.WriteU16Little(forceStack);
                }

                reply.WriteU8(0);   //Unknown
                reply.WriteU8(0);   //Mitama type

                // Reunion bonuses (12 * 8 ranks)
                for(size_t i = 0; i < 96; i++)
                {
                    reply.WriteU8(0);
                }

                // Equipment
                for(size_t i = 0; i < 4; i++)
                {
                    auto equip = d->GetEquippedItems(i).Get();
                    if(equip)
                    {
                        reply.WriteS64Little(state->GetObjectID(
                            equip->GetUUID()));
                        reply.WriteU32Little(equip->GetType());
                    }
                    else
                    {
                        reply.WriteS64Little(-1);
                        reply.WriteU32Little(static_cast<uint32_t>(-1));
                    }
                }
            }
            else
            {
                reply.WriteBlank(179);
            }

            client->SendPacket(reply);
        }
        break;
    default:
        LOG_ERROR(libcomp::String("Attempted to analyze an entity that is"
            " not valid: %1\n").Arg(state->GetAccountUID().ToString()));
        break;
    }

    return true;
}
