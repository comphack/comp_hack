/**
 * @file server/channel/src/packets/game/DemonReunion.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the clent to reunion the summoned partner demon.
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
#include <MiDevilData.h>
#include <MiDevilLVUpRateData.h>
#include <MiDevilReunionConditionData.h>
#include <MiGrowthData.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

void HandleDemonReunion(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client,
    int64_t demonID, uint8_t growthType, uint32_t costItemType)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto devilData = dState->GetDevilData();

    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto demon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID)));
    auto cs = demon ? demon->GetCoreStats().Get() : nullptr;

    bool success = false;

    auto growthData = definitionManager->GetDevilLVUpRateData(growthType);
    if(demon && dState->GetEntity() == demon && devilData &&
        growthData && growthData->GetGroupID() >= 1)
    {
        bool anyItem = false;
        bool itemFound = false;
        uint16_t itemsRequired = 0;
        for(auto con : growthData->GetReunionConditions())
        {
            uint32_t itemType = con->GetItemID();
            if(itemType)
            {
                if(costItemType == itemType)
                {
                    itemsRequired = con->GetAmount();
                    itemFound = true;
                }
                anyItem = true;
            }
        }

        size_t groupIdx = (size_t)(growthData->GetGroupID() - 1);
        int8_t rank = growthData->GetSubID();
        int8_t targetRank = demon->GetReunion(groupIdx);

        auto growthData2 = definitionManager->GetDevilLVUpRateData(
            demon->GetGrowthType());
        bool isSwitch = growthData2 &&
            growthData2->GetGroupID() != growthData->GetGroupID();
        bool isReset = devilData->GetGrowth()->GetGrowthType() == growthType;

        // Valid if an item matched the request item and the growth type is
        // being reset, set to a lower or equal rank value or set to the next
        // sequential rank value
        success = (!anyItem || itemFound) &&
            (isReset || targetRank >= rank ||
            (isSwitch && (rank == 1 && targetRank == 0)) ||
            (!isSwitch &&
                (targetRank == (rank - 1) || (rank == 2 && targetRank == 0))));
        if(success && !isReset && rank > 1)
        {
            // Base criteria valid, make sure the demon is leveled enough
            int8_t lvl = demon->GetCoreStats()->GetLevel();
            if(rank >= 9)
            {
                if(lvl < 99)
                {
                    success = false;
                }
            }
            else if((int8_t)(rank * 10 + 10) > lvl)
            {
                success = false;
            }
        }

        if(success)
        {
            // Pay cost
            std::list<std::shared_ptr<objects::Item>> inserts;
            std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> cost;

            uint64_t maccaCost = (uint64_t)((rank > 0 ? rank : 1) * 500 *
                cs->GetLevel());
            success = characterManager->CalculateMaccaPayment(client,
                maccaCost, inserts, cost);

            if(costItemType)
            {
                success &= characterManager->CalculateItemRemoval(client,
                    costItemType, (uint64_t)itemsRequired, cost) == 0;
            }

            success &= characterManager->UpdateItems(client, false, inserts,
                cost);
        }

        if(success)
        {
            if(growthData2 && growthData2->GetGroupID() > 0 && isSwitch)
            {
                // Make sure the growth type being changed from has at least
                // rank 1
                size_t groupIdx2 = (size_t)(growthData2->GetGroupID() - 1);
                if(demon->GetReunion(groupIdx2) == 0)
                {
                    demon->SetReunion(groupIdx2, 1);
                }
            }

            // Ranks can only go up
            if(targetRank < rank)
            {
                demon->SetReunion(groupIdx, rank);
            }
            else if(!isSwitch && targetRank >= 9 && rank == 9 &&
                growthType == demon->GetGrowthType())
            {
                // Setting to normal max again, check if the rank is configured
                // to exceed normal max
                uint8_t max = server->GetWorldSharedConfig()->GetReunionMax();
                if((uint8_t)(targetRank + 1) <= max)
                {
                    demon->SetReunion(groupIdx, (int8_t)(targetRank + 1));
                }
            }

            auto dbChanges = libcomp::DatabaseChangeSet::Create(
                state->GetAccountUID());

            cs->SetLevel(1);
            demon->SetGrowthType(growthType);
            characterManager->CalculateDemonBaseStats(demon);

            server->GetTokuseiManager()->Recalculate(cState, true,
                std::set<int32_t>{ dState->GetEntityID() });
            characterManager->RecalculateStats(dState, client, false);

            cs->SetHP(dState->GetMaxHP());
            cs->SetMP(dState->GetMaxMP());

            dbChanges->Update(demon);
            dbChanges->Update(cs);

            server->GetWorldDatabase()->QueueChangeSet(dbChanges);
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_REUNION);
    reply.WriteS8(success ? 0 : -1);
    reply.WriteS64Little(demonID);
    reply.WriteU8(growthType);

    client->QueuePacket(reply);

    if(success)
    {
        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_PARTNER_LEVEL_DOWN);
        notify.WriteS32Little(dState->GetEntityID());
        notify.WriteS8(cs->GetLevel());
        notify.WriteS64Little(0);    // Probably object ID, always 0 though
        characterManager->GetEntityStatsPacketData(notify, cs, dState, 1);
        notify.WriteU8(growthType);

        for(int8_t reunionRank : demon->GetReunion())
        {
            notify.WriteS8(reunionRank);
        }

        notify.WriteS8(demon->GetMagReduction());

        server->GetZoneManager()->BroadcastPacket(client, notify);
    }

    client->FlushOutgoing();
}

bool Parsers::DemonReunion::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 13)
    {
        return false;
    }

    int64_t demonID = p.ReadS64Little();
    uint8_t growthType = p.ReadU8();
    uint32_t costItemType = p.ReadU32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    server->QueueWork(HandleDemonReunion, server, client, demonID,
        growthType, costItemType);

    return true;
}
