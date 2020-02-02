/**
 * @file server/channel/src/packets/game/SkillTarget.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to target/retarget a skill being used.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

// channel Includes
#include "ChannelServer.h"
#include "SkillManager.h"

using namespace channel;

bool Parsers::SkillTarget::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8 && p.Size() != 12)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto skillManager = server->GetSkillManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();

    int32_t sourceEntityID = p.ReadS32Little();
    int64_t targetObjectID = p.Size() == 8 ? (int64_t)p.ReadS32Little() : p.ReadS64Little();

    auto source = state->GetEntityState(sourceEntityID);
    if(!source)
    {
        LogSkillManagerErrorMsg("Invalid skill source sent from client for "
            "skill target\n");

        return false;
    }

    server->QueueWork([](SkillManager* pSkillManager, const std::shared_ptr<
        ActiveEntityState> pSource, int64_t pTargetObjectID)
        {
            pSkillManager->TargetSkill(pSource, pTargetObjectID);
        }, skillManager, source, targetObjectID);

    return true;
}
