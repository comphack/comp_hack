/**
 * @file server/channel/src/packets/game/QuestTitle.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to obtain a quest bonus title.
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
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Character.h>
#include <CharacterProgress.h>
#include <MiQuestBonusCodeData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::QuestTitle::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    uint32_t bonusID = p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    auto bonusData = definitionManager->GetQuestBonusCodeData(bonusID);
    if(!bonusData ||
        (bonusData->GetTitleID() >= (int32_t)(progress->SpecialTitlesCount() * 8)))
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Invalid quest bonus ID supplied"
                " for QuestTitle request: %1\n").Arg(bonusID);
        });
    }
    else if((uint32_t)bonusData->GetCount() > cState->GetQuestBonusCount())
    {
        LogGeneralError([&]()
        {
            return libcomp::String("QuestTitle request encountered for"
                " a quest bonus count that has not been obtained: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });
    }
    else
    {
        int16_t titleID = (int16_t)bonusData->GetTitleID();
        characterManager->AddTitle(client, titleID);
    }

    return true;
}
