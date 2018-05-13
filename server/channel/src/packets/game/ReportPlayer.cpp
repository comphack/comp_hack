/**
 * @file server/channel/src/packets/game/ReportPlayer.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to report a player for abusive actions.
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
#include <ReportedPlayer.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ReportPlayer::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() < 11)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();

    int32_t unused = p.ReadS32Little(); // Always zero
    (void)unused;

    int8_t subject = p.ReadS8();

    // Player name => location => comment
    std::array<libcomp::String, 3> textParams;
    for(size_t i = 0; i < 3; i++)
    {
        if(p.Left() < (uint16_t)(2 + p.PeekU16Little()))
        {
            return false;
        }

        textParams[i] = p.ReadString16Little(
            state->GetClientStringEncoding(), true);
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto worldDB = server->GetWorldDatabase();

    auto player = objects::Character::LoadCharacterByName(worldDB,
        textParams[0]);

    auto report = libcomp::PersistentObject::New<objects::ReportedPlayer>(
        true);
    report->SetPlayerName(textParams[0]);
    report->SetPlayer(player);
    report->SetLocation(textParams[1]);
    report->SetComment(textParams[2]);
    report->SetSubject((objects::ReportedPlayer::Subject_t)subject);
    report->SetReporter(state->GetAccountUID());
    report->SetReportTime((uint32_t)std::time(0));

    worldDB->QueueInsert(report);

    return true;
}
