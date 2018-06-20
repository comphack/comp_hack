/**
 * @file server/channel/src/packets/game/ReviveCharacter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to revive the player character
 *  (or in some instances the partner demon).
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
#include <Constants.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ChannelConfig.h>
#include <ItemBox.h>
#include <MiSpotData.h>
#include <ServerZone.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::ReviveCharacter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 8)
    {
        return false;
    }

    const static int8_t REVIVAL_REVIVE_DONE = -1;
    const static int8_t REVIVAL_REVIVE_AND_WAIT = 1;    // Waits on -1
    const static int8_t REVIVAL_REVIVE_NORMAL = 3;
    const static int8_t REVIVAL_REVIVE_ACCEPT = 4;
    const static int8_t REVIVAL_REVIVE_DENY = 5;
    //const static int8_t REVIVAL_REVIVE_UNKNOWN = 7;  // Used by revival mode 596
    const static int8_t REVIVAL_DEMON_ONLY_QUIT = 8;

    int32_t entityID = p.ReadS32Little();
    int32_t revivalMode = p.ReadS32Little();
    (void)entityID;

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto zoneManager = server->GetZoneManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto zone = state->GetZone();

    auto character = cState->GetEntity();
    auto characterLevel = cState->GetCoreStats()->GetLevel();

    int8_t responseType1 = -1, responseType2 = -1;

    int64_t xpLoss = 0;
    uint32_t newZoneID = 0;
    float newX = 0.f, newY = 0.f, newRot = 0.f;
    std::unordered_map<std::shared_ptr<ActiveEntityState>, int32_t> hpRestores;

    bool xpLossLevel = characterLevel >= 10 && characterLevel < 99;
    switch(revivalMode)
    {
    case 104:   // Home point
        if(character->GetHomepointZone())
        {
            responseType1 = REVIVAL_REVIVE_AND_WAIT;
            responseType2 = REVIVAL_REVIVE_DONE;
            hpRestores[cState] = cState->GetMaxHP();

            // Adjust XP
            if(xpLossLevel)
            {
                xpLoss = (int64_t)floorl(
                    (double)libcomp::LEVEL_XP_REQUIREMENTS[(size_t)characterLevel] *
                    (double)(0.01 - (0.00005 * characterLevel)) - 0.01);
            }

            // Change zone
            newZoneID = character->GetHomepointZone();

            auto zoneDef = server->GetServerDataManager()->GetZoneData(newZoneID, 0);
            if(zoneDef)
            {
                zoneManager->GetSpotPosition(zoneDef->GetDynamicMapID(),
                    character->GetHomepointSpotID(), newX, newY, newRot);
            }
        }
        break;
    case 105:   // Dungeon entrance/last zone-in point
        {
            responseType1 = REVIVAL_REVIVE_AND_WAIT;
            responseType2 = REVIVAL_REVIVE_DONE;
            hpRestores[cState] = (int32_t)floorl((float)cState->GetMaxHP() * 0.3f);

            // Adjust XP
            if(xpLossLevel)
            {
                xpLoss = (int64_t)floorl(
                    (double)libcomp::LEVEL_XP_REQUIREMENTS[(size_t)characterLevel] *
                    (double)(0.02 - (0.00005 * characterLevel)));
            }

            // Move to entrance unless a zone-in spot overrides it
            auto zoneDef = zone->GetDefinition();
            newZoneID = zoneDef->GetID();
            newX = zoneDef->GetStartingX();
            newY = zoneDef->GetStartingY();
            newRot = zoneDef->GetStartingRotation();

            uint32_t spotID = state->GetZoneInSpotID();
            if(spotID)
            {
                auto definitionManager = server->GetDefinitionManager();
                auto zoneData = definitionManager->GetZoneData(
                    zoneDef->GetID());
                auto spots = definitionManager->GetSpotData(zoneDef
                    ->GetDynamicMapID());
                auto spotIter = spots.find(spotID);
                if(spotIter != spots.end())
                {
                    Point point = zoneManager->GetRandomSpotPoint(
                        spotIter->second, zoneData);
                    newX = point.x;
                    newY = point.y;
                    newRot = spotIter->second->GetRotation();
                }
            }
        }
        break;
    case 107:   // Item revival
        {
            std::unordered_map<uint32_t, uint32_t> itemMap;
            itemMap[SVR_CONST.ITEM_BALM_OF_LIFE] = 1;

            if(characterManager->AddRemoveItems(client, itemMap, false))
            {
                responseType1 = REVIVAL_REVIVE_NORMAL;
                hpRestores[cState] = cState->GetMaxHP();
            }
        }
        break;
    case 108:   // Receive revival
        {
            responseType1 = REVIVAL_REVIVE_ACCEPT;
            state->SetAcceptRevival(true);
        }
        break;
    case 109:   // Deny receive revival
        {
            responseType1 = REVIVAL_REVIVE_DENY;
            state->SetAcceptRevival(false);
        }
        break;
    case 664:   // Demon dungeon item revival
        {
            std::unordered_map<uint32_t, uint32_t> itemMap;
            itemMap[SVR_CONST.ITEM_BALM_OF_LIFE_DEMON] = 1;

            if(characterManager->AddRemoveItems(client, itemMap, false))
            {
                responseType1 = REVIVAL_REVIVE_NORMAL;
                hpRestores[dState] = dState->GetMaxHP();
                hpRestores[cState] = 1;
            }
        }
        break;
    case 665:   // Demon dungeon give up
        {
            responseType1 = REVIVAL_DEMON_ONLY_QUIT;

            auto zoneDef = zone->GetDefinition();
            newZoneID = zoneDef->GetGroupID();

            zoneDef = server->GetServerDataManager()
                ->GetZoneData(newZoneID, 0);
            if(zoneDef)
            {
                newX = zoneDef->GetStartingX();
                newY = zoneDef->GetStartingY();
                newRot = zoneDef->GetStartingRotation();
            }
            else
            {
                newZoneID = 0;
            }
        }
        break;
    case 596:   /// @todo: Special dungeon entrance revival?
    default:
        LOG_ERROR(libcomp::String("Unknown revival mode requested: %1\n")
            .Arg(revivalMode));
        return true;
        break;
    }

    bool deathPenaltyDisabled = server->GetWorldSharedConfig()
        ->GetDeathPenaltyDisabled();

    if(xpLoss > 0 && !deathPenaltyDisabled)
    {
        auto cs = character->GetCoreStats().Get();
        if(xpLoss > cs->GetXP())
        {
            xpLoss = cs->GetXP();
        }

        cs->SetXP(cs->GetXP() - xpLoss);
    }

    if(hpRestores.size() > 0)
    {
        std::set<std::shared_ptr<ActiveEntityState>> displayState;

        for(auto& pair : hpRestores)
        {
            if(pair.first->SetHPMP(pair.second, -1, false))
            {
                displayState.insert(pair.first);

                // Trigger revival actions
                zoneManager->TriggerZoneActions(zone, { pair.first },
                    ZoneTrigger_t::ON_REVIVAL, client);
            }
        }

        characterManager->UpdateWorldDisplayState(displayState);

        state->SetAcceptRevival(false);
    }

    libcomp::Packet reply;
    if(hpRestores.size() == 0)
    {
        characterManager->GetEntityRevivalPacket(reply, cState, responseType1);
        zoneManager->BroadcastPacket(client, reply);
    }
    else
    {
        for(auto& pair : hpRestores)
        {
            reply.Clear();
            characterManager->GetEntityRevivalPacket(reply, pair.first,
                responseType1);
            zoneManager->BroadcastPacket(zone, reply);
        }

        // If reviving in a demon only instance, clear the death time-out
        if(responseType1 == REVIVAL_REVIVE_NORMAL &&
            zone->GetInstanceType() == InstanceType_t::DEMON_ONLY)
        {
            zoneManager->UpdateDeathTimeOut(state, -1);
        }
    }

    if(newZoneID)
    {
        zoneManager->EnterZone(client, newZoneID, 0, newX, newY, newRot, true);

        // Send the revival info to players in the new zone
        reply.Clear();
        characterManager->GetEntityRevivalPacket(reply, cState, responseType1);
        zoneManager->BroadcastPacket(client, reply, false);

        // Complete the revival
        if(responseType2 != -1)
        {
            reply.Clear();
            characterManager->GetEntityRevivalPacket(reply, cState, responseType2);
            zoneManager->BroadcastPacket(client, reply);
        }
    }

    client->FlushOutgoing();

    for(auto& pair : hpRestores)
    {
        // If any entity was revived, check HP based effects
        server->GetTokuseiManager()->Recalculate(pair.first,
            std::set<TokuseiConditionType> { TokuseiConditionType::CURRENT_HP });
    }

    return true;
}
