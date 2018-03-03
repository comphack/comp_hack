/**
 * @file tools/capfilter/src/ShopFilter.h
 * @ingroup capfilter
 *
 * @author HACKfrost
 *
 * @brief Packet filter dialog.
 *
 * Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
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

#include "ShopFilter.h"

// libcomp Includes
#include <DataStore.h>
#include <PacketCodes.h>

// C++11 Includes
#include <iostream>

// object Includes
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiShopProductData.h>
#include <ServerShop.h>
#include <ServerShopTab.h>
#include <ServerShopProduct.h>

ShopFilter::ShopFilter(const char *szProgram,
    const libcomp::String& dataStorePath)
{
    libcomp::DataStore store(szProgram);

    if(!store.AddSearchPath(dataStorePath))
    {
        std::cerr << "Failed to add search path." << std::endl;
    }

    if(!mDefinitions.LoadItemData(&store))
    {
        std::cerr << "Failed to load item data." << std::endl;
    }

    if(!mDefinitions.LoadShopProductData(&store))
    {
        std::cerr << "Failed to load shop product data." << std::endl;
    }
}

ShopFilter::~ShopFilter()
{
}

bool ShopFilter::ProcessCommand(const libcomp::String& capturePath,
    uint16_t commandCode, libcomp::ReadOnlyPacket& packet)
{
    (void)capturePath;

    switch(commandCode)
    {
    case to_underlying(ChannelToClientPacketCode_t::PACKET_SHOP_DATA):
        {
            if(8 > packet.Left())
            {
                std::cerr << "Bad shop data packet found." << std::endl;

                return false;
            }

            int32_t shopID = packet.ReadS32Little();
            int32_t cacheID = packet.ReadS32Little();
            (void)cacheID;

            if(packet.Left() < 17)
            {
                return true;
            }

            auto shop = std::make_shared<objects::ServerShop>();
            shop->SetShopID(shopID);

            shop->SetShop1(packet.ReadU16Little());
            shop->SetRepairCostMultiplier(packet.ReadFloat());
            shop->SetRepairRate(packet.ReadFloat());
            shop->SetLNCAdjust(packet.ReadU8() == 1);
            shop->SetLNCCenter(packet.ReadFloat());
            shop->SetShop5(packet.ReadU8());

            int8_t tabCount = packet.ReadS8();
            for(int8_t i = 0; i < tabCount; i++)
            {
                if(packet.Left() < 5 || packet.PeekU16() == 0 ||
                    (uint32_t)(packet.PeekU16() - 2) > packet.Left())
                {
                    std::cerr << "Malformed shop data packet tab found."
                        << std::endl;

                    return true;
                }

                auto shopTab = std::make_shared<objects::ServerShopTab>();

                shopTab->SetName(packet.ReadString16(
                    libcomp::Convert::ENCODING_UTF8, true));

                uint8_t tab1 = packet.ReadU8();
                shopTab->SetTab1(tab1);

                if(tab1 != 0)
                {
                    uint16_t tab2 = packet.ReadU16Little();
                    shopTab->SetTab2(tab2);

                    if(tab2 != 0)
                    {
                        shopTab->SetTab3(packet.ReadU16Little());
                    }
                }

                int8_t productCount = packet.ReadS8();
                for(int8_t k = 0; k < productCount; k++)
                {
                    if(packet.Left() < 9)
                    {
                        std::cerr << "Malformed shop data packet product found."
                            << std::endl;

                        return true;
                    }

                    auto product = std::make_shared<objects::ServerShopProduct>();
                    uint16_t productID = packet.ReadU16Little();

                    product->SetProductID(productID);
                    product->SetMerchantDescription(packet.ReadU8());

                    uint8_t flags = packet.ReadU8();
                    int32_t price = packet.ReadS32Little();
                    uint8_t trend = packet.ReadU8();

                    /// @todo: figure out what each of these represents
                    uint8_t exByteCount = 0;
                    if(flags & 0x01)
                    {
                        exByteCount++;
                    }

                    if(flags & 0x20)
                    {
                        exByteCount = (uint8_t)(exByteCount + 2);
                    }

                    if(flags & 0x40)
                    {
                        exByteCount = (uint8_t)(exByteCount + 2);
                    }

                    for(uint8_t ii = 0; ii < exByteCount; ii++)
                    {
                        product->AppendExtraBytes(packet.ReadU8());
                    }

                    /// @todo: do something with price variances
                    if(trend != 0)
                    {
                        auto shopProd = mDefinitions.GetShopProductData(productID);
                        auto item = shopProd != nullptr
                            ? mDefinitions.GetItemData(shopProd->GetItem()) : nullptr;

                        if(!shopProd || !item)
                        {
                            std::cerr << "Unknown shop product encountered."
                                << std::endl;

                            return true;
                        }

                        if(shopProd->GetCPCost() != 0)
                        {
                            price = item->GetBasic()->GetBuyPrice();
                        }
                    }

                    product->SetFlags(flags);
                    product->SetBasePrice(price);
                    shopTab->AppendProducts(product);
                }

                shop->AppendTabs(shopTab);
            }

            mShops[(uint32_t)shop->GetShopID()].push_back(shop);
        }
        break;
    default:
        break;
    }

    return true;
}

bool ShopFilter::PostProcess()
{
    for(auto shopPair : mShops)
    {
        /// @todo: merge products or set up min max trend costs?

        // Grab the last known one
        auto shopDef = shopPair.second.back();

        std::ostringstream ss;
        ss << std::setw(3) << std::setfill('0') << shopPair.first;
        libcomp::String fileName = libcomp::String("shop-%1").Arg(ss.str());

        {
            tinyxml2::XMLDocument doc;

            tinyxml2::XMLElement *pRoot = doc.NewElement("objects");
            doc.InsertEndChild(pRoot);

            if(!shopDef->Save(doc, *pRoot))
            {
                return false;
            }

            if(tinyxml2::XML_NO_ERROR != doc.SaveFile(libcomp::String(
                "shop-%1.xml").Arg(fileName).C()))
            {
                std::cerr << "Failed to save shop XML file." << std::endl;

                return false;
            }
        }
    }

    return true;
}
