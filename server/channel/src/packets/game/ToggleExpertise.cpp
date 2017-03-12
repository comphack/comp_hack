/**
 * @file server/channel/src/packets/game/ToggleExpertise.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to enable or disable an expertise.
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

// channel Includes
#include "ChannelServer.h"

// objects Includes
#include <Character.h>
#include <Expertise.h>

using namespace channel;

bool Parsers::ToggleExpertise::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 6)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    int8_t expID = p.ReadS8();
    int8_t disabled = p.ReadS8();

    // Check the expertise array bounds
    if(expID < 0 || expID > 37)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    if(cState->GetEntityID() != entityID)
    {
        return false;
    }

    auto character = cState->GetEntity();
    auto expertise = character->GetExpertises((size_t)expID).Get();
    if(nullptr == expertise)
    {
        expertise = std::shared_ptr<objects::Expertise>(new objects::Expertise);
        expertise->Register(expertise);
        expertise->SetCharacter(character);

        auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
        auto db = server->GetWorldDatabase();
        if(!expertise->Insert(db))
        {
            return false;
        }

        character->SetExpertises((size_t)expID, expertise);
    }

    expertise->SetDisabled(disabled != 0);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TOGGLE_EXPERTISE);
    reply.WriteS32Little(entityID);
    reply.WriteS8(expID);
    reply.WriteU8((uint8_t)disabled);

    client->SendPacket(reply);

    return true;
}
