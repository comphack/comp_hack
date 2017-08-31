/**
 * @file server/channel/src/packets/game/ClanForm.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to form a clan based on a clan item being
 *  used.
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
#include <ServerConstants.h>

// object Includes
#include <ActivatedAbility.h>
#include <Clan.h>
#include <Item.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ClanForm::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 7)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();

    int32_t entityID = p.ReadS32Little();
    int8_t activationID = p.ReadS8();
    libcomp::String clanName = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    const int8_t ERR_IN_CLAN = -1;
    const int8_t ERR_DUPE_NAME = -2;
    const int8_t ERR_INVALID_NAME = -3;
    const int8_t ERR_FAIL = -5;

    int8_t errorCode = 0;

    auto sourceState = state->GetEntityState(entityID);
    auto activatedAbility = sourceState ? sourceState->GetActivatedAbility() : nullptr;
    if(!activatedAbility || activatedAbility->GetActivationID() != activationID)
    {
        // Request is invalid but don't kill the connection or cancel the skill
        errorCode = ERR_FAIL;
    }
    else if(state->GetClanID() > 0)
    {
        errorCode = ERR_IN_CLAN;
    }
    else if(clanName.Size() > 32)
    {
        // 16 2-byte or 8 4-byte character limit
        errorCode = ERR_INVALID_NAME;
    }
    else
    {
        auto worldDB = server->GetWorldDatabase();
        auto existing = objects::Clan::LoadClanByName(worldDB, clanName);
        if(existing)
        {
            errorCode = ERR_DUPE_NAME;
        }
        else
        {
            auto item = std::dynamic_pointer_cast<objects::Item>(
                libcomp::PersistentObject::GetObjectByUUID(
                state->GetObjectUUID(activatedAbility->GetTargetObjectID())));

            uint32_t itemID = item ? item->GetType() : 0;
            auto baseZoneIter = SVR_CONST.CLAN_FORM_MAP.find(itemID);

            if(baseZoneIter != SVR_CONST.CLAN_FORM_MAP.end())
            {
                libcomp::Packet request;
                request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
                request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
                request.WriteS32Little(state->GetWorldCID());
                request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                    clanName, true);
                request.WriteU32Little(baseZoneIter->second);
                request.WriteS8(activationID);

                server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
            }
            else
            {
                errorCode = ERR_FAIL;
            }
        }
    }

    if(errorCode)
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_FORM);
        reply.WriteS32Little(0);
        reply.WriteS8(errorCode);

        client->SendPacket(reply);

        server->GetSkillManager()->CancelSkill(sourceState, (uint8_t)activationID);
    }

    return true;
}
