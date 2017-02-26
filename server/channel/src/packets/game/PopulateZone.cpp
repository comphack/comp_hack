/**
 * @file server/channel/src/packets/game/PopulateZone.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to populate a zone with objects
 *  and entities.
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
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <ServerDataManager.h>
#include <TcpConnection.h>

// objects Include
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

void SendZoneData(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int32_t characterUID)
{
    (void)characterUID;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    // The client's partner demon will be shown elsewhere

    auto characterManager = server->GetCharacterManager();
    characterManager->ShowEntity(client, cState->GetEntityID());

    /// @todo: Populate enemies, other players, etc
    /// @todo: Fix packet size limit when sending too many NPCs

    auto serverDataManager = server->GetServerDataManager();
    /// @todo: remove hardcoded values
    auto zoneData = serverDataManager->GetZoneData(20101);
    for(auto npc : zoneData->GetNPCs())
    {
        int32_t npcID = server->GetNextEntityID();
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_NPC_DATA);
        reply.WriteS32Little(npcID);
        reply.WriteU32Little(npc->GetID());
        reply.WriteS32Little(zoneData->GetSet());
        reply.WriteS32Little(zoneData->GetID());
        reply.WriteFloat(npc->GetX());
        reply.WriteFloat(npc->GetY());
        reply.WriteFloat(npc->GetRotation());
        reply.WriteS16Little(0);    //Unknown

        client->SendPacket(reply);

        characterManager->ShowEntity(client, npcID);
    }

    for(auto onpc : zoneData->GetObjects())
    {
        int32_t objID = server->GetNextEntityID();
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OBJECT_NPC_DATA);
        reply.WriteS32Little(objID);
        reply.WriteU32Little(onpc->GetID());
        reply.WriteU8(onpc->GetState());
        reply.WriteS32Little(zoneData->GetSet());
        reply.WriteS32Little(zoneData->GetID());
        reply.WriteFloat(onpc->GetX());
        reply.WriteFloat(onpc->GetY());
        reply.WriteFloat(onpc->GetRotation());

        client->SendPacket(reply);

        characterManager->ShowEntity(client, objID);
    }
}

bool Parsers::PopulateZone::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    int32_t characterUID = p.ReadS32Little();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    int32_t clientCharacterUID = client->GetClientState()->GetCharacterState()->GetEntityID();
    if(clientCharacterUID != characterUID)
    {
        LOG_ERROR(libcomp::String("Populate zone request sent with a character UID not matching"
            " the client connection.\nClient UID: %1\nPacket UID: %2\n").Arg(clientCharacterUID)
            .Arg(characterUID));
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(SendZoneData, server, client, characterUID);

    return true;
}
