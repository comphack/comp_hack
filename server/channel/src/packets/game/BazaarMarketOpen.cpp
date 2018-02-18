/**
 * @file server/channel/src/packets/game/BazaarMarketOpen.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request to open a market at a bazaar.
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

// object Includes
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <EventInstance.h>
#include <EventState.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"
#include "Zone.h"

using namespace channel;

bool Parsers::BazaarMarketOpen::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    int32_t maccaCost = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto zoneManager = server->GetZoneManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto zone = cState->GetZone();

    auto currentEvent = state->GetEventState()->GetCurrent();
    uint32_t marketID = currentEvent ? currentEvent->GetShopID() : 0;
    auto bazaar = currentEvent
        ? zone->GetBazaar(currentEvent->GetSourceEntityID()) : nullptr;

    bool success = marketID != 0 && bazaar;
    if(success && maccaCost > 0)
    {
        success = server->GetCharacterManager()->PayMacca(client, (uint64_t)maccaCost);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BAZAAR_MARKET_OPEN);
    if(success)
    {
        uint32_t timeLeft = (uint32_t)(zone->GetDefinition()->GetBazaarMarketTime() * 60);
        uint32_t timestamp = (uint32_t)time(0);
        uint32_t expirationTime = (uint32_t)(timestamp + (uint32_t)timeLeft);

        auto worldDB = server->GetWorldDatabase();

        // Always reload
        auto bazaarData = objects::BazaarData::LoadBazaarDataByAccount(worldDB,
            state->GetAccountUID());

        bool isNew = bazaarData == nullptr;
        if(isNew)
        {
            bazaarData = libcomp::PersistentObject::New<objects::BazaarData>(true);
            bazaarData->SetAccount(state->GetAccountUID());
            bazaarData->SetNPCType(1);
        }

        bazaarData->SetCharacter(cState->GetEntity());
        bazaarData->SetZone(zone->GetDefinition()->GetID());
        bazaarData->SetChannelID(server->GetRegisteredChannel()->GetID());
        bazaarData->SetMarketID(marketID);
        bazaarData->SetState(objects::BazaarData::State_t::BAZAAR_PREPARING);

        bazaarData->SetExpiration(expirationTime);

        auto dbChanges = libcomp::DatabaseChangeSet::Create();
        if(isNew)
        {
            auto worldData = state->GetAccountWorldData().Get();
            worldData->SetBazaarData(bazaarData);

            dbChanges->Insert(bazaarData);
            dbChanges->Update(worldData);
        }
        else
        {
            dbChanges->Update(bazaarData);
        }

        if(!worldDB->ProcessChangeSet(dbChanges))
        {
            LOG_ERROR(libcomp::String("BazaarData failed to save: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            state->SetLogoutSave(false);
            client->Close();
            return true;
        }

        bazaar->SetCurrentMarket(marketID, bazaarData);

        zoneManager->SendBazaarMarketData(zone, bazaar, marketID);

        // Refresh markets in the same bazaar
        zoneManager->ExpireBazaarMarkets(zone, bazaar);

        reply.WriteS32Little((int32_t)timeLeft);
        reply.WriteS32Little(0);        // Success
    }
    else
    {
        reply.WriteS32Little(-1);
        reply.WriteS32Little(-1);   // Failure
    }

    connection->SendPacket(reply);

    return true;
}
