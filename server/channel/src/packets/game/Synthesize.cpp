/**
 * @file server/channel/src/packets/game/Synthesize.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to synthesize a weapon from the
 *  contents of the material tank.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// object Includes
#include <ActivatedAbility.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiSynthesisData.h>
#include <MiSynthesisItemData.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::Synthesize::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 0)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto exchangeSession = state->GetExchangeSession();

    auto catalyst = exchangeSession
        ? exchangeSession->GetItems(0).Get() : nullptr;
    auto synthData = exchangeSession
        ? definitionManager->GetSynthesisData(exchangeSession->GetSelectionID())
        : nullptr;

    int16_t successRate = 0;
    auto freeSlots = characterManager->GetFreeSlots(client);

    bool success = freeSlots.size() >= 1 && synthData &&
        cState->CurrentSkillsContains(synthData->GetSkillID()) &&
        cState->CurrentSkillsContains(synthData->GetBaseSkillID());
    if(success)
    {
        success = true;

        if(catalyst)
        {
            int8_t catalystIdx = -1;

            auto it = SVR_CONST.RATE_SCALING_ITEMS[3].begin();
            for(size_t i = 0; i < SVR_CONST.RATE_SCALING_ITEMS[3].size(); i++)
            {
                if(*it == catalyst->GetType())
                {
                    catalystIdx = (int8_t)(i + 1);
                    break;
                }

                it++;
            }

            if(catalystIdx != -1)
            {
                successRate = synthData->GetRateScaling((size_t)catalystIdx);
            }
            else
            {
                success = false;
            }
        }
        else
        {
            successRate = synthData->GetRateScaling(0);
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SYNTHESIZE);

    if(success)
    {
        // Materials get paid no matter what so prepare the change for
        // them now
        auto character = cState->GetEntity();
        auto materials = character->GetMaterials();
        
        std::set<uint32_t> materialIDs;
        for(auto mat : synthData->GetMaterials())
        {
            uint32_t materialID = mat->GetItemID();
            if(!materialID) continue;

            auto matIter = materials.find(materialID);
            if(matIter == materials.end() ||
                matIter->second < mat->GetAmount())
            {
                LOG_ERROR(libcomp::String("Synthesize attampted"
                    " without the necessary materials: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                success = false;
                break;
            }
            else
            {
                materials[materialID] = (uint16_t)(materials[materialID] -
                    mat->GetAmount());
                materialIDs.insert(materialID);
            }
        }

        // If we haven't failed yet determine if the item will
        // be synthesized
        uint8_t modSlotCount = 0;
        if(success)
        {
            if(successRate >= 10000 || (successRate > 0 &&
                RNG(int32_t, 1, 10000) <= successRate))
            {
                if(synthData->GetSlotMax() != 0)
                {
                    // Calculate "sliding" weighted slot determination
                    // As the player's expertise and related stats raise the
                    // ability to create more slots unlocks and the chance to
                    // create less slots diminishes

                    const uint16_t weights[] = { 400, 225, 150, 100, 75, 50 };

                    uint16_t min = 0;
                    uint16_t max = 800;

                    auto dState = state->GetDemonState();

                    uint8_t expertRank = 0;
                    switch(exchangeSession->GetType())
                    {
                    case objects::PlayerExchangeSession::Type_t::SYNTH_MELEE:
                        expertRank = cState->GetExpertiseRank(definitionManager,
                            EXPERTISE_CHAIN_SWORDSMITH);
                        for(auto pair : SVR_CONST.SYNTH_ADJUSTMENTS)
                        {
                            if(pair.second[0] == 2 &&
                                dState->CurrentSkillsContains((uint32_t)pair.first))
                            {
                                // Boost on vit and luck
                                uint16_t vitBoost = (uint16_t)(
                                    dState->GetVIT() / pair.second[1]);
                                uint16_t luckBoost = (uint16_t)(
                                    dState->GetLUCK() / pair.second[2]);

                                min = (uint16_t)(min + vitBoost + luckBoost);
                            }
                        }
                        break;
                    case objects::PlayerExchangeSession::Type_t::SYNTH_GUN:
                        expertRank = cState->GetExpertiseRank(definitionManager,
                            EXPERTISE_CHAIN_ARMS_MAKER);
                        for(auto pair : SVR_CONST.SYNTH_ADJUSTMENTS)
                        {
                            if(pair.second[0] == 3 &&
                                dState->CurrentSkillsContains((uint32_t)pair.first))
                            {
                                // Boost on int and luck
                                uint16_t intBoost = (uint16_t)(
                                    dState->GetINTEL() / pair.second[1]);
                                uint16_t luckBoost = (uint16_t)(
                                    dState->GetLUCK() / pair.second[2]);

                                min = (uint16_t)(min + intBoost + luckBoost);
                            }
                        }
                        break;
                    default:
                        break;
                    }

                    min = (uint16_t)(min + expertRank * 8);

                    // "Slide" max by the increase to min and limit
                    max = (uint16_t)(max + min);
                    if(max > 1000)
                    {
                        max = 1000;
                    }

                    uint16_t val = (min >= max) ? max
                        : RNG(uint16_t, min, max);

                    for(size_t i = 0; i < 6; i++)
                    {
                        if(weights[i] >= val)
                        {
                            modSlotCount = (uint8_t)i;
                            break;
                        }

                        val = (uint16_t)(val - weights[i]);
                    }

                    // Limit max
                    if(modSlotCount > synthData->GetSlotMax())
                    {
                        modSlotCount = synthData->GetSlotMax();
                    }
                }
            }
            else
            {
                success = false;
            }
        }

        // Boost the skill execution expertise growth rate based upon
        // success or failure
        auto activated = cState->GetActivatedAbility();
        if(activated &&
            activated->GetSkillID() == synthData->GetBaseSkillID())
        {
            activated->SetExpertiseBoost(synthData->GetExpertBoosts(
                success ? 1 : 0));
        }

        reply.WriteS32Little(0);

        client->QueuePacket(reply);

        // Consume materials
        character->SetMaterials(materials);

        characterManager->SendMaterials(client, materialIDs);

        // Consume catlyst
        if(catalyst)
        {
            std::unordered_map<uint32_t, uint32_t> itemMap;
            itemMap[catalyst->GetType()] = 1;
            characterManager->AddRemoveItems(client, itemMap, false);
        }

        auto dbChanges = libcomp::DatabaseChangeSet::Create(
            state->GetAccountUID());

        // Generate the item if successful
        if(success)
        {
            auto item = characterManager->GenerateItem(synthData->GetItemID(),
                synthData->GetCount());

            for(uint8_t i = 0; i < 5; i++)
            {
                item->SetModSlots((size_t)i,
                    i < modSlotCount ? MOD_SLOT_NULL_EFFECT : 0);
            }

            auto inventory = character->GetItemBoxes(0).Get();
            size_t slot = *freeSlots.begin();

            item->SetBoxSlot((int8_t)slot);
            item->SetItemBox(inventory->GetUUID());
            inventory->SetItems(slot, item);

            dbChanges->Update(inventory);
            dbChanges->Insert(item);

            characterManager->SendItemBoxData(client, inventory,
                { (uint16_t)slot });
        }

        dbChanges->Update(character);

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);

        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_SYNTHESIZED);
        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteS32Little(success ? 0 : -1);

        server->GetZoneManager()->BroadcastPacket(client, notify);
    }
    else
    {
        reply.WriteS32Little(-1);   // Error

        client->SendPacket(reply);
    }

    characterManager->EndExchange(client);

    return true;
}
