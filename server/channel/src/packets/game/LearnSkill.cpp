/**
 * @file server/channel/src/packets/game/LearnSkill.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for a character to learn a skill.
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
#include <MiExpertChainData.h>
#include <MiExpertClassData.h>
#include <MiExpertData.h>
#include <MiExpertRankData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::LearnSkill::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    int32_t entityID = p.ReadS32Little();
    uint32_t skillID = p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    if(cState->GetEntityID() != entityID)
    {
        LogSkillManagerError([&]()
        {
            return libcomp::String("Attempted to learn a skill on an entity"
                " that is not the current character: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });

        client->Close();
        return true;
    }

    // Make sure the skill being learned is on an expertise the character
    // can learn
    bool valid = false;

    uint32_t maxExpertise = (uint32_t)(EXPERTISE_COUNT + CHAIN_EXPERTISE_COUNT);
    for(uint32_t i = 0; i < maxExpertise && !valid; i++)
    {
        auto expertData = definitionManager->GetExpertClassData(i);

        if(!expertData) continue;

        uint32_t currentRank = cState->GetExpertiseRank(i, definitionManager);

        uint32_t rank = 0;
        for(auto classData : expertData->GetClassData())
        {
            for(auto rankData : classData->GetRankData())
            {
                if(rank > currentRank) break;

                for(uint32_t eSkillID : rankData->GetSkill())
                {
                    if(eSkillID && skillID == eSkillID)
                    {
                        valid = true;
                        break;
                    }
                }

                if(valid)
                {
                    // Make sure the max skill count is not exceeded
                    auto character = cState->GetEntity();

                    size_t count = 0;
                    for(uint32_t eSkillID : rankData->GetSkill())
                    {
                        if(eSkillID &&
                            character->LearnedSkillsContains(eSkillID))
                        {
                            count++;
                        }
                    }

                    if(count >= (size_t)rankData->GetSkillCount())
                    {
                        valid = false;
                    }
                    else
                    {
                        break;
                    }
                }

                rank++;
            }

            if(valid) break;
        }
    }

    if(valid)
    {
        server->GetCharacterManager()->LearnSkill(client, entityID, skillID);
    }
    else
    {
        LogSkillManagerError([&]()
        {
            return libcomp::String("Attempted to learn a skill not available"
                " to the current character's expertise ranks: %1\n")
                .Arg(state->GetAccountUID().ToString());
        });
    }

    return true;
}
