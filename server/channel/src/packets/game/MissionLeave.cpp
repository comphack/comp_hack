/**
 * @file server/channel/src/packets/game/MissionLeave.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to leave a Mission instance.
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
#include <DefinitionManager.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerDataManager.h>

// objects Includes
#include <MiMissionData.h>
#include <MiMissionExit.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::MissionLeave::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 5)
    {
        return false;
    }

    uint32_t missionID = p.ReadU32Little();
    int8_t exitID = p.ReadS8();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto definitionManager = server->GetDefinitionManager();
    auto zoneManager = server->GetZoneManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto zone = state->GetZone();
    auto instance = zone ? zone->GetInstance() : nullptr;
    auto variant = instance ? instance->GetVariant() : nullptr;

    std::shared_ptr<objects::MiMissionExit> exit;
    if(variant && variant->GetInstanceType() == InstanceType_t::MISSION &&
        variant->GetSubID() == missionID)
    {
        auto missionData = definitionManager->GetMissionData(missionID);
        exit = missionData ? missionData->GetExits((size_t)exitID) : nullptr;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MISSION_LEAVE);
    reply.WriteS8(exit ? 0 : -1);

    client->QueuePacket(reply);

    if(exit)
    {
        uint32_t zoneID = exit->GetZoneID();
        float xCoord = exit->GetX();
        float yCoord = exit->GetY();
        float rot = exit->GetRotation();

        auto zoneDef = server->GetServerDataManager()->GetZoneData(zoneID, 0);
        uint32_t dynamicMapID = zoneDef ? zoneDef->GetDynamicMapID() : 0;

        zoneManager->EnterZone(client, zoneID, dynamicMapID, xCoord,
            yCoord, rot, true);
    }

    client->FlushOutgoing();

    return true;
}
