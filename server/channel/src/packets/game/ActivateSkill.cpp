/**
 * @file server/channel/src/packets/game/ActivateSkill.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to activate a character or demon skill.
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
#include <Constants.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>

// object Includes
#include <Character.h>
#include <Item.h>
#include <ItemBox.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

const uint32_t SKILL_SUMMON_DEMON = 0x00001648;
const uint32_t SKILL_STORE_DEMON = 0x00001649;
const uint32_t SKILL_EQUIP_ITEM = 0x00001654;

/// @todo: Replace the following two functions with proper skill handling

void SendExecuteSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t characterEntityID, uint32_t skillID)
{
    auto state = client->GetClientState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EXECUTE_SKILL);
    reply.WriteS32Little(characterEntityID);
    reply.WriteU32Little(skillID);
    reply.WriteS8(1);
    reply.WriteS32Little(0);
    reply.WriteFloat(state->ToClientTime(ChannelServer::GetServerTime()));
    reply.WriteFloat(state->ToClientTime(ChannelServer::GetServerTime()));
    reply.WriteBlank(31);

    client->SendPacket(reply);
}

void SendCompleteSkill(const std::shared_ptr<ChannelClientConnection> client,
    int32_t characterEntityID, uint32_t skillID)
{
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_COMPLETE_SKILL);
    reply.WriteS32Little(characterEntityID);
    reply.WriteU32Little(skillID);
    reply.WriteS8(1);
    reply.WriteFloat(0.0f);
    reply.WriteU8(1);
    reply.WriteFloat(300.0f);   //Run speed
    reply.WriteU8(0);

    client->SendPacket(reply);
}

void EquipItem(CharacterManager* characterManager,
    const std::shared_ptr<ChannelClientConnection> client,
    int64_t itemID)
{
    auto characterEntityID = client->GetClientState()
        ->GetCharacterState()->GetEntityID();

    /// @todo: add charge time
    SendExecuteSkill(client, characterEntityID, SKILL_EQUIP_ITEM);
    SendCompleteSkill(client, characterEntityID, SKILL_EQUIP_ITEM);

    characterManager->EquipItem(client, itemID);
}

void SummonDemon(CharacterManager* characterManager,
    const std::shared_ptr<ChannelClientConnection> client,
    int64_t demonID)
{
    auto characterEntityID = client->GetClientState()
        ->GetCharacterState()->GetEntityID();

    /// @todo: implement MAG cost
    SendExecuteSkill(client, characterEntityID, SKILL_SUMMON_DEMON);
    SendCompleteSkill(client, characterEntityID, SKILL_SUMMON_DEMON);

    characterManager->SummonDemon(client, demonID);
}

void StoreDemon(CharacterManager* characterManager,
    const std::shared_ptr<ChannelClientConnection> client)
{
    auto characterEntityID = client->GetClientState()
        ->GetCharacterState()->GetEntityID();

    SendExecuteSkill(client, characterEntityID, SKILL_STORE_DEMON);
    SendCompleteSkill(client, characterEntityID, SKILL_STORE_DEMON);

    characterManager->StoreDemon(client);
}

bool Parsers::ActivateSkill::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 12)
    {
        return false;
    }

    int32_t sourceEntityID = p.ReadS32Little();
    uint32_t skillType = p.ReadU32Little();

    uint32_t targetType = p.ReadU32Little();
    if(targetType != ACTIVATION_NOTARGET && p.Left() == 0)
    {
        return false;
    }

    int64_t targetObjectID = -1;
    switch(targetType)
    {
        case ACTIVATION_NOTARGET:
            //Nothing special to do
            break;
        case ACTIVATION_DEMON:
        case ACTIVATION_ITEM:
            targetObjectID = p.ReadS64Little();
            break;
        case ACTIVATION_TARGET:
            /// @todo: implement these
        default:
            {
                LOG_ERROR(libcomp::String("Unknown skill target type encountered: %1")
                    .Arg(targetType));
                return false;
            }
            break;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    switch(skillType)
    {
        case SKILL_EQUIP_ITEM:
            if(targetObjectID == -1)
            {
                LOG_ERROR(libcomp::String("Invalid item specified to equip: %1\n")
                    .Arg(targetObjectID));
                return false;
            }

            if(sourceEntityID != state->GetCharacterState()->GetEntityID())
            {
                LOG_ERROR(libcomp::String("Invalid character specified to equip: %1\n")
                    .Arg(sourceEntityID));
                return false;
            }
            
            server->QueueWork(EquipItem, server->GetCharacterManager(), client, targetObjectID);
            break;
        case SKILL_SUMMON_DEMON:
            if(targetObjectID == -1)
            {
                LOG_ERROR(libcomp::String("Invalid demon specified to summon: %1\n")
                    .Arg(targetObjectID));
                return false;
            }

            if(sourceEntityID != state->GetCharacterState()->GetEntityID())
            {
                LOG_ERROR(libcomp::String("Invalid character specified to summon specified demon: %1\n")
                    .Arg(sourceEntityID));
                return false;
            }

            server->QueueWork(SummonDemon, server->GetCharacterManager(), client, targetObjectID);
            break;
        case SKILL_STORE_DEMON:
            if(targetObjectID == -1)
            {
                LOG_ERROR(libcomp::String("Invalid demon specified to store: %1\n")
                    .Arg(targetObjectID));
                return false;
            }

            if(sourceEntityID != state->GetCharacterState()->GetEntityID())
            {
                LOG_ERROR(libcomp::String("Invalid character specified to store specified demon: %1\n")
                    .Arg(sourceEntityID));
                return false;
            }

            server->QueueWork(StoreDemon, server->GetCharacterManager(), client);
            break;
            /// @todo: Handle normal abilities
        default:
            LOG_ERROR(libcomp::String("Unknown skill type encountered: %1\n")
                .Arg(skillType));
            return false;
            break;
    }

    return true;
}
