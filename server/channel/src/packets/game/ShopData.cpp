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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <ServerShop.h>
#include <ServerShopProduct.h>
#include <ServerShopTab.h>

// channel Includes
#include "ChannelServer.h"

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
    int32_t cacheID = p.ReadS32Little();
    (void)cacheID;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto shopData = server->GetServerDataManager()->GetShopData((uint32_t)shopID);

    if(shopData == nullptr)
    {
        LOG_ERROR(libcomp::String("Unknown shop encountered: %1\n").Arg(shopID));
        return true;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOP_DATA);
    reply.WriteS32Little(shopID);
    reply.WriteS32Little(1);    /// @todo: change cacheID when trends are working
    reply.WriteS32Little(shopData->GetShop1());
    reply.WriteS32Little(shopData->GetShop2());
    reply.WriteS32Little(shopData->GetShop3());
    reply.WriteU16Little(shopData->GetShop4());
    reply.WriteS8(shopData->GetShop5());
    reply.WriteS8(shopData->GetShop6());

    auto tabs = shopData->GetTabs();
    reply.WriteS8((int8_t)tabs.size());
    for(auto tab : tabs)
    {
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            tab->GetName(), true);
        reply.WriteU8(tab->GetTab1());

        if(tab->GetTab1() != 0)
        {
            reply.WriteU16Little(tab->GetTab2());
            if(tab->GetTab2() != 0)
            {
                reply.WriteU16Little(tab->GetTab3());
            }
        }

        auto products = tab->GetProducts();
        reply.WriteU8((uint8_t)products.size());
        for(auto product : products)
        {
            reply.WriteU16Little(product->GetProductID());
            reply.WriteU8(product->GetMerchantDescription());
            reply.WriteU8(product->GetFlags());

            /// @todo: implement trends
            int32_t basePrice = product->GetBasePrice();
            uint8_t trend = 0;
            if(trend == 0)
            {
                reply.WriteS32Little(basePrice > 0 ? basePrice : 1);
            }
            else if(trend == 1)
            {
                // Increased price
            }
            else if(trend == 2)
            {
                // Decreased price
            }
            reply.WriteU8(trend);

            for(auto extra : product->GetExtraBytes())
            {
                reply.WriteU8(extra);
            }
        }
    }

    client->SendPacket(reply);

    return true;
}
