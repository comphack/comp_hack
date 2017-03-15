/**
 * @file server/channel/src/packets/game/CancelSkill.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to cancel a skill that was activated.
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

// channel Includes
#include "ChannelServer.h"

using namespace channel;

void SkillCancellation(SkillManager* skillManager,
    const std::shared_ptr<ChannelClientConnection> client,
    int32_t sourceEntityID, uint8_t activationID)
{
    skillManager->CancelSkill(client, sourceEntityID, activationID);
}

bool Parsers::CancelSkill::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 5)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto skillManager = server->GetSkillManager();

    int32_t sourceEntityID = p.ReadS32Little();
    uint8_t activationID = p.ReadU8();

    server->QueueWork(SkillCancellation, skillManager, client, sourceEntityID, activationID);

    return true;
}
