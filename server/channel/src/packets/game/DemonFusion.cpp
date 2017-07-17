/**
 * @file server/channel/src/packets/game/DemonFusion.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to fuse a new demon.
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
#include <Packet.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <DemonBox.h>
#include <InheritedSkill.h>
#include <MiAcquisitionData.h>
#include <MiDevilData.h>
#include <MiDCategoryData.h>
#include <MiGrowthData.h>
#include <MiSkillData.h>
#include <MiTriUnionSpecialData.h>
#include <MiUnionData.h>

// channel Includes
#include "ChannelServer.h"
#include "FusionTables.h"

using namespace channel;

bool ProcessDemonFusion(const std::shared_ptr<ChannelServer>& server,
    const std::shared_ptr<ChannelClientConnection>& client,
    int64_t demonID1, int64_t demonID2)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto demon1 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID1)));
    auto demon2 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID2)));

    if(!demon1 || !demon2)
    {
        return false;
    }

    uint32_t demonType1 = demon1->GetType();
    uint32_t demonType2 = demon2->GetType();

    auto def1 = definitionManager->GetDevilData(demonType1);
    auto def2 = definitionManager->GetDevilData(demonType2);
    if(!def1 || !def2)
    {
        return false;
    }

    uint32_t baseDemonType1 = def1->GetUnionData()->GetBaseDemonID();
    uint32_t baseDemonType2 = def2->GetUnionData()->GetBaseDemonID();
    if(baseDemonType1 == baseDemonType2)
    {
        return false;
    }

    std::shared_ptr<objects::MiTriUnionSpecialData> specialFusion;
    auto specialFusions = definitionManager->GetTriUnionSpecialData(demonType1);
    for(auto special : specialFusions)
    {
        auto sourceCount = (special->GetSourceID1() ? 1 : 0) +
            (special->GetSourceID2() ? 1 : 0) +
            (special->GetSourceID3() ? 1 : 0);

        if(sourceCount != 2) continue;

        /// @todo: support plugins
        if(special->GetPluginID() > 0) continue;

        bool match = true;
        for(auto source : { special->GetSourceID1(), special->GetSourceID2(),
            special->GetSourceID3() })
        {
            if(!source) continue;

            auto sourceID = special->GetSourceID1();
            if(sourceID)
            {
                if(special->GetVariant1Allowed())
                {
                    auto specialDef = definitionManager->GetDevilData(sourceID);
                    auto sourceBaseDemonType = specialDef->GetUnionData()
                        ->GetBaseDemonID();
                    if(baseDemonType1 != sourceBaseDemonType &&
                        baseDemonType2 != sourceBaseDemonType)
                    {
                        match = false;
                        break;
                    }
                }
                else if(sourceID != demonType1 && sourceID != demonType2)
                {
                    match = false;
                    break;
                }
            }
        }

        if(match)
        {
            specialFusion = special;
            break;
        }
    }

    uint32_t resultDemonType = 0;
    if(specialFusion)
    {
        resultDemonType = specialFusion->GetResultID();
    }
    else
    {
        uint8_t race1 = def1->GetCategory()->GetRace();
        uint8_t race2 = def2->GetCategory()->GetRace();

        // Get the race axis mappings from the first map row
        size_t race1Idx = 0, race2Idx = 0;
        bool found1 = false, found2 = false;
        for(size_t i = 0; i < 34; i++)
        {
            if(FUSION_RACE_MAP[0][i] == race1)
            {
                race1Idx = i;
                found1 = true;
            }
        
            if(FUSION_RACE_MAP[0][i] == race2)
            {
                race2Idx = i;
                found2 = true;
            }
        }

        if(race1 == 55 || race2 == 55)
        {
            // Elemental source fusion

            if(race1 == race2)
            {
                /// @todo: support mitama result fusions
                return false;
            }

            uint32_t demonType = 0, elementalType = 0;
            uint8_t race = 0;
            size_t raceIdx = 0, elementalIdx = 0;
            bool found = false, elementalFound = false;
            if(race1 == 55)
            {
                elementalType = baseDemonType1;
                demonType = demonType2;
                race = race2;
                raceIdx = race2Idx;
                found = found2;
            }
            else
            {
                elementalType = baseDemonType2;
                demonType = demonType1;
                race = race1;
                raceIdx = race1Idx;
                found = found1;
            }

            for(size_t i = 0; i < 4; i++)
            {
                if(FUSION_ELEMENTAL_TYPES[i] == elementalType)
                {
                    elementalIdx = i;
                    elementalFound = true;
                    break;
                }
            }

            if(!found || !elementalFound ||
                FUSION_ELEMENTAL_ADJUST[raceIdx][elementalIdx] == 0)
            {
                LOG_ERROR(libcomp::String("Invalid elemental fusion request"
                    " of demon IDs  %1 and %2 received from account: %3\n")
                    .Arg(demonType1).Arg(demonType2)
                    .Arg(state->GetAccountUID().ToString()));
                return false;
            }

            bool increase = FUSION_ELEMENTAL_ADJUST[raceIdx][elementalIdx] == 1;
            auto fusionRanges = definitionManager->GetFusionRanges(race);

            // Default to the current demon for up/down fusion at limit already
            resultDemonType = demonType;
            for(auto it = fusionRanges.begin(); it != fusionRanges.end(); it++)
            {
                if(it->second == demonType)
                {
                    if(increase)
                    {
                        it++;
                        if(it != fusionRanges.end())
                        {
                            resultDemonType = it->second;
                        }
                    }
                    else if(it != fusionRanges.begin())
                    {
                        it--;
                        resultDemonType = it->second;
                    }

                    break;
                }
            }
        }
        else
        {
            if(!found1 || !found2)
            {
                LOG_ERROR(libcomp::String("Invalid fusion request of demon IDs"
                    " %1 and %2 received from account: %3\n").Arg(demonType1)
                    .Arg(demonType2).Arg(state->GetAccountUID().ToString()));
                return false;
            }

            // Skip the first map row
            race1Idx++;

            uint8_t resultID = FUSION_RACE_MAP
                [race1Idx][race2Idx];
            if(resultID == 0)
            {
                LOG_ERROR(libcomp::String("Invalid fusion result of demon IDs"
                    " %1 and %2 requested from account: %3\n").Arg(demonType1)
                    .Arg(demonType2).Arg(state->GetAccountUID().ToString()));
                return false;
            }

            if(race1 != race2)
            {
                // Normal 2-way fusion
                auto fusionRanges = definitionManager->GetFusionRanges(resultID);
                if(fusionRanges.size() == 0)
                {
                    LOG_ERROR(libcomp::String("No valid fusion fusion range found"
                        " for race ID: %3\n").Arg(resultID));
                    return false;
                }

                // Find the correct fusion range for the target race
                uint8_t levelSum = (uint8_t)(demon1->GetCoreStats()->GetLevel() +
                    demon2->GetCoreStats()->GetLevel());

                // Traverse the pre-sorted list and stop when the range max
                // is greater than or equal to the level sum
                for(auto pair : fusionRanges)
                {
                    if(pair.first >= levelSum)
                    {
                        resultDemonType = pair.second;
                        break;
                    }
                }

                if(resultDemonType == 0)
                {
                    // Take the last one
                    resultDemonType = fusionRanges.back().second;
                }
            }    
            else
            {
                // Elemental resulting fusion
                resultDemonType = FUSION_ELEMENTAL_TYPES[(size_t)(resultID-1)];
            }
        }
    }

    if(resultDemonType)
    {
        // Result demon identified, check success rate, pay cost and fuse

        auto demonData = definitionManager->GetDevilData(resultDemonType);
        if(!demonData)
        {
            LOG_ERROR(libcomp::String("Fusion result demon ID does not"
                " exist: %1\n").Arg(resultDemonType));
            return false;
        }

        uint8_t baseLevel = demonData->GetGrowth()->GetBaseLevel();
        uint8_t difficulty = demonData->GetUnionData()->GetFusionDifficulty();
        double successRate = ceil((140 - (difficulty * 2.5)) +
            (difficulty - baseLevel * 1.5) + (difficulty * 0.5));

        /// @todo: calculate and pay costs

        /// @todo: figure out fusion success adjustments from familiarity

        /// @todo: use/adjust success, then remove this
        LOG_WARNING(libcomp::String("Success rate: %1\n").Arg(successRate));

        // Put either demon back in the COMP if they're out
        if(dState->GetEntity() == demon1 ||
            dState->GetEntity() == demon2)
        {
            characterManager->StoreDemon(client);
        }

        // Create the new demon
        auto newDemon = characterManager->GenerateDemon(demonData);
        
        auto inheritRestrictions = demonData->GetGrowth()->
            GetInheritanceRestrictions();
        std::map<uint32_t, std::shared_ptr<objects::MiSkillData>> inherited;
        for(auto source : { demon1, demon2 })
        {
            for(auto learned : source->GetLearnedSkills())
            {
                if(learned == 0) continue;

                auto lData = definitionManager->GetSkillData(learned);

                // Check inheritance flags for valid skills
                auto r = lData->GetAcquisition()->GetInheritanceRestriction();
                if((inheritRestrictions & (uint16_t)(1 << r)) == 0) continue;

                inherited[learned] = lData;
            }
        }

        // Remove skills the result demon already knows
        for(auto skillID : demonData->GetGrowth()->GetSkills())
        {
            inherited.erase(skillID);
        }

        // Correct the COMP
        auto comp = character->GetCOMP().Get();

        newDemon->SetDemonBox(comp);
        newDemon->SetBoxSlot(demon1->GetBoxSlot());
        comp->SetDemons((size_t)demon1->GetBoxSlot(), newDemon);

        // Prepare the updates and generate the inherited skills
        auto changes = libcomp::DatabaseChangeSet::Create(
            character->GetAccount().GetUUID());
        changes->Insert(newDemon);
        changes->Insert(newDemon->GetCoreStats().Get());

        for(auto iPair : inherited)
        {
            /// @todo: get confirmed inheritance percentages,
            /// double if both sources have it
            auto iSkill = libcomp::PersistentObject::New<
                objects::InheritedSkill>(true);
            iSkill->SetSkill(iPair.first);
            iSkill->SetProgress(10000);
            iSkill->SetDemon(newDemon);
            newDemon->AppendInheritedSkills(iSkill);

            changes->Insert(iSkill);
        }

        // Tell the client it succeeded
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_FUSION);
        reply.WriteS32Little(0);    // No error
        reply.WriteU32Little(resultDemonType);

        client->QueuePacket(reply);

        state->SetObjectID(newDemon->GetUUID(),
            server->GetNextObjectID());

        changes->Update(comp);
        characterManager->DeleteDemon(demon1, changes);
        characterManager->DeleteDemon(demon2, changes);

        // Send the new COMP slot info
        for(int8_t slot : { demon2->GetBoxSlot(), newDemon->GetBoxSlot() })
        {
            characterManager->SendDemonBoxData(client, 0, { slot });
        }

        server->GetWorldDatabase()->QueueChangeSet(changes);
    }

    return false;
}

void HandleDemonFusion(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int64_t demonID1, int64_t demonID2)
{
    if(!ProcessDemonFusion(server, client, demonID1, demonID2))
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_FUSION);
        reply.WriteS32Little(1);    // Failed

        client->SendPacket(reply);
    }
}

bool Parsers::DemonFusion::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 28)
    {
        return false;
    }

    int32_t fusionType = p.ReadS32Little();
    int64_t demonID1 = p.ReadS64Little();
    int64_t demonID2 = p.ReadS64Little();
    int64_t unknown = p.ReadS64Little();
    (void)fusionType;
    (void)unknown;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    server->QueueWork(HandleDemonFusion, server, client, demonID1, demonID2);

    return true;
}
