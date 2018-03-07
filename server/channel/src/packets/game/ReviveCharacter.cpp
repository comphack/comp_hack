/**
 * @file server/channel/src/packets/game/ReviveCharacter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to revive the player character.
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
#include <Constants.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <ItemBox.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ReviveCharacter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

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

    int32_t entityID = p.ReadS32Little();
    int32_t revivalMode = p.ReadS32Little();
    (void)entityID;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto zoneManager = server->GetZoneManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto cs = cState->GetCoreStats();

    int8_t responseType1 = -1, responseType2 = -1;

    int64_t xpLoss = 0;
    uint32_t newZoneID = 0;
    float newX = 0.f, newY = 0.f, newRot = 0.f;
    float hpRestore = 0;
    bool xpLossLevel = cs->GetLevel() >= 10 && cs->GetLevel() < 99;
    switch(revivalMode)
    {
    case 104:   // Home point
        if(character->GetHomepointZone())
        {
            responseType1 = REVIVAL_REVIVE_AND_WAIT;
            responseType2 = REVIVAL_REVIVE_DONE;
            hpRestore = 1.f;

            // Adjust XP
            if(xpLossLevel)
            {
                xpLoss = (int64_t)floorl(
                    (double)libcomp::LEVEL_XP_REQUIREMENTS[(size_t)cs->GetLevel()] *
                    (double)(0.01 - (0.00005 * cs->GetLevel())) - 0.01);
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
    case 105:   // Dungeon entrance
        {
            responseType1 = REVIVAL_REVIVE_AND_WAIT;
            responseType2 = REVIVAL_REVIVE_DONE;
            hpRestore = 0.3f;

            // Adjust XP
            if(xpLossLevel)
            {
                xpLoss = (int64_t)floorl(
                    (double)libcomp::LEVEL_XP_REQUIREMENTS[(size_t)cs->GetLevel()] *
                    (double)(0.02 - (0.00005 * cs->GetLevel())));
            }

            // Move to entrance
            auto zone = cState->GetZone();
            auto zoneDef = zone->GetDefinition();
            newZoneID = zoneDef->GetID();
            newX = zoneDef->GetStartingX();
            newY = zoneDef->GetStartingY();
            newRot = zoneDef->GetStartingRotation();
        }
        break;
    case 107:   // Item revival
        {
            std::unordered_map<uint32_t, uint32_t> itemMap;
            itemMap[SVR_CONST.ITEM_BALM_OF_LIFE] = 1;

            if(characterManager->AddRemoveItems(client, itemMap, false))
            {
                responseType1 = REVIVAL_REVIVE_NORMAL;
                hpRestore = 1.f;
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
    case 596:   /// @todo: Special dungeon entrance revival?
    case 665:   /// @todo: Followed by a zone change
    default:
        LOG_ERROR(libcomp::String("Unknown revival mode requested: %1\n")
            .Arg(revivalMode));
        return true;
        break;
    }

    if(xpLoss > 0)
    {
        if(xpLoss > cs->GetXP())
        {
            xpLoss = cs->GetXP();
        }

        cs->SetXP(cs->GetXP() - xpLoss);
    }

    if(hpRestore > 0.f)
    {
        if(cState->SetHPMP((int32_t)floorl((float)cState->GetMaxHP() *
            hpRestore), -1, false))
        {
            std::set<std::shared_ptr<ActiveEntityState>> displayState;
            displayState.insert(cState);

            characterManager->UpdateWorldDisplayState(displayState);

            state->SetAcceptRevival(false);
        }
    }

    libcomp::Packet reply;
    characterManager->GetEntityRevivalPacket(reply, cState, responseType1);

    if(responseType1 == REVIVAL_REVIVE_NORMAL ||
        responseType1 == REVIVAL_REVIVE_ACCEPT ||
        responseType1 == REVIVAL_REVIVE_DENY)
    {
        zoneManager->BroadcastPacket(client, reply);
    }
    else
    {
        client->QueuePacket(reply);
    }

    if(newZoneID)
    {
        zoneManager->EnterZone(client, newZoneID, 0, newX, newY, newRot, true);

        // Send the revival info to players in the new zone
        reply.Clear();
        characterManager->GetEntityRevivalPacket(reply, cState, responseType1);
        zoneManager->BroadcastPacket(client, reply, false);

        // Complete the revival
        reply.Clear();
        characterManager->GetEntityRevivalPacket(reply, cState, responseType2);
        zoneManager->BroadcastPacket(client, reply);
    }
    else
    {
        client->FlushOutgoing();
    }

    if(hpRestore > 0.f)
    {
        // If the character was revived, check HP baed effects
        server->GetTokuseiManager()->Recalculate(cState,
            std::set<TokuseiConditionType> { TokuseiConditionType::CURRENT_HP });
    }

    return true;
}
