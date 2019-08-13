/**
 * @file server/channel/src/FusionManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager class used to handle all demon fusion based actions.
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

#include "FusionManager.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Account.h>
#include <Character.h>
#include <CharacterProgress.h>
#include <Demon.h>
#include <DemonBox.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiAcquisitionData.h>
#include <MiDCategoryData.h>
#include <MiDevilCrystalData.h>
#include <MiDevilData.h>
#include <MiEnchantData.h>
#include <MiGrowthData.h>
#include <MiNPCBasicData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiTriUnionSpecialData.h>
#include <MiUnionData.h>
#include <TriFusionHostSession.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "FusionTables.h"

using namespace channel;

FusionManager::FusionManager(const std::weak_ptr<
    ChannelServer>& server) : mServer(server)
{
}

FusionManager::~FusionManager()
{
}

bool FusionManager::HandleFusion(
    const std::shared_ptr<ChannelClientConnection>& client,
    int64_t demonID1, int64_t demonID2, uint32_t costItemType)
{
    std::shared_ptr<objects::Demon> resultDemon;
    int8_t result = ProcessFusion(client, demonID1, demonID2, -1,
        costItemType, resultDemon);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_FUSION);
    reply.WriteS32Little(result == 0 ? 0 : 1);
    reply.WriteU32Little(resultDemon ? resultDemon->GetType() : 0);

    client->SendPacket(reply);

    return result == 0;
}

bool FusionManager::HandleTriFusion(
    const std::shared_ptr<ChannelClientConnection>& client,
    int64_t demonID1, int64_t demonID2, int64_t demonID3, bool soloFusion)
{
    // Pull the demons involved local for use in notifications as they will
    // be deleted upon success
    auto state = client->GetClientState();
    auto demon1 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID1)));
    auto demon2 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID2)));
    auto demon3 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID3)));

    auto server = mServer.lock();
    auto managerConnection = server->GetManagerConnection();

    std::unordered_map<std::shared_ptr<objects::Demon>,
        std::shared_ptr<ChannelClientConnection>> dClientMap;
    for(auto demon : { demon1, demon2, demon3 })
    {
        if(demon)
        {
            auto dBox = std::dynamic_pointer_cast<objects::DemonBox>(
                libcomp::PersistentObject::GetObjectByUUID(demon->GetDemonBox()));
            auto account = dBox ? std::dynamic_pointer_cast<objects::Account>(
                libcomp::PersistentObject::GetObjectByUUID(dBox->GetAccount()))
                : nullptr;
            auto dClient = account ? managerConnection->GetClientConnection(
                account->GetUsername()) : nullptr;
            if(dClient)
            {
                dClientMap[demon] = dClient;
            }
        }
    }

    uint32_t costItemType = soloFusion
        ? SVR_CONST.ITEM_KREUZ : SVR_CONST.ITEM_MACCA;

    std::shared_ptr<objects::Demon> resultDemon;
    int8_t result = ProcessFusion(client, demonID1, demonID2, demonID3,
        costItemType, resultDemon);

    if(soloFusion)
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRIFUSION_SOLO);
        reply.WriteS8(result == 0 ? 0 : 1);
        reply.WriteU32Little(resultDemon ? resultDemon->GetType() : 0);

        client->SendPacket(reply);
    }
    else
    {
        auto cState = state->GetCharacterState();
        auto tfSession = std::dynamic_pointer_cast<
            objects::TriFusionHostSession>(state->GetExchangeSession());

        if(!tfSession)
        {
            // Weird but not an error
            return true;
        }

        std::set<int32_t> participantIDs;
        participantIDs.insert(tfSession->GetSourceEntityID());
        for(auto pState : tfSession->GetGuests())
        {
            participantIDs.insert(pState->GetEntityID());
        }

        std::list<std::shared_ptr<ChannelClientConnection>> pClients;
        for(int32_t pID : participantIDs)
        {
            auto pClient = managerConnection->GetEntityClient(pID,
                false);
            if(pClient)
            {
                pClients.push_back(pClient);
            }
        }

        libcomp::Packet notify;
        notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRIFUSION);
        notify.WriteS8(result == 0 ? 0 : 1);
        notify.WriteU32Little(result == 0 ? resultDemon->GetType() : 0);
        notify.WriteU32Little(static_cast<uint32_t>(-1));   // Unknown

        ChannelClientConnection::BroadcastPacket(pClients, notify);

        if(result == 0)
        {
            // Handle crystals and rewards
            auto characterManager = server->GetCharacterManager();
            auto definitionManager = server->GetDefinitionManager();

            std::list<uint16_t> updatedSourceSlots;
            std::unordered_map<std::shared_ptr<ChannelClientConnection>,
                std::set<size_t>> freeSlots;
            std::unordered_map<std::shared_ptr<ChannelClientConnection>,
                std::unordered_map<int8_t, std::shared_ptr<objects::Item>>> newItemMap;
            for(auto pClient : pClients)
            {
                auto pState = pClient->GetClientState();
                freeSlots[pClient] = characterManager->GetFreeSlots(pClient);

                auto exchange = pState->GetExchangeSession();

                if(!exchange) continue;

                for(size_t i = 0; i < 4; i++)
                {
                    // Add the items to the first available slots (do not combine)
                    auto item = exchange->GetItems(i).Get();
                    if(item && freeSlots[pClient].size() > 0)
                    {
                        size_t slot = *freeSlots[pClient].begin();
                        freeSlots[pClient].erase(slot);

                        updatedSourceSlots.push_back((uint16_t)item->GetBoxSlot());
                        newItemMap[pClient][(int8_t)slot] = item;
                    }
                }
            }

            for(auto dPair : dClientMap)
            {
                // Every player has a flat 10% chance of getting their
                // demon back as a crystal
                auto pClient = dPair.second;
                auto dEnchantData = definitionManager->GetEnchantDataByDemonID(
                    dPair.first->GetType());
                if(freeSlots[pClient].size() > 0 && dEnchantData && RNG(int16_t, 1, 10) == 1)
                {
                    uint32_t crystalItem = dEnchantData->GetDevilCrystal()
                        ->GetItemID();
                    if(crystalItem)
                    {
                        auto crystal = characterManager->GenerateItem(crystalItem, 1);

                        size_t slot = *freeSlots[pClient].begin();
                        freeSlots[pClient].erase(slot);
                        newItemMap[pClient][(int8_t)slot] = crystal;

                        notify.Clear();
                        notify.WritePacketCode(
                            ChannelToClientPacketCode_t::PACKET_TRIFUSION_DEMON_CRYSTAL);
                        notify.WriteU32Little(crystalItem);

                        pClient->QueuePacket(notify);
                    }
                }
            }

            if(newItemMap.size() > 0)
            {
                // Items updated
                std::unordered_map<std::shared_ptr<ChannelClientConnection>,
                    std::list<uint16_t>> updatedSlots;

                auto changes = libcomp::DatabaseChangeSet::Create(
                    state->GetAccountUID());
                for(auto clientPair : freeSlots)
                {
                    auto pClient = clientPair.first;

                    // Skip the source first
                    if(pClient == client)
                    {
                        updatedSlots[pClient] = updatedSourceSlots;
                    }

                    if(newItemMap[pClient].size() > 0)
                    {
                        auto pCharacter = pClient->GetClientState()
                            ->GetCharacterState()->GetEntity();
                        auto targetBox = pCharacter->GetItemBoxes(0).Get();
                        for(auto itemPair : newItemMap[pClient])
                        {
                            auto item = itemPair.second;
                            auto sourceBox = std::dynamic_pointer_cast<objects::ItemBox>(
                                libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
                            if(sourceBox)
                            {
                                characterManager->UnequipItem(client, item);
                                sourceBox->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
                                changes->Update(item);
                                changes->Update(sourceBox);
                            }
                            else
                            {
                                changes->Insert(item);
                            }

                            changes->Update(targetBox);

                            item->SetBoxSlot(itemPair.first);
                            item->SetItemBox(targetBox->GetUUID());
                            targetBox->SetItems((size_t)itemPair.first, item);
                            updatedSlots[pClient].push_back((uint16_t)itemPair.first);
                        }
                    }
                }

                if(!server->GetWorldDatabase()->ProcessChangeSet(changes))
                {
                    LogFusionManagerError([&]()
                    {
                        return libcomp::String("TriFusion items failed to save "
                        "for account '%1'. Disconnecting all participants to "
                        "avoid additional errors.\n")
                        .Arg(state->GetAccountUID().ToString());
                    });

                    for(auto pClient : pClients)
                    {
                        pClient->Kill();
                    }

                    return false;
                }

                // Now send the updates
                for(auto updatePair : updatedSlots)
                {
                    auto pCharacter = updatePair.first->GetClientState()
                        ->GetCharacterState()->GetEntity();
                    auto box = pCharacter->GetItemBoxes(0).Get();
                    characterManager->SendItemBoxData(updatePair.first, box,
                        updatePair.second);
                }
            }

            // Update the demons available in case another fusion is chained
            auto cs = resultDemon->GetCoreStats().Get();

            std::unordered_map<uint32_t, std::shared_ptr<objects::Demon>> dMap;

            notify.Clear();
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TRIFUSION_UPDATE);
            notify.WriteS32Little(cState->GetEntityID());

            dMap[notify.Size()] = resultDemon;
            notify.WriteS64Little(0);

            notify.WriteU32Little(resultDemon->GetType());
            notify.WriteS8(cs ? cs->GetLevel() : 0);
            notify.WriteU16Little(resultDemon->GetFamiliarity());

            std::list<uint32_t> skillIDs;
            for(uint32_t skillID : resultDemon->GetLearnedSkills())
            {
                if(skillID != 0)
                {
                    skillIDs.push_back(skillID);
                }
            }

            notify.WriteS8((int8_t)skillIDs.size());
            for(uint32_t skillID : skillIDs)
            {
                notify.WriteU32Little(skillID);
            }

            // Write removed demons
            for(auto d : { demon1, demon2, demon3 })
            {
                auto dBox = std::dynamic_pointer_cast<objects::DemonBox>(
                    libcomp::PersistentObject::GetObjectByUUID(d->GetDemonBox()));
                auto c = dBox ? std::dynamic_pointer_cast<objects::Character>(
                    libcomp::PersistentObject::GetObjectByUUID(dBox->GetCharacter()))
                    : nullptr;

                int32_t ownerEntityID = 0;
                for(auto pClient : pClients)
                {
                    auto pCState = pClient->GetClientState()->GetCharacterState();
                    if(pCState->GetEntity() == c)
                    {
                        ownerEntityID = pCState->GetEntityID();
                    }
                }

                notify.WriteS32Little(ownerEntityID);
                dMap[notify.Size()] = d;
                notify.WriteS64Little(0);
            }

            // Create a copy for each participant with local object IDs
            for(auto pClient : pClients)
            {
                auto pState = pClient->GetClientState();

                libcomp::Packet nCopy(notify);
                for(auto dPair : dMap)
                {
                    auto d = dPair.second;

                    int64_t objID = pState->GetObjectID(d->GetUUID());
                    if(objID <= 0)
                    {
                        objID = server->GetNextObjectID();
                        pState->SetObjectID(d->GetUUID(), objID);
                    }

                    nCopy.Seek(dPair.first);
                    nCopy.WriteS64Little(objID);
                }

                pClient->SendPacket(nCopy);
            }
        }
    }

    return result == 0;
}

uint32_t FusionManager::GetResultDemon(const std::shared_ptr<
    ChannelClientConnection>& client, int64_t demonID1, int64_t demonID2,
    int64_t demonID3)
{
    bool triFusion = demonID3 > 0;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto demon1 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID1)));
    auto demon2 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID2)));
    auto demon3 = demonID3 > 0 ? std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID3)))
        : nullptr;

    // Fail if any demon is missing or the same one is supplied twice for a
    // normal fusion
    if(!demon1 || !demon2 || (triFusion && !demon3) ||
        (!triFusion && demon1 == demon2))
    {
        return 0;
    }

    uint32_t demonType1 = demon1->GetType();
    uint32_t demonType2 = demon2->GetType();
    uint32_t demonType3 = demon3 ? demon3->GetType() : 0;

    auto def1 = std::pair<uint8_t, std::shared_ptr<objects::MiDevilData>>(
            (uint8_t)demon1->GetCoreStats()->GetLevel(),
            definitionManager->GetDevilData(demonType1));
    auto def2 = std::pair<uint8_t, std::shared_ptr<objects::MiDevilData>>(
            (uint8_t)demon2->GetCoreStats()->GetLevel(),
            definitionManager->GetDevilData(demonType2));
    auto def3 = std::pair<uint8_t, std::shared_ptr<objects::MiDevilData>>(
            demon3 ? (uint8_t)demon3->GetCoreStats()->GetLevel() : 0,
            demonType3 ? definitionManager->GetDevilData(demonType3) : nullptr);
    if(!def1.second || !def2.second || (triFusion && !def3.second))
    {
        return 0;
    }

    uint32_t baseDemonType1 = def1.second->GetUnionData()->GetBaseDemonID();
    uint32_t baseDemonType2 = def2.second->GetUnionData()->GetBaseDemonID();
    uint32_t baseDemonType3 = def3.second
        ? def3.second->GetUnionData()->GetBaseDemonID() : 0;

    auto specialFusions = definitionManager->GetTriUnionSpecialData(demonType1);
    for(auto special : specialFusions)
    {
        if(triFusion != (special->GetSourceID3() > 0)) continue;

        // Map of source ID to its "variant allowed" value
        std::array<std::pair<uint32_t, bool>, 3> sources;
        sources[0] = std::pair<uint32_t, bool>(special->GetSourceID1(),
            special->GetVariant1Allowed() == 1);
        sources[1] = std::pair<uint32_t, bool>(special->GetSourceID2(),
            special->GetVariant2Allowed() == 1);
        sources[2] = std::pair<uint32_t, bool>(special->GetSourceID3(),
            special->GetVariant3Allowed() == 1);

        // Store each demon number that matches the corresponding source
        std::array<std::set<uint8_t>, 3> matches;

        bool match = true;
        for(size_t i = 0; i < 3; i++)
        {
            std::pair<uint32_t, bool>& sourcePair = sources[i];

            uint32_t sourceID = sourcePair.first;
            if(!sourceID) continue;

            if(sourcePair.second)
            {
                // Match against base demon
                auto specialDef = definitionManager->GetDevilData(sourceID);
                auto sourceBaseDemonType = specialDef->GetUnionData()
                    ->GetBaseDemonID();
                if(baseDemonType1 == sourceBaseDemonType)
                {
                    matches[i].insert(1);
                }

                if(baseDemonType2 == sourceBaseDemonType)
                {
                    matches[i].insert(2);
                }

                if(baseDemonType3 == sourceBaseDemonType)
                {
                    matches[i].insert(3);
                }
            }
            else
            {
                // Match against exact demon
                if(demonType1 == sourceID)
                {
                    matches[i].insert(1);
                }

                if(demonType2 == sourceID)
                {
                    matches[i].insert(2);
                }

                if(demonType3 == sourceID)
                {
                    matches[i].insert(3);
                }
            }

            if(matches[i].size() == 0)
            {
                // No match found for the current source demon
                match = false;
                break;
            }
        }

        if(match)
        {
            // If one of each required type is found, check to make sure
            // there is a valid combination available (vital when fusing
            // two of the same type or a variant and a specific demon with
            // the same base type)
            match = false;
            for(uint8_t m1 : matches[0])
            {
                for(uint8_t m2 : matches[1])
                {
                    if(triFusion)
                    {
                        for(uint8_t m3 : matches[2])
                        {
                            if(m1 != m2 && m1 != m3 && m2 != m3)
                            {
                                match = true;
                                break;
                            }
                        }

                        if(match) break;
                    }
                    else if(m1 != m2)
                    {
                        match = true;
                        break;
                    }
                }

                if(match) break;
            }
        }

        if(match && special->GetPluginID() > 0)
        {
            // Check that the player has the plugin
            size_t index;
            uint8_t shiftVal;
            CharacterManager::ConvertIDToMaskValues(
                (uint16_t)special->GetPluginID(), index, shiftVal);

            uint8_t indexVal = character->GetProgress()->GetPlugins(index);

            match = (indexVal & shiftVal) != 0;
        }

        if(match)
        {
            return special->GetResultID();
        }
    }

    const uint8_t eRace = (uint8_t)objects::MiDCategoryData::Race_t::ELEMENTAL;
    const uint8_t mRace = (uint8_t)objects::MiDCategoryData::Race_t::MITAMA;

    if(triFusion)
    {
        // Sort by level and priority for logic purposes
        std::list<std::pair<uint8_t,
            std::shared_ptr<objects::MiDevilData>>> defs = { def1, def2, def3 };
        defs.sort([](auto& a, auto& b)
            {
                if(a.second->GetGrowth()->GetBaseLevel() !=
                    b.second->GetGrowth()->GetBaseLevel())
                {
                    // Higher base level first
                    return a.second->GetGrowth()->GetBaseLevel() >
                        b.second->GetGrowth()->GetBaseLevel();
                }
                else
                {
                    // Higher priority first
                    uint8_t ra = (uint8_t)a.second->GetCategory()->GetRace();
                    uint8_t rb = (uint8_t)b.second->GetCategory()->GetRace();

                    size_t pa = 0;
                    size_t pb = 0;

                    for(size_t i = 0; i < 34; i++)
                    {
                        if(TRIFUSION_RACE_PRIORITY[i] == ra)
                        {
                            break;
                        }

                        pa++;
                    }

                    for(size_t i = 0; i < 34; i++)
                    {
                        if(TRIFUSION_RACE_PRIORITY[i] == rb)
                        {
                            break;
                        }

                        pb++;
                    }

                    return pa < pb;
                }
            });

        def1 = defs.front();
        defs.pop_front();
        def2 = defs.front();
        defs.pop_front();
        def3 = defs.front();
        defs.pop_front();

        uint8_t f1 = (uint8_t)def1.second->GetCategory()->GetFamily();
        uint8_t f2 = (uint8_t)def2.second->GetCategory()->GetFamily();
        uint8_t f3 = (uint8_t)def3.second->GetCategory()->GetFamily();

        uint8_t race1 = (uint8_t)def1.second->GetCategory()->GetRace();
        uint8_t race2 = (uint8_t)def2.second->GetCategory()->GetRace();
        uint8_t race3 = (uint8_t)def3.second->GetCategory()->GetRace();

        std::shared_ptr<objects::MiDevilData> resultDef;

        const uint8_t eFam = (uint8_t)objects::MiDCategoryData::Family_t::ELEMENTAL;
        const uint8_t gFam = (uint8_t)objects::MiDCategoryData::Family_t::GOD;
        const uint8_t dRace1 = (uint8_t)objects::MiDCategoryData::Race_t::HAUNT;
        const uint8_t dRace2 = (uint8_t)objects::MiDCategoryData::Race_t::FOUL;

        uint8_t darkCount = (uint8_t)((race1 == dRace1 || race1 == dRace2 ? 1 : 0) +
            (race2 == dRace1 || race2 == dRace2 ? 1 : 0) +
            (race3 == dRace1 || race3 == dRace2 ? 1 : 0));
        uint8_t elementalCount = (uint8_t)((f1 == eFam ? 1 : 0) +
            (f2 == eFam ? 1 : 0) + (f3 == eFam ? 1 : 0));
        uint8_t godCount = (uint8_t)((f1 == gFam ? 1 : 0) +
            (f2 == gFam ? 1 : 0) + (f3 == gFam ? 1 : 0));
        if(darkCount > 0)
        {
            if(darkCount == 1)
            {
                // Fuse non-dark demons then fuse with dark
                auto otherDef1 = (race1 != dRace1 && race1 != dRace2) ? def1 : def2;
                auto otherDef2 = (race3 != dRace1 && race3 != dRace2) ? def3 : def2;
                auto darkDef = (race1 == dRace1 || race1 == dRace2)
                                ? def1 : ((race2 == dRace1 || race2 == dRace2)
                                    ? def2 : def3);

                bool found1, found2;
                size_t race1Idx = GetRaceIndex((uint8_t)otherDef1.second
                    ->GetCategory()->GetRace(), found1);
                size_t race2Idx = GetRaceIndex((uint8_t)otherDef2.second
                    ->GetCategory()->GetRace(), found2);

                if(!found1 || !found2)
                {
                    LogFusionManagerErrorMsg("Invalid single dark, dual fusion "
                        "race encountered for trifusion\n");

                    return 0;
                }

                uint8_t resultRace = FUSION_RACE_MAP[race1Idx + 1][race2Idx];
                resultDef = GetResultDemon(resultRace,
                    GetAdjustedLevelSum(otherDef1.first, otherDef2.first));

                race1Idx = GetRaceIndex(resultRace, found1);
                race2Idx = GetRaceIndex((uint8_t)darkDef.second
                    ->GetCategory()->GetRace(), found2);

                if(!found1 || !found2)
                {
                    LogFusionManagerErrorMsg("Invalid single dark, 2nd dual "
                        "fusion race encountered for trifusion\n");

                    return 0;
                }
                else if(!resultDef)
                {
                    return 0;
                }

                resultRace = FUSION_RACE_MAP[race1Idx + 1][race2Idx];
                resultDef = GetResultDemon(resultRace,
                    GetAdjustedLevelSum(darkDef.first,
                        resultDef->GetGrowth()->GetBaseLevel()));
            }
            else if(darkCount == 2)
            {
                // Fuse non-dark demon with top priority dark demon, then
                // fuse with the low priority dark demon
                auto darkDef1 = (race1 == dRace1 || race1 == dRace2) ? def1 : def2;
                auto darkDef2 = (race3 == dRace1 || race3 == dRace2) ? def3 : def2;
                auto otherDef = (race1 != dRace1 && race1 != dRace2)
                                ? def1 : ((race2 != dRace1 && race2 != dRace2)
                                    ? def2 : def3);

                bool found1, found2;
                size_t race1Idx = GetRaceIndex((uint8_t)darkDef1.second
                    ->GetCategory()->GetRace(), found1);
                size_t race2Idx = GetRaceIndex((uint8_t)otherDef.second
                    ->GetCategory()->GetRace(), found2);

                if(!found1 || !found2)
                {
                    LogFusionManagerErrorMsg("Invalid double dark, dual fusion "
                        "race encountered for trifusion\n");

                    return 0;
                }

                uint8_t resultRace = FUSION_RACE_MAP[race1Idx + 1][race2Idx];
                resultDef = GetResultDemon(resultRace,
                    GetAdjustedLevelSum(darkDef1.first, otherDef.first));

                race1Idx = GetRaceIndex(resultRace, found1);
                race2Idx = GetRaceIndex((uint8_t)darkDef2.second
                    ->GetCategory()->GetRace(), found2);

                if(!found1 || !found2 || !resultDef)
                {
                    LogFusionManagerErrorMsg("Invalid double dark, 2nd dual "
                        "fusion race encountered for trifusion\n");

                    return 0;
                }

                resultRace = FUSION_RACE_MAP[race1Idx + 1][race2Idx];
                resultDef = GetResultDemon(resultRace,
                    GetAdjustedLevelSum(darkDef2.first,
                    resultDef->GetGrowth()->GetBaseLevel()));
            }
            else
            {
                // Get corrected level sum and return explicit level
                // range demon
                uint16_t levelSum = (uint16_t)(demon1->GetCoreStats()->GetLevel() +
                    demon2->GetCoreStats()->GetLevel() +
                    demon3->GetCoreStats()->GetLevel());

                uint32_t resultID = SVR_CONST.TRIFUSION_SPECIAL_DARK.front().second;
                for(auto pair : SVR_CONST.TRIFUSION_SPECIAL_DARK)
                {
                    if((uint16_t)pair.first > levelSum)
                    {
                        break;
                    }

                    resultID = pair.second;
                }

                return resultID;
            }
        }
        else if(elementalCount == 3)
        {
            LogFusionManagerErrorMsg("Attempted to fuse 3 elementals\n");

            return 0;
        }
        else if(elementalCount == 2)
        {
            // Special logic fusion based on 2 elemental types and
            // a set of race types
            uint32_t otherRace = 0;
            uint32_t elemType1 = 0, elemType2 = 0;
            if(f1 != eFam)
            {
                otherRace = (uint32_t)def1.second->GetCategory()->GetRace();
                elemType1 = def2.second->GetBasic()->GetID();
                elemType2 = def3.second->GetBasic()->GetID();
            }
            else if(f2 != eFam)
            {
                elemType1 = def1.second->GetBasic()->GetID();
                otherRace = (uint32_t)def2.second->GetCategory()->GetRace();
                elemType2 = def3.second->GetBasic()->GetID();
            }
            else
            {
                elemType1 = def1.second->GetBasic()->GetID();
                elemType2 = def2.second->GetBasic()->GetID();
                otherRace = (uint32_t)def3.second->GetCategory()->GetRace();
            }

            for(auto elemSpecial : SVR_CONST.TRIFUSION_SPECIAL_ELEMENTAL)
            {
                // Check explicit elemental types
                if((elemSpecial[0] == elemType1 && elemSpecial[1] == elemType2) ||
                    (elemSpecial[0] == elemType2 && elemSpecial[1] == elemType1))
                {
                    // Check valid races
                    if(otherRace == elemSpecial[2] ||
                        otherRace == elemSpecial[3] ||
                        otherRace == elemSpecial[4])
                    {
                        // Match found
                        return elemSpecial[5];
                    }
                }
            }

            LogFusionManagerErrorMsg(
                "Invalid double elemental trifusion encountered\n");

            return 0;
        }
        else if(elementalCount == 1)
        {
            // Fuse the non-elementals and scale using elemental level too
            auto otherDef1 = f1 != eFam ? def1 : def2;
            auto otherDef2 = f3 != eFam ? def3 : def2;
            auto elemDef = f1 == eFam ? def1 : (f2 == eFam ? def2 : def3);

            bool found1, found2;
            size_t race1Idx = GetRaceIndex((uint8_t)otherDef1.second
                ->GetCategory()->GetRace(), found1);
            size_t race2Idx = GetRaceIndex((uint8_t)otherDef2.second
                ->GetCategory()->GetRace(), found2);

            if(!found1 || !found2)
            {
                LogFusionManagerErrorMsg("Invalid single element, dual fusion "
                    "race encountered for trifusion\n");

                return 0;
            }

            uint8_t resultRace = FUSION_RACE_MAP[race1Idx + 1][race2Idx];
            resultDef = GetResultDemon(resultRace,
                GetAdjustedLevelSum(otherDef1.first, otherDef2.first));
            if(resultRace == eRace)
            {
                LogFusionManagerErrorMsg("Single element, dual fusion race for"
                    " trifusion resulted in a second elemental\n");

                return 0;
            }

            if(resultDef)
            {
                resultDef = GetResultDemon(resultRace,
                    GetAdjustedLevelSum(elemDef.first,
                        resultDef->GetGrowth()->GetBaseLevel()));
            }
        }
        else
        {
            // Perform normal TriFusion
            int8_t finalLevelAdjust = 0;

            // Existence of a god type boosts the fusion level by 4
            if(godCount > 0)
            {
                finalLevelAdjust = 4;
            }

            // All neutral demons lowers level by 4
            int16_t lnc1 = def1.second->GetBasic()->GetLNC();
            int16_t lnc2 = def2.second->GetBasic()->GetLNC();
            int16_t lnc3 = def3.second->GetBasic()->GetLNC();
            if((lnc1 < 5000 && lnc1 > -5000) &&
                (lnc2 < 5000 && lnc2 > -5000) &&
                (lnc3 < 5000 && lnc3 > -5000))
            {
                finalLevelAdjust = (int8_t)(finalLevelAdjust - 4);
            }

            uint16_t levelSum = (uint16_t)(def1.first + def2.first + def3.first);
            int8_t adjustedLevelSum = (int8_t)(((float)levelSum / 3.f) +
                1.f + (float)finalLevelAdjust);

            race1 = (uint8_t)def1.second->GetCategory()->GetRace();
            race2 = (uint8_t)def2.second->GetCategory()->GetRace();
            race3 = (uint8_t)def3.second->GetCategory()->GetRace();

            // Apply special logic if the top 2 races match
            if(race1 == race2)
            {
                bool found1, found2;
                size_t race1Idx = GetRaceIndex(race1, found1);
                size_t race2Idx = GetRaceIndex(race2, found2);

                if(!found1 || !found2)
                {
                    LogFusionManagerErrorMsg("Invalid dual fusion race"
                        " encountered for trifusion\n");

                    return 0;
                }

                // Perform "nested" fusion with high priority fused first, then
                // result fused to low priority (midway should be elemental)
                uint8_t elemIdx = FUSION_RACE_MAP[race1Idx + 1][race2Idx];
                if(elemIdx)
                {
                    auto elemType = GetElementalType((size_t)(elemIdx - 1));

                    uint32_t result = GetElementalFuseResult(elemType,
                        race3, def3.second->GetBasic()->GetID());
                    if(result == 0)
                    {
                        LogFusionManagerError([&]()
                        {
                            return libcomp::String("Invalid elemental fusion "
                                "request during TriFusion mid-point fusion: "
                                "%1, %2, %3\n").Arg(demonType1).Arg(demonType2)
                                .Arg(demonType3);
                        });
                    }

                    // Rank is always boosted by one at this point (can result
                    // in same low priority demon if decreased by elemental)
                    result = RankUpDown(race3, result, true);

                    return result;
                }
                else
                {
                    LogFusionManagerError([&]()
                    {
                        return libcomp::String("Attempted TriFusion on same"
                            " race highest level demons that did not result in"
                            " an elemental midpoint result: %1, %2, %3\n")
                            .Arg(demonType1).Arg(demonType2).Arg(demonType3);
                    });

                    return 0;
                }
            }
            else if(f1 >= 2 && f1 <= 9 && f2 >= 2 && f2 <= 9 &&
                f3 >= 2 && f3 <= 9)
            {
                // Top 2 races and families do not match and no special
                // conditions found, use lookup table
                size_t fIdx1 = (size_t)((f1 > f2 ? f2 : f1) - 2);
                size_t fIdx2 = (size_t)((f1 > f2 ? f1 : f2) - 3);
                size_t fIdx3 = (size_t)(f3 - 2);

                uint8_t resultRace = TRIFUSION_FAMILY_MAP[fIdx1][fIdx2][fIdx3];

                resultDef = GetResultDemon(resultRace, adjustedLevelSum);
            }
        }

        return resultDef ? resultDef->GetBasic()->GetID() : 0;
    }

    // Perform a 2-way standard fusion
    uint8_t resultRace = 0;
    uint8_t race1 = (uint8_t)def1.second->GetCategory()->GetRace();
    uint8_t race2 = (uint8_t)def2.second->GetCategory()->GetRace();

    // Get the race axis mappings from the first map row
    bool found1, found2;
    size_t race1Idx = GetRaceIndex(race1, found1);
    size_t race2Idx = GetRaceIndex(race2, found2);

    if(race1 == mRace || race2 == mRace)
    {
        // Mitama source fusion (overrides elemental)

        if(race1 == race2)
        {
            // Cannot fuse two mitamas
            return 0;
        }

        std::shared_ptr<objects::Demon> demon;
        uint32_t mitamaType = 0;
        if(race1 == mRace)
        {
            mitamaType = baseDemonType1;
            demon = demon2;
        }
        else
        {
            mitamaType = baseDemonType2;
            demon = demon1;
        }

        // Ensure the non-mitama demon has the minimum reunion
        // rank total
        if(server->GetCharacterManager()->GetReunionRankTotal(demon) < 48)
        {
            return 0;
        }

        // Double check to make sure the mitama type is valid
        GetMitamaIndex(mitamaType, found1);
        if(!found1)
        {
            return 0;
        }

        auto def = definitionManager->GetDevilData(demon->GetType());
        return def ? def->GetUnionData()->GetMitamaFusionID() : 0;
    }
    else if(race1 == eRace || race2 == eRace)
    {
        // Elemental source fusion

        if(race1 == race2)
        {
            // Two (differing) elementals result in a mitama
            size_t eIdx1 = GetElementalIndex(baseDemonType1, found1);
            size_t eIdx2 = GetElementalIndex(baseDemonType2, found2);
            if(!found1 || !found2)
            {
                return 0;
            }

            return GetMitamaType((size_t)
                FUSION_ELEMENTAL_MITAMA[eIdx1][eIdx2]);
        }

        uint32_t demonType = 0, elementalType = 0;
        uint8_t race = 0;
        if(race1 == eRace)
        {
            elementalType = baseDemonType1;
            demonType = baseDemonType2;
            race = race2;
        }
        else
        {
            elementalType = baseDemonType2;
            demonType = baseDemonType1;
            race = race1;
        }

        uint32_t result = GetElementalFuseResult(elementalType, race,
            demonType);
        if(result == 0)
        {
            LogFusionManagerError([&]()
            {
                return libcomp::String("Invalid elemental fusion request"
                    " of demon IDs  %1 and %2 received from account: %3\n")
                    .Arg(demonType1).Arg(demonType2)
                    .Arg(state->GetAccountUID().ToString());
            });
        }

        return result;
    }
    else
    {
        if(!found1 || !found2)
        {
            LogFusionManagerError([&]()
            {
                return libcomp::String("Invalid fusion request of demon IDs"
                    " %1 and %2 received from account: %3\n").Arg(demonType1)
                    .Arg(demonType2).Arg(state->GetAccountUID().ToString());
            });

            return 0;
        }

        resultRace = FUSION_RACE_MAP[(size_t)(race1Idx + 1)][race2Idx];
        if(resultRace == 0)
        {
            LogFusionManagerError([&]()
            {
                return libcomp::String("Invalid fusion result of demon IDs"
                    " %1 and %2 requested from account: %3\n").Arg(demonType1)
                    .Arg(demonType2).Arg(state->GetAccountUID().ToString());
            });

            return 0;
        }

        if(race1 == race2)
        {
            // Elemental resulting fusion
            return GetElementalType((size_t)(resultRace - 1));
        }
    }

    if(resultRace != 0)
    {
        auto resultDef = GetResultDemon(resultRace,
            GetAdjustedLevelSum((uint8_t)demon1->GetCoreStats()->GetLevel(),
                (uint8_t)demon2->GetCoreStats()->GetLevel()));
        return resultDef ? resultDef->GetBasic()->GetID() : 0;
    }

    return 0;
}


void FusionManager::EndExchange(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto exchange = state->GetExchangeSession();

    if(exchange)
    {
        auto server = mServer.lock();
        auto characterManager = server->GetCharacterManager();
        auto managerConnection = server->GetManagerConnection();

        switch(exchange->GetType())
        {
        case objects::PlayerExchangeSession::Type_t::TRIFUSION_GUEST:
        case objects::PlayerExchangeSession::Type_t::TRIFUSION_HOST:
            {
                auto cState = state->GetCharacterState();

                // Notify the whole party in the zone that the player left
                libcomp::Packet p;
                p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRIFUSION_LEFT);
                p.WriteS32Little(cState->GetEntityID());

                auto partyClients = managerConnection->GetPartyConnections(client,
                    true, true);

                ChannelClientConnection::BroadcastPacket(partyClients, p);

                // End the TriFusion for self or everyone
                libcomp::Packet request;
                request.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_TRIFUSION_END);
                request.WriteS8(1);   // Cancelled

                if(exchange->GetType() ==
                    objects::PlayerExchangeSession::Type_t::TRIFUSION_HOST)
                {
                    // End for all
                    for(auto pClient : partyClients)
                    {
                        auto pState = pClient->GetClientState();
                        auto pExchange = pState->GetExchangeSession();
                        if(pExchange && pExchange->GetType() ==
                            objects::PlayerExchangeSession::Type_t::TRIFUSION_GUEST)
                        {
                            pState->SetExchangeSession(nullptr);
                            characterManager->SetStatusIcon(pClient, 0);
                        }
                    }

                    ChannelClientConnection::BroadcastPacket(partyClients,
                        request);
                }
                else
                {
                    // End for self
                    client->QueuePacket(request);

                    // Now remove from participants
                    auto otherCState = std::dynamic_pointer_cast<CharacterState>(
                        exchange->GetOtherCharacterState());
                    auto otherClient = otherCState ? managerConnection->GetEntityClient(
                        otherCState->GetEntityID(), false) : nullptr;
                    auto otherState = otherClient ? otherClient->GetClientState() : nullptr;
                    auto tfSession = otherState ? std::dynamic_pointer_cast<
                        objects::TriFusionHostSession>(otherState->GetExchangeSession())
                        : nullptr;

                    if(tfSession)
                    {
                        for(size_t i = 0; i < tfSession->GuestsCount(); i++)
                        {
                            if(tfSession->GetGuests(i) == cState)
                            {
                                tfSession->RemoveGuests(i);
                                break;
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
        }

        state->SetExchangeSession(nullptr);
        characterManager->SetStatusIcon(client, 0);

        client->FlushOutgoing();
    }
}

int8_t FusionManager::ProcessFusion(
    const std::shared_ptr<ChannelClientConnection>& client, int64_t demonID1,
    int64_t demonID2, int64_t demonID3, uint32_t costItemType,
    std::shared_ptr<objects::Demon>& resultDemon)
{
    auto state = client->GetClientState();

    auto demon1 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID1)));
    auto demon2 = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID2)));
    auto demon3 = demonID3 > 0 ? std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID3)))
        : nullptr;
    if(!demon1 || !demon2 || (demonID3 > 0 && !demon3))
    {
        return -1;
    }

    uint32_t resultDemonType = GetResultDemon(client, demonID1, demonID2,
        demonID3);
    if(resultDemonType == 0)
    {
        return -2;
    }

    // Result demon identified, pay cost, check success rate and fuse
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto managerConnection = server->GetManagerConnection();

    auto demonData = definitionManager->GetDevilData(resultDemonType);
    double baseLevel = (double)demonData->GetGrowth()->GetBaseLevel();

    bool mitamaFusion = characterManager->IsMitamaDemon(demonData);

    // Costs get paid regardless of outcome
    bool paymentSuccess = true;
    if(costItemType == 0 || costItemType == SVR_CONST.ITEM_MACCA)
    {
        uint32_t maccaCost = 0;
        if(demonID3 > 0)
        {
            // Tri-Fusion macca cost
            maccaCost = (uint32_t)floor(1.5 * pow(baseLevel, 2));
        }
        else if(mitamaFusion)
        {
            // Set cost
            maccaCost = 50000;
        }
        else
        {
            // Normal fusion macca cost
            maccaCost = (uint32_t)floor(0.5 * pow(baseLevel, 2));
        }

        paymentSuccess = maccaCost == 0 ||
            characterManager->PayMacca(client, (uint64_t)maccaCost);
    }
    else if(costItemType == SVR_CONST.ITEM_KREUZ)
    {
        std::unordered_map<uint32_t, uint32_t> itemCost;

        uint32_t kreuzCost = 0;
        if(demonID3 > 0)
        {
            // Tri-Fusion kreuz cost
            kreuzCost = (uint32_t)ceil(baseLevel / 1.25);

            // Tri-Fusion kreuz fusion also costs one bloodstone
            itemCost[SVR_CONST.ITEM_RBLOODSTONE] = 1;
        }
        else if(mitamaFusion)
        {
            // Set cost
            kreuzCost = 2500;
        }
        else
        {
            // Normal fusion kreuz cost
            kreuzCost = (uint32_t)ceil(0.0001 * pow(baseLevel, 3));
        }

        if(kreuzCost > 0)
        {
            itemCost[SVR_CONST.ITEM_KREUZ] = kreuzCost;
        }

        paymentSuccess = itemCost.size() == 0 ||
            characterManager->AddRemoveItems(client, itemCost, false);
    }
    else
    {
        LogFusionManagerError([&]()
        {
            return libcomp::String("Invalid cost item type supplied"
                " for demon fusion: %1\n").Arg(resultDemonType);
        });

        return -3;
    }

    if(!paymentSuccess)
    {
        LogFusionManagerErrorMsg("Failed to pay fusion item cost\n");

        return -4;
    }

    // Map each demon to the appropriate participant client
    std::unordered_map<std::shared_ptr<objects::Demon>,
        std::shared_ptr<ChannelClientConnection>> dMap;
    for(auto demon : { demon1, demon2, demon3 })
    {
        if(demon)
        {
            auto dBox = std::dynamic_pointer_cast<objects::DemonBox>(
                libcomp::PersistentObject::GetObjectByUUID(demon->GetDemonBox()));
            auto account = dBox ? std::dynamic_pointer_cast<objects::Account>(
                libcomp::PersistentObject::GetObjectByUUID(dBox->GetAccount()))
                : nullptr;
            auto dClient = account ? managerConnection->GetClientConnection(
                account->GetUsername()) : nullptr;
            if(dClient)
            {
                dMap[demon] = dClient;
            }
        }
    }

    double difficulty = (double)demonData->GetUnionData()->GetFusionDifficulty();

    // Tri-Fusion success uses the same formula as normal fusion but gets a flat 12% boost
    double successRate = ceil((140.0 - (difficulty * 2.5)) +
        (difficulty - (baseLevel * 1.5)) + (difficulty * 0.5) + (demonID3 > 0 ? 12.0 : 0.0));
    if(demonID3 > 0)
    {
        // Tri-Fusion success rate gets adjusted by familiarity sum / 4000
        double adjust = floor((double)(demon1->GetFamiliarity() +
            demon2->GetFamiliarity() + demon3->GetFamiliarity()) / 4000.0);
        successRate += adjust;
    }
    else if(costItemType == SVR_CONST.ITEM_KREUZ)
    {
        // Dual kreuz fusion is an automatic 100% success
        successRate = 100.0;
    }
    else
    {
        // Normal fusion success rate gets adjusted by each demon's
        // level and familiarity rank using a lookup table
        std::list<std::pair<uint8_t, uint8_t>> famMap;
        famMap.push_back(std::pair<uint8_t, uint8_t>(
            (uint8_t)demon1->GetCoreStats()->GetLevel(),
            (uint8_t)characterManager->GetFamiliarityRank(
                demon1->GetFamiliarity())));
        famMap.push_back(std::pair<uint8_t, uint8_t>(
            (uint8_t)demon2->GetCoreStats()->GetLevel(),
            (uint8_t)characterManager->GetFamiliarityRank(
                demon2->GetFamiliarity())));

        for(auto pair : famMap)
        {
            if(pair.second >= 1 && pair.second <= 4)
            {
                uint8_t adjust = 0;
                for(size_t i = 0; i < 5; i++)
                {
                    if(FUSION_FAMILIARITY_BONUS[i][0] > pair.first)
                    {
                        break;
                    }

                    adjust = FUSION_FAMILIARITY_BONUS[i][pair.second];
                }

                successRate = (successRate + (double)adjust);
            }
        }
    }

    // Apply expertise success bonuses from all participants
    std::unordered_map<int32_t, uint16_t> expertiseBoost;
    for(auto dPair : dMap)
    {
        auto dClient = dPair.second;
        auto dState = dClient->GetClientState();
        auto dCState = dState->GetCharacterState();

        int32_t entityID = dCState->GetEntityID();
        if(expertiseBoost.find(entityID) == expertiseBoost.end())
        {
            uint16_t boost = 0;

            uint8_t fRank = dCState->GetExpertiseRank(EXPERTISE_FUSION);
            if(client == dClient)
            {
                uint8_t dRank = dCState->GetExpertiseRank(
                    EXPERTISE_DEMONOLOGY);
                if(demonID3 > 0)
                {
                    // Host adds fusion rank / 30, demonology rank / 25
                    boost = (uint16_t)(floor((double)fRank / 30.0) +
                        floor((double)dRank / 25.0));
                }
                else
                {
                    // Add fusion rank / 30, demonology rank / 5
                    boost = (uint16_t)(floor((double)fRank / 30.0) +
                        floor((double)dRank / 5.0));
                }
            }
            else
            {
                if(demonID3 > 0)
                {
                    // Guest adds fusion rank / 25
                    boost = (uint16_t)floor((double)fRank / 25.0);
                }
            }

            successRate = (double)(successRate + (double)boost);
            expertiseBoost[entityID] = boost;
        }
    }

    // Apply extra boosts for dual fusion
    if(demonID3 <= 0)
    {
        // Apply passive skill boosts
        for(auto pair : SVR_CONST.FUSION_BOOST_SKILLS)
        {
            // Apply boost if the source character has the passive and there
            // is either no race filter or the filter matches the result demon
            if(cState->SkillAvailable(pair.first) &&
                (pair.second[0] == -1 ||
                pair.second[0] == (uint8_t)demonData->GetCategory()->GetRace()))
            {
                successRate = (double)(successRate + (double)pair.second[1]);
            }
        }

        // Apply status boosts
        auto statusEffects = cState->GetStatusEffects();
        for(auto pair : SVR_CONST.FUSION_BOOST_STATUSES)
        {
            if(statusEffects.find(pair.first) != statusEffects.end())
            {
                successRate = (double)(successRate + (double)pair.second);
            }
        }
    }

    // Fusion is ready to be attempted, check for normal failure
    if(successRate <= 0.0 || (successRate < 100.0 &&
        RNG(uint16_t, 1, 10000) > (uint16_t)(successRate * 100.0)))
    {
        // Update expertise for failure
        std::list<std::pair<uint8_t, int32_t>> expPoints;

        int32_t ePoints = characterManager->CalculateExpertiseGain(cState,
            EXPERTISE_FUSION, 0.5f);
        expPoints.push_back(std::pair<uint8_t, int32_t>(EXPERTISE_FUSION,
            ePoints + 10));

        ePoints = characterManager->CalculateExpertiseGain(cState,
            EXPERTISE_DEMONOLOGY, 0.25f);
        expPoints.push_back(std::pair<uint8_t, int32_t>(EXPERTISE_DEMONOLOGY,
            ePoints + 10));

        characterManager->UpdateExpertisePoints(client, expPoints);

        return 1;
    }

    // Fusion success past this point, create the demon and update all old data

    // Calculate familiarity, store demons in the COMP and determine first
    // slot to add the new demon to
    int8_t newSlot = 10;
    uint16_t familiarity = 0;
    for(auto demon : { demon1, demon2, demon3 })
    {
        if(demon)
        {
            // Add 25% familiarity for double, 20% for triple
            familiarity = (uint16_t)(familiarity +
                ((float)demon->GetFamiliarity() * (demon3 ? 0.2f : 0.25f)));

            auto it = dMap.find(demon);
            if(it != dMap.end())
            {
                auto dClient = it->second;
                auto dState = dClient->GetClientState()->GetDemonState();
                if(dState->GetEntity() == demon)
                {
                    characterManager->StoreDemon(dClient, true,
                        demonID3 > 0 ? 16 : 12);
                }
            }

            // The first demon alwayas belongs to the "host"
            if(demon->GetDemonBox() == demon1->GetDemonBox() &&
                newSlot > demon->GetBoxSlot())
            {
                newSlot = demon->GetBoxSlot();
            }
        }
    }

    auto changes = libcomp::DatabaseChangeSet::Create(
        character->GetAccount());
    if(mitamaFusion)
    {
        // Perform mitama process on existing demon
        auto mitama = demon1;
        auto mitamaDef = definitionManager->GetDevilData(mitama->GetType());

        bool found = false;
        size_t mitamaIdx = GetMitamaIndex(mitamaDef->GetUnionData()
            ->GetBaseDemonID(), found);
        if(!found)
        {
            mitama = demon2;
            mitamaDef = definitionManager->GetDevilData(mitama->GetType());
            mitamaIdx = GetMitamaIndex(mitamaDef->GetUnionData()
                ->GetBaseDemonID(), found);
            if(!found)
            {
                // Shouldn't happen
                return -1;
            }
        }

        auto nonMitama = mitama == demon1 ? demon2 : demon1;

        auto growthData = demonData->GetGrowth();
        if(!characterManager->MitamaDemon(client, state->GetObjectID(
            nonMitama->GetUUID()), growthData->GetGrowthType(),
            (uint8_t)(mitamaIdx + 1)))
        {
            return -1;
        }

        // Clear all reunion values
        for(size_t i = 0; i < nonMitama->ReunionCount(); i++)
        {
            nonMitama->SetReunion(i, 0);
        }

        for(size_t i = 0; i < nonMitama->MitamaReunionCount(); i++)
        {
            nonMitama->SetMitamaReunion(i, 0);
        }

        characterManager->CalculateDemonBaseStats(nonMitama);

        resultDemon = nonMitama;
    }
    else
    {
        // Create the new demon
        resultDemon = characterManager->GenerateDemon(demonData,
            familiarity);

        // Determine skill inheritance
        auto inheritRestrictions = demonData->GetGrowth()->
            GetInheritanceRestrictions();
        std::map<uint32_t, std::shared_ptr<objects::MiSkillData>> inherited;
        std::unordered_map<uint32_t, int32_t> inheritedSkillCounts;
        for(auto source : { demon1, demon2, demon3 })
        {
            if(source)
            {
                for(auto learned : source->GetLearnedSkills())
                {
                    if(learned == 0) continue;

                    auto lData = definitionManager->GetSkillData(learned);

                    // Check inheritance flags for valid skills
                    auto r = lData->GetAcquisition()->GetInheritanceRestriction();
                    if((inheritRestrictions & (uint16_t)(1 << r)) == 0) continue;

                    inherited[learned] = lData;

                    if(inheritedSkillCounts.find(learned) == inheritedSkillCounts.end())
                    {
                        inheritedSkillCounts[learned] = 1;
                    }
                    else
                    {
                        inheritedSkillCounts[learned]++;
                    }
                }
            }
        }

        // Remove skills the result demon already knows
        for(auto skillID : demonData->GetGrowth()->GetSkills())
        {
            inherited.erase(skillID);
        }

        // Correct the COMP
        auto comp = character->GetCOMP().Get();

        resultDemon->SetDemonBox(comp->GetUUID());
        resultDemon->SetBoxSlot(newSlot);
        comp->SetDemons((size_t)newSlot, resultDemon);

        // Prepare the updates and generate the inherited skills
        changes->Insert(resultDemon);
        changes->Insert(resultDemon->GetCoreStats().Get());

        uint8_t iType = demonData->GetGrowth()->GetInheritanceType();
        if(iType <= 21)
        {
            for(auto iPair : inherited)
            {
                // Add inherited skills, double or triple if two or
                // three sources learned it respectively
                uint8_t affinity = iPair.second->GetCommon()->GetAffinity();

                // Skip "none" and weapon affinity
                if(affinity <= 1) continue;

                uint8_t baseValue = INHERITENCE_SKILL_MAP
                    [(size_t)(affinity - 2)][iType];
                int32_t multiplier = inheritedSkillCounts[iPair.first] * 100;

                int32_t progress = (int32_t)baseValue * multiplier;
                if(progress > MAX_INHERIT_SKILL)
                {
                    progress = MAX_INHERIT_SKILL;
                }

                auto iSkill = libcomp::PersistentObject::New<
                    objects::InheritedSkill>(true);
                iSkill->SetSkill(iPair.first);
                iSkill->SetProgress((int16_t)progress);
                iSkill->SetDemon(resultDemon->GetUUID());
                resultDemon->AppendInheritedSkills(iSkill);

                changes->Insert(iSkill);
            }
        }

        changes->Update(comp);
    }

    // Register the object ID, reset if its already there
    state->SetObjectID(resultDemon->GetUUID(),
        server->GetNextObjectID(), true);

    // Delete the demons and send the new COMP slot info
    for(auto demon : { demon1, demon2, demon3 })
    {
        if(demon)
        {
            if(demon != resultDemon)
            {
                characterManager->DeleteDemon(demon, changes);
            }

            auto it = dMap.find(demon);
            if(it != dMap.end())
            {
                auto dClient = it->second;
                characterManager->SendDemonBoxData(dClient,
                    0, { demon->GetBoxSlot() });
            }
        }
    }

    server->GetWorldDatabase()->QueueChangeSet(changes);

    // Update demon quest if active
    server->GetEventManager()->UpdateDemonQuestCount(client,
        objects::DemonQuest::Type_t::FUSE, resultDemonType, 1);

    // Update expertise for success
    std::list<std::pair<uint8_t, int32_t>> expPoints;

    int32_t ePoints = characterManager->CalculateExpertiseGain(cState,
        EXPERTISE_FUSION, 2.f);
    expPoints.push_back(std::pair<uint8_t, int32_t>(EXPERTISE_FUSION,
        ePoints + 10));

    ePoints = characterManager->CalculateExpertiseGain(cState,
        EXPERTISE_DEMONOLOGY, 1.f);
    expPoints.push_back(std::pair<uint8_t, int32_t>(EXPERTISE_DEMONOLOGY,
        ePoints + 10));

    characterManager->UpdateExpertisePoints(client, expPoints);

    return 0;
}

int8_t FusionManager::GetAdjustedLevelSum(uint8_t level1, uint8_t level2,
    int8_t finalLevelAdjust)
{
    uint8_t levelSum = (uint8_t)(level1 + level2);
    return (int8_t)(((float)levelSum / 2.f) + 1.f + (float)finalLevelAdjust);
}

std::shared_ptr<objects::MiDevilData> FusionManager::GetResultDemon(
    uint8_t race, int8_t adjustedLevelSum)
{
    // Normal race selection adjusted for level range
    auto definitionManager = mServer.lock()->GetDefinitionManager();
    auto fusionRanges = definitionManager->GetFusionRanges(race);
    if(fusionRanges.size() == 0)
    {
        LogFusionManagerError([&]()
        {
            return libcomp::String("No valid fusion range found"
                " for race ID: %1\n").Arg(race);
        });

        return 0;
    }

    // Traverse the pre-sorted list and take the highest range accessible
    uint32_t resultID = fusionRanges.front().second;
    for(auto pair : fusionRanges)
    {
        resultID = pair.second;

        if(pair.first >= adjustedLevelSum)
        {
            break;
        }
    }

    return resultID ? definitionManager->GetDevilData(resultID) : nullptr;
}

uint32_t FusionManager::GetElementalType(size_t elementalIndex) const
{
    const static uint32_t FUSION_ELEMENTAL_TYPES[] =
        {
            SVR_CONST.ELEMENTAL_1_FLAEMIS,
            SVR_CONST.ELEMENTAL_2_AQUANS,
            SVR_CONST.ELEMENTAL_3_AEROS,
            SVR_CONST.ELEMENTAL_4_ERTHYS
        };

    return elementalIndex < 4 ? FUSION_ELEMENTAL_TYPES[elementalIndex] : 0;
}

uint32_t FusionManager::GetMitamaType(size_t mitamaIndex) const
{
    const static uint32_t FUSION_MITAMA_TYPES[] =
        {
            SVR_CONST.MITAMA_1_ARAMITAMA,
            SVR_CONST.MITAMA_2_NIGIMITAMA,
            SVR_CONST.MITAMA_3_KUSHIMITAMA,
            SVR_CONST.MITAMA_4_SAKIMITAMA
        };

    return mitamaIndex < 4 ? FUSION_MITAMA_TYPES[mitamaIndex] : 0;
}

uint32_t FusionManager::GetElementalFuseResult(uint32_t elementalType,
    uint8_t otherRace, uint32_t otherType)
{
    bool raceFound = false;
    size_t raceIdx = GetRaceIndex(otherRace, raceFound);

    bool elementalFound = false;
    size_t elementalIdx = GetElementalIndex(elementalType, elementalFound);
    if(!elementalFound || !raceFound ||
        FUSION_ELEMENTAL_ADJUST[raceIdx][elementalIdx] == 0)
    {
        return 0;
    }

    bool up = FUSION_ELEMENTAL_ADJUST[raceIdx][elementalIdx] == 1;
    return RankUpDown(otherRace, otherType, up);
}

size_t FusionManager::GetRaceIndex(uint8_t raceID, bool& found)
{
    found = false;

    for(size_t i = 0; i < 34; i++)
    {
        if(FUSION_RACE_MAP[0][i] == raceID)
        {
            found = true;
            return i;
        }
    }

    return false;
}

size_t FusionManager::GetElementalIndex(uint32_t elemType, bool& found)
{
    found = false;

    for(size_t i = 0; i < 4; i++)
    {
        if(GetElementalType(i) == elemType)
        {
            found = true;
            return i;
        }
    }

    return 0;
}

size_t FusionManager::GetMitamaIndex(uint32_t mitamaType, bool& found)
{
    found = false;

    for(size_t i = 0; i < 4; i++)
    {
        if(GetMitamaType(i) == mitamaType)
        {
            found = true;
            return i;
        }
    }

    return 0;
}

bool FusionManager::IsTriFusionValid(const std::shared_ptr<
    objects::Demon>& demon)
{
    // Demon cannot be locked (or null obviously)
    if(!demon || demon->GetLocked())
    {
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto devilData = definitionManager->GetDevilData(demon->GetType());

    // Demon cannot be mitama demon or a base mitama type
    bool found = false;
    GetMitamaIndex(devilData->GetUnionData()->GetBaseDemonID(), found);
    return !found && !characterManager->IsMitamaDemon(devilData);
}

uint32_t FusionManager::RankUpDown(uint8_t raceID, uint32_t demonType,
    bool up)
{
    auto fusionRanges = mServer.lock()->GetDefinitionManager()
        ->GetFusionRanges(raceID);

    // Default to the current demon for up/down fusion at limit already
    for(auto it = fusionRanges.begin(); it != fusionRanges.end(); it++)
    {
        if(it->second == demonType)
        {
            if(up)
            {
                it++;
                if(it != fusionRanges.end())
                {
                    return it->second;
                }
            }
            else if(it != fusionRanges.begin())
            {
                it--;
                return it->second;
            }

            break;
        }
    }

    return demonType;
}
