/**
 * @file server/channel/src/BazaarState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a bazaar on the channel.
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

#include "BazaarState.h"

// libcomp Includes
#include <DatabaseChangeSet.h>
#include <Log.h>

// objects Includes
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <BazaarItem.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Item.h>
#include <ItemBox.h>
#include <ServerZone.h>

// channel Includes
#include "ClientState.h"
#include "Zone.h"

using namespace channel;

BazaarState::BazaarState(const std::shared_ptr<objects::ServerBazaar>& bazaar)
    : EntityState<objects::ServerBazaar>(bazaar)
{
}

std::shared_ptr<objects::BazaarData> BazaarState::GetCurrentMarket(uint32_t marketID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mCurrentMarkets.find(marketID);
    return it != mCurrentMarkets.end() ? it->second : nullptr;
}

void BazaarState::SetCurrentMarket(uint32_t marketID, const std::shared_ptr<
    objects::BazaarData>& data)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(GetEntity()->MarketIDsContains(marketID))
    {
        mCurrentMarkets[marketID] = data;
    }
}

bool BazaarState::AddItem(channel::ClientState* state, int8_t slot, int64_t itemID,
    int32_t price, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges)
{
    auto worldData = state->GetAccountWorldData().Get();
    auto bazaarData = worldData->GetBazaarData().Get();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    std::lock_guard<std::mutex> lock(mLock);
    if(VerifyMarket(bazaarData))
    {   
        if(item && bazaarData->GetItems((size_t)slot).IsNull())
        {
            // Create the item
            auto bItem = libcomp::PersistentObject::New<objects::BazaarItem>(true);
            bItem->SetAccount(state->GetAccountUID());
            bItem->SetItem(item);
            bItem->SetType(item->GetType());
            bItem->SetStackSize(item->GetStackSize());
            bItem->SetCost((uint32_t)price);

            // Add it to the bazaar
            bazaarData->SetItems((size_t)slot, bItem);

            // Remove it from the old box
            auto box = std::dynamic_pointer_cast<objects::ItemBox>(
                libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
            if(box)
            {
                int8_t oldSlot = item->GetBoxSlot();
                box->SetItems((size_t)oldSlot, NULLUUID);
                dbChanges->Update(box);
            }

            item->SetBoxSlot(-1);
            item->SetItemBox(NULLUUID);

            dbChanges->Insert(bItem);
            dbChanges->Update(bazaarData);
            dbChanges->Update(item);

            return true;
        }
        else
        {
            LOG_ERROR(libcomp::String("Failed to add bazaar item to market"
                " belonging to account: %1\n").Arg(state->GetAccountUID().ToString()));
            return false;
        }
    }

    return false;
}

bool BazaarState::DropItemFromMarket(ClientState* state, int8_t srcSlot, int64_t itemID,
    int8_t destSlot, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges)
{
    auto worldData = state->GetAccountWorldData().Get();
    auto bazaarData = worldData->GetBazaarData().Get();
    if(bazaarData == nullptr)
    {
        return false;
    }

    auto eventState = state->GetEventState();
    uint32_t eventMarketID = (uint32_t)state->GetCurrentMenuShopID();

    auto bState = state->GetBazaarState();
    if(bState && eventMarketID == bazaarData->GetMarketID())
    {
        return bState->DropItem(state, srcSlot, itemID, destSlot, dbChanges);
    }
    else if(bazaarData->GetState() != objects::BazaarData::State_t::BAZAAR_INACTIVE)
    {
        LOG_ERROR("Remote DropItem request encountered for an active market\n");
        return false;
    }
    else
    {
        return DropItemInternal(state, srcSlot, itemID, destSlot, dbChanges);
    }
}

std::shared_ptr<objects::BazaarItem> BazaarState::TryBuyItem(ClientState* state,
    uint32_t marketID, int8_t slot, int64_t itemID, int32_t price)
{
    auto market = GetCurrentMarket(marketID);
    if(market == nullptr)
    {
        LOG_ERROR("BuyItem request encountered with invalid market ID\n");
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mLock);

    auto itemUUID = state->GetObjectUUID(itemID);

    auto bItem = market->GetItems((size_t)slot).Get();
    if(bItem == nullptr || itemUUID.IsNull() || bItem->GetItem().GetUUID() != itemUUID)
    {
        LOG_ERROR(libcomp::String("BuyItem request encountered with invalid item UID: %1\n")
            .Arg(itemUUID.ToString()));
        return nullptr;
    }

    if(bItem->GetCost() != (uint32_t)price)
    {
        LOG_ERROR(libcomp::String("BuyItem request encountered with invalid price: "
            "Expected %1, found %2\n").Arg(price).Arg(bItem->GetCost()));
        return nullptr;
    }

    if(bItem->GetSold())
    {
        LOG_ERROR(libcomp::String("BuyItem request encountered for already sold item: %1\n")
            .Arg(itemUUID.ToString()));
        return nullptr;
    }

    return bItem;
}

bool BazaarState::BuyItem(std::shared_ptr<objects::BazaarItem> bItem)
{
    std::lock_guard<std::mutex> lock(mLock);
    if(bItem->GetSold())
    {
        return false;
    }

    bItem->SetSold(true);
    return true;
}

bool BazaarState::VerifyMarket(const std::shared_ptr<objects::BazaarData>& data)
{
    auto market = mCurrentMarkets.find(data != nullptr ? data->GetMarketID() : 0);
    if(market == mCurrentMarkets.end() || market->second != data)
    {
        LOG_ERROR(libcomp::String("Market '%1' does not match the supplied definition"
            " for bazaar %2\n").Arg(data != nullptr ? data->GetMarketID() : 0)
            .Arg(GetEntity()->GetID()));
        return false;
    }

    return true;
}

bool BazaarState::DropItem(ClientState* state, int8_t srcSlot, int64_t itemID,
    int8_t destSlot, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges)
{
    auto worldData = state->GetAccountWorldData().Get();
    auto bazaarData = worldData->GetBazaarData().Get();

    std::lock_guard<std::mutex> lock(mLock);
    if(VerifyMarket(bazaarData))
    {
        return DropItemInternal(state, srcSlot, itemID, destSlot, dbChanges);
    }

    return false;
}

bool BazaarState::DropItemInternal(ClientState* state, int8_t srcSlot, int64_t itemID,
    int8_t destSlot, std::shared_ptr<libcomp::DatabaseChangeSet>& dbChanges)
{
    auto worldData = state->GetAccountWorldData().Get();
    auto bazaarData = worldData->GetBazaarData().Get();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(itemID)));

    auto bItem = bazaarData->GetItems((size_t)srcSlot).Get();
    if(item == nullptr || bItem == nullptr || bItem->GetItem().Get() != item)
    {
        LOG_ERROR("DropItem request encountered with invalid item or"
            " source slot\n");
        return false;
    }

    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    if(!inventory->GetItems((size_t)destSlot).IsNull())
    {
        LOG_ERROR("DropItem request encountered with invalid"
            " destination slot\n");
        return false;
    }
    else
    {
        bazaarData->SetItems((size_t)srcSlot, NULLUUID);

        inventory->SetItems((size_t)destSlot, item);
        item->SetBoxSlot(destSlot);
        item->SetItemBox(inventory->GetUUID());

        dbChanges->Delete(bItem);
        dbChanges->Update(bazaarData);
        dbChanges->Update(inventory);
        dbChanges->Update(item);

        return true;
    }
}
