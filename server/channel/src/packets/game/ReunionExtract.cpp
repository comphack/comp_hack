/**
 * @file server/channel/src/packets/game/ReunionExtract.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to extract reunion conversion points from
 *  a demon.
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
#include <ServerConstants.h>

// object Includes
#include <AccountWorldData.h>
#include <MiDevilData.h>
#include <MiGrowthData.h>
#include <MiNPCBasicData.h>
#include <MiUnionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "FusionTables.h"

using namespace channel;

void ExtractReunionPoints(const std::shared_ptr<ChannelServer> server,
    const std::shared_ptr<ChannelClientConnection> client)
{
    auto characterManager = server->GetCharacterManager();

    auto state = client->GetClientState();
    auto awd = state->GetAccountWorldData().Get();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();
    auto demonData = dState->GetDevilData();

    // Get the highest priority extraction item type available
    uint32_t extractItem = 0;
    for(uint32_t itemType : SVR_CONST.REUNION_EXTRACT_ITEMS)
    {
        if(characterManager->GetExistingItemCount(character, itemType) > 0)
        {
            extractItem = itemType;
            break;
        }
    }

    int32_t rPoints = 0, mPoints = 0;

    bool success = awd && extractItem && demon && demonData;
    if(success)
    {
        // Consume the item and store the demon
        std::unordered_map<uint32_t, uint32_t> items;
        items[extractItem] = 1;
        if(characterManager->AddRemoveItems(client, items, false))
        {
            characterManager->StoreDemon(client);

            // Calculate point gain
            int32_t points = 0;
            for(int8_t rank : demon->GetReunion())
            {
                for(int8_t p = 0; p <= rank && p < 10; p++)
                {
                    points = (int32_t)(points +
                        REUNION_RANK_POINTS[(size_t)p]);
                }
            }

            if(characterManager->IsMitamaDemon(demonData))
            {
                mPoints = points;

                if(points > 0)
                {
                    // Reunion points have a minimum of 180
                    rPoints = 180;
                    if(points > 180)
                    {
                        rPoints = points;
                    }
                }
            }
            else
            {
                rPoints = points;
            }
        }
        else
        {
            success = false;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_REUNION_EXTRACT);
    reply.WriteS32Little(0);    // Unknown
    reply.WriteS32Little(success ? 0 : -1);
    reply.WriteS32Little(rPoints);
    reply.WriteS32Little(mPoints);
    reply.WriteU32Little(demonData ? demonData->GetBasic()->GetID() : 0);
    reply.WriteU32Little(demonData
        ? demonData->GetUnionData()->GetBaseDemonID() : 0);

    client->QueuePacket(reply);

    if(success)
    {
        // Add the points, update the demon and recalc
        rPoints = rPoints + (int32_t)awd->GetReunionPoints();
        mPoints = mPoints + (int32_t)awd->GetMitamaReunionPoints();

        // Points max at 100000
        if(rPoints > 100000)
        {
            rPoints = 100000;
        }

        if(mPoints > 100000)
        {
            mPoints = 100000;
        }

        awd->SetReunionPoints((uint32_t)rPoints);
        awd->SetMitamaReunionPoints((uint32_t)mPoints);

        // All reunion ranks reset
        auto newDemonData = demonData;
        for(size_t i = 0; i < demon->ReunionCount(); i++)
        {
            demon->SetReunion(i, 0);
        }

        // Mitama'd types revert to their base demon type and reset all mitama
        // information
        uint32_t baseDemonType = demonData->GetUnionData()->GetBaseDemonID();
        if(characterManager->IsMitamaDemon(demonData) && baseDemonType)
        {
            uint32_t currentType = demon->GetType();

            auto definitionManager = server->GetDefinitionManager();
            newDemonData = definitionManager->GetDevilData(baseDemonType);

            demon->SetType(baseDemonType);
            demon->SetMitamaRank(0);
            demon->SetMitamaType(0);

            for(size_t i = 0; i < demon->MitamaReunionCount(); i++)
            {
                demon->SetMitamaReunion(i, 0);
            }

            LogCharacterManagerDebug([&]()
            {
                return libcomp::String("Extracting %1 reunion point(s), %2"
                    " mitama point(s) and reverting demon type %3 to base"
                    " type %4: %5\n")
                    .Arg(rPoints).Arg(mPoints).Arg(currentType)
                    .Arg(baseDemonType).Arg(demon->GetUUID().ToString());
            });
        }
        else
        {
            LogCharacterManagerDebug([rPoints, demon]()
            {
                return libcomp::String("Extracting %1 reunion point(s) from"
                    " demon: %2\n").Arg(rPoints)
                    .Arg(demon->GetUUID().ToString());
            });
        }

        // Growth type reverts to default
        demon->SetGrowthType(newDemonData->GetGrowth()->GetGrowthType());

        // Recalculate demon stats
        characterManager->CalculateDemonBaseStats(demon);
        characterManager->SendDemonData(client, 0, demon->GetBoxSlot(),
            state->GetObjectID(demon->GetUUID()));

        auto dbChanges = libcomp::DatabaseChangeSet::Create(state
            ->GetAccountUID());
        dbChanges->Update(awd);
        dbChanges->Update(demon);

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);
    }

    client->FlushOutgoing();
}

bool Parsers::ReunionExtract::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    int32_t unknown = p.ReadS32Little();
    (void)unknown;  // Always 0

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    server->QueueWork(ExtractReunionPoints, server, client);

    return true;
}
