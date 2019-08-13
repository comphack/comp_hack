/**
 * @file server/channel/src/packets/game/DiasporaEnter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to enter a Diaspora instance after team
 *  establishment.
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
#include <ServerDataManager.h>

// object Includes
#include <InstanceAccess.h>

// channel Includes
#include "ChannelServer.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::DiasporaEnter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t confirmation = p.ReadS8();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();

    if(confirmation != 0)
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Player set up to enter Disapora but"
                " confirmation was not returned: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        return true;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto zoneManager = server->GetZoneManager();

    auto instAccess = zoneManager->GetInstanceAccess(state->GetWorldCID());
    auto variant = instAccess ? server->GetServerDataManager()
        ->GetZoneInstanceVariantData(instAccess->GetVariantID()) : nullptr;
    if(variant && variant->GetInstanceType() == InstanceType_t::DIASPORA)
    {
        libcomp::Packet reply;
        reply.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DIASPORA_ENTER);
        reply.WriteS8(0);

        client->QueuePacket(reply);

        zoneManager->MoveToInstance(client, instAccess, true);

        client->FlushOutgoing();
    }

    return true;
}
