/**
 * @file server/channel/src/packets/game/ShopData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client details about a shop.
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
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>
#include <random>

// object Includes
#include <EventInstance.h>
#include <EventState.h>
#include <MiShopProductData.h>
#include <ServerShop.h>
#include <ServerShopProduct.h>
#include <ServerShopTab.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"

using namespace channel;

bool Parsers::ShopData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 8)
    {
        return false;
    }

    int32_t shopID = p.ReadS32Little();
    int32_t clientTrendTime = p.ReadS32Little();
    (void)clientTrendTime;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto serverDataManager = server->GetServerDataManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);

    auto shopData = serverDataManager->GetShopData((uint32_t)shopID);
    if(!shopData)
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Unknown shop encountered: %1\n")
                .Arg(shopID);
        });

        return true;
    }

    auto cEvent = client->GetClientState()->GetEventState()->GetCurrent();

    std::set<uint8_t> disabledTabs;
    if(cEvent)
    {
        // Filter down tabs that are event condition restricted
        auto eventManager = server->GetEventManager();
        for(uint8_t i = 0; i < (uint8_t)shopData->TabsCount(); i++)
        {
            auto tab = shopData->GetTabs(i);
            if(tab->ConditionsCount() > 0 && !eventManager
                ->EvaluateEventConditions(client, tab->GetConditions()))
            {
                cEvent->InsertDisabledChoices(i);
                disabledTabs.insert(i);
            }
        }
    }

    // Multiple supported product flags listed below
    // 0x01: Mutiply the base price by an additional byte value (unused)
    // 0x02-0x10: Product will also show in numbered existing filter group tabs
    // 0x20: Apparently unsupported (needs 2 additional bytes)
    // 0x40: Product is only visible during moon phases matching extra bytes
    // 0x80: Apparently unsupported (needs 2 additional bytes)

    // Trends reset every 5 minutes based on the server system time
    uint32_t trendTime = server->GetWorldClockTime().SystemTime;
    trendTime = trendTime - (trendTime % 300);

    float trendAdjust = shopData->GetTrendAdjustment();
    if(shopData->GetType() == objects::ServerShop::Type_t::COMP_SHOP)
    {
        // COMP shops have no trends
        trendAdjust = 0.f;
    }

    std::unordered_map<uint32_t, std::pair<uint8_t, int32_t>> productTrends;

    // Seed the (repeatable) random number generators for trend calculation
    std::mt19937 rand, pRand;
    if(trendAdjust > 0.f)
    {
        rand.seed(trendTime);
        pRand.seed((uint32_t)(trendTime - 300));
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOP_DATA);
    reply.WriteS32Little(shopID);
    reply.WriteS32Little((int32_t)trendTime);

    switch(shopData->GetRepairType())
    {
    case objects::ServerShop::RepairType_t::WEAPON_ONLY:
        reply.WriteU16Little(0x0100);
        break;
    case objects::ServerShop::RepairType_t::ARMOR_ONLY:
        reply.WriteU16Little(0x0200);
        break;
    default:
    case objects::ServerShop::RepairType_t::ANY:
        reply.WriteU16Little(0);
        break;
    }

    reply.WriteFloat(shopData->GetRepairCostMultiplier());
    reply.WriteFloat(shopData->GetRepairRate());
    reply.WriteU8(shopData->GetLNCAdjust() ? 1 : 0);
    reply.WriteFloat(shopData->GetLNCCenter());
    reply.WriteU8(0);   // Deprecated ID/flag

    reply.WriteS8((int8_t)(shopData->TabsCount() - disabledTabs.size()));
    for(uint8_t i = 0; i < (uint8_t)shopData->TabsCount(); i++)
    {
        auto tab = shopData->GetTabs(i);

        if(disabledTabs.find(i) != disabledTabs.end()) continue;

        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            tab->GetName(), true);

        // There used to be several other flags that were supported here
        // but they appear to have been disabled. Some required multiple
        // extra bytes: 2 for 0x3X, 0x5X, 0x9X; 4 for 0x7X, 0xBX, 0xDX;
        // 6 for 0xFX
        reply.WriteU8(tab->GetFilterGroup());

        auto products = tab->GetProducts();
        reply.WriteU8((uint8_t)products.size());
        for(auto product : products)
        {
            reply.WriteU16Little(product->GetProductID());
            reply.WriteU8(product->GetMerchantDescription());

            // Get product flags
            uint8_t flags = 0;
            if(product->GetFilterGroups())
            {
                flags = (uint8_t)(product->GetFilterGroups() << 1);
            }

            bool moonRestricted = product->GetMoonRestrict() &&
                product->GetMoonRestrict() != 0xFFFF;
            if(moonRestricted)
            {
                flags |= 0x40;
            }

            reply.WriteU8(flags);

            int32_t price = product->GetBasePrice();
            uint8_t trend = 0;

            // If the product has already been seen, do not recalculate the
            // price and trend
            auto it = productTrends.find(product->GetProductID());
            if(it == productTrends.end())
            {
                // Calculate price and trend
                auto pData = definitionManager->GetShopProductData((uint32_t)
                    product->GetProductID());
                auto def = pData
                    ? definitionManager->GetItemData(pData->GetItem())
                    : nullptr;

                uint8_t pTrend = 0;
                if(trendAdjust > 0.f && !product->GetTrendDisabled() &&
                    !characterManager->IsCPItem(def))
                {
                    std::uniform_int_distribution<uint32_t> dis(0, 1000);
                    trend = (uint8_t)(dis(rand) % 3);
                    pTrend = (uint8_t)(dis(pRand) % 3);
                }

                if(trend == 1)
                {
                    // Increased price
                    price = (int32_t)floor((double)price *
                        (double)(1.f + trendAdjust + 0.005f));
                }
                else if(trend == 2)
                {
                    // Decreased price
                    price = (int32_t)ceil((double)price *
                        (double)(1.f - trendAdjust));
                }

                if(!trend)
                {
                    if(pTrend == 1)
                    {
                        // Decreased to normal
                        trend = 2;
                    }
                    else if(pTrend == 2)
                    {
                        // Increased to normal
                        trend = 1;
                    }
                }
                else if(trend == pTrend)
                {
                    // Do not actually send the trend as it has not updated
                    trend = 0;
                }

                productTrends[product->GetProductID()] = std::make_pair(trend,
                    price);
            }
            else
            {
                // Use already calculated price and trend
                trend = it->second.first;
                price = it->second.second;
            }

            reply.WriteS32Little(price > 0 ? price : 1);

            reply.WriteU8(trend);

            if(moonRestricted)
            {
                reply.WriteU16Little(product->GetMoonRestrict());
            }
        }
    }

    client->SendPacket(reply);

    return true;
}
