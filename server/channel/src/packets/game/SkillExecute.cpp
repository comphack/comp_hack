/**
 * @file server/channel/src/packets/game/SkillExecute.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to execute a skill that has charged.
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

// channel Includes
#include "ChannelServer.h"
#include "SkillManager.h"

using namespace channel;

bool Parsers::SkillExecute::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 9)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto skillManager = server->GetSkillManager();

    int32_t sourceEntityID = p.ReadS32Little();
    int8_t activationID = p.ReadS8();
    int64_t targetObjectID = p.Size() == 9 ? (int64_t)p.ReadS32Little() : p.ReadS64Little();

    // Load the player entity and let the processer handle it not being ready
    auto source = state->GetEntityState(sourceEntityID, false);
    if(!source)
    {
        LogSkillManagerError([&]()
        {
            return libcomp::String("Invalid skill source sent from client for"
                " skill execution: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        client->Close();
        return true;
    }

    server->QueueWork([](SkillManager* pSkillManager, const std::shared_ptr<
        ActiveEntityState> pSource, int8_t pActivationID, int64_t pTargetObjectID)
        {
            pSkillManager->ExecuteSkill(pSource, pActivationID, pTargetObjectID);
        }, skillManager, source, activationID, targetObjectID);

    return true;
}
