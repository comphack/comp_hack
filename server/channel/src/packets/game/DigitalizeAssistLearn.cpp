/**
 * @file server/channel/src/packets/game/DigitalizeAssistLearn.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to learn a digitalize assist skill.
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

// object Includes
#include <CharacterProgress.h>
#include <MiGuardianAssistData.h>
#include <MiGuardianLevelData.h>
#include <MiGuardianLevelDataEntry.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::DigitalizeAssistLearn::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    int32_t unknown = p.ReadS32Little();
    (void)unknown;  // Always 0

    uint32_t assistID = p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character ? character->GetProgress().Get() : nullptr;

    bool success = false;
    if(progress)
    {
        // Check that the player has access to the assist skill
        auto assistData = definitionManager->GetGuardianAssistData(assistID);
        auto levelData = assistData ? definitionManager->GetGuardianLevelData(
            assistData->GetRaceID()) : nullptr;
        if(levelData)
        {
            uint8_t lvl = (uint8_t)progress->GetDigitalizeLevels(assistData
                ->GetRaceID());
            for(uint8_t i = 1; i <= lvl; i++)
            {
                for(uint32_t aID : levelData->GetLevels(i)->GetAssists())
                {
                    if(aID == assistID)
                    {
                        success = true;
                        break;
                    }
                }
            }
        }
    }

    if(success)
    {
        size_t index;
        uint8_t shiftVal;
        CharacterManager::ConvertIDToMaskValues((uint16_t)assistID, index,
            shiftVal);
        if(index < progress->DigitalizeAssistsCount())
        {
            auto oldValue = progress->GetDigitalizeAssists(index);
            uint8_t newValue = static_cast<uint8_t>(oldValue | shiftVal);

            if(oldValue != newValue)
            {
                progress->SetDigitalizeAssists((size_t)index, newValue);

                server->GetWorldDatabase()->QueueUpdate(progress);
            }
        }
        else
        {
            success = false;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_DIGITALIZE_ASSIST_LEARN);
    reply.WriteS32Little(0);    // Unknown
    reply.WriteS32Little(success ? 0 : -1);
    reply.WriteU32Little(assistID);

    client->SendPacket(reply);

    return true;
}
