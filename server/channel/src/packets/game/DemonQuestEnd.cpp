/**
 * @file server/channel/src/packets/game/DemonQuestEnd.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to turn in the active demon quest. If the quest has
 *  expired, this will send a failure notification instead, effectively
 *  acting like a cancellation.
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
#include <Packet.h>
#include <PacketCodes.h>
#include <Randomizer.h>

// object Includes
#include <CharacterProgress.h>
#include <Demon.h>
#include <DemonQuest.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "DefinitionManager.h"
#include "EventManager.h"

using namespace channel;

bool Parsers::DemonQuestEnd::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 1)
    {
        return false;
    }

    int8_t unknown = p.ReadS8();
    (void)unknown;

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
    auto dQuest = character->GetDemonQuest().Get();

    auto demon = dQuest ? std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(dQuest->GetDemon()))
        : nullptr;

    int8_t failCode = demon
        ? server->GetEventManager()->EndDemonQuest(client, 0) : 2;
    if(!failCode)
    {
        // Grant a random new title
        std::set<int16_t> existingTitles;
        for(int16_t title : progress->GetTitles())
        {
            existingTitles.insert(title);
        }

        std::set<int16_t> possibleTitles;
        for(int16_t title : definitionManager->GetTitleIDs())
        {
            if(existingTitles.find(title) == existingTitles.end())
            {
                possibleTitles.insert(title);
            }
        }

        if(possibleTitles.size() > 0)
        {
            int16_t newTitle = libcomp::Randomizer::GetEntry(possibleTitles);
            characterManager->AddTitle(client, newTitle);
        }

        // Grant all reward items
        std::unordered_map<uint32_t, uint32_t> addItems;
        if(dQuest->GetChanceItem())
        {
            addItems[dQuest->GetChanceItem()] =
                (uint32_t)dQuest->GetChanceItemCount();
        }

        for(auto& pair : dQuest->GetRewardItems())
        {
            auto it = addItems.find(pair.first);

            uint32_t newStack = it != addItems.end()
                ? it->second : 0;
            addItems[pair.first] = (uint32_t)(newStack + pair.second);
        }

        for(auto& pair : dQuest->GetBonusItems())
        {
            auto it = addItems.find(pair.first);

            uint32_t newStack = it != addItems.end()
                ? it->second : 0;
            addItems[pair.first] = (uint32_t)(newStack + pair.second);
        }

        if(addItems.size() > 0)
        {
            characterManager->AddRemoveItems(client, addItems, true);
        }

        // Grant all bonus titles
        for(uint16_t titleID : dQuest->GetBonusTitles())
        {
            characterManager->AddTitle(client, (int16_t)titleID);
        }

        // Grant all XP rewards
        int64_t xp = 0;
        if(dQuest->GetXPReward() > 0)
        {
            xp = dQuest->GetXPReward();
        }

        for(uint32_t bonusXP : dQuest->GetBonusXP())
        {
            xp = (int64_t)(xp + bonusXP);
        }

        if(xp)
        {
            characterManager->UpdateExperience(client, xp,
                cState->GetEntityID());
        }
    }
    else if(failCode != -1)
    {
        // Fail the quest
        server->GetEventManager()->EndDemonQuest(client, failCode);
    }

    return true;
}
