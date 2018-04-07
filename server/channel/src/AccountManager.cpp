/**
 * @file server/channel/src/AccountManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages accounts on the channel.
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

#include "AccountManager.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <Log.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <BazaarItem.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <Clan.h>
#include <ClanMember.h>
#include <DemonBox.h>
#include <EventState.h>
#include <Expertise.h>
#include <FriendSettings.h>
#include <Hotbar.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <Quest.h>
#include <ServerZone.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

AccountManager::AccountManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

AccountManager::~AccountManager()
{
}

void AccountManager::HandleLoginRequest(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const libcomp::String& username, uint32_t sessionKey)
{
    auto server = mServer.lock();
    auto worldConnection = server->GetManagerConnection()->GetWorldConnection();

    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    auto account = objects::Account::LoadAccountByUsername(lobbyDB, username);

    if(nullptr != account)
    {
        auto state = client->GetClientState();
        auto login = state->GetAccountLogin();
        login->SetAccount(account);
        login->SetSessionKey(sessionKey);

        server->GetManagerConnection()->SetClientConnection(client);

        LOG_DEBUG(libcomp::String("Logging in account '%1' with session key"
            " %2\n").Arg(username).Arg(login->GetSessionKey()));

        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);
        request.WriteU32(sessionKey);
        request.WriteString16Little(libcomp::Convert::ENCODING_UTF8, username);

        worldConnection->SendPacket(request);
    }
    else
    {
        LOG_ERROR(libcomp::String("Account '%1' not found. "
            "Can't log them in.\n").Arg(username));
    }
}

void AccountManager::HandleLoginResponse(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();
    auto state = client->GetClientState();
    auto login = state->GetAccountLogin();
    auto account = login->GetAccount();
    auto cLogin = login->GetCharacterLogin();
    auto character = cLogin->GetCharacter();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOGIN);

    if(InitializeCharacter(character, state))
    {
        auto definitionManager = server->GetDefinitionManager();
        auto demon = character->GetActiveDemon().Get();

        // Get entity IDs for the character and demon
        auto cState = state->GetCharacterState();
        cState->SetEntity(character.Get(), nullptr);
        cState->SetEntityID(server->GetNextEntityID());

        // If we don't have an active demon, set up the state anyway
        auto dState = state->GetDemonState();
        dState->SetEntity(demon, demon
            ? definitionManager->GetDevilData(demon->GetType()) : nullptr);
        dState->SetEntityID(server->GetNextEntityID());
        dState->RefreshLearningSkills(0, definitionManager);

        // Initialize some run-time data
        cState->RecalcEquipState(definitionManager);
        cState->RecalcDisabledSkills(definitionManager);

        // Prepare active quests
        server->GetEventManager()->UpdateQuestTargetEnemies(client);

        state->Register();

        dState->UpdateSharedState(character.Get(), definitionManager);

        // Recalculating the character will recalculate the partner too
        server->GetTokuseiManager()->Recalculate(cState, true,
            std::set<int32_t>{ cState->GetEntityID(), dState->GetEntityID() });

        cState->RecalculateStats(definitionManager);
        dState->RecalculateStats(definitionManager);

        reply.WriteU32Little(1);

        state->SetLoggedIn(true);
    }
    else
    {
        LOG_ERROR(libcomp::String("User account could not be logged in:"
            " %1\n").Arg(account->GetUsername()));
        reply.WriteU32Little(static_cast<uint32_t>(-1));

        state->SetLogoutSave(false);
        LogoutCharacter(state);

        // Tell the world that the character login failed without performing
        // any logout save actions etc
        libcomp::Packet p;
        p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
        p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
        p.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, account->GetUsername());
        server->GetManagerConnection()->GetWorldConnection()->SendPacket(p);
    }

    client->SendPacket(reply);
}

void AccountManager::HandleLogoutRequest(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    LogoutCode_t code, uint8_t channelIdx)
{
    switch(code)
    {
        case LogoutCode_t::LOGOUT_CODE_QUIT:
            {
                // No need to tell the world, just disconnect
                libcomp::Packet reply;
                reply.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_LOGOUT);
                reply.WriteU32Little(
                    (uint32_t)LogoutPacketAction_t::LOGOUT_PREPARE);
                client->QueuePacket(reply);

                reply.Clear();
                reply.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_LOGOUT);
                reply.WriteU32Little(
                    (uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
                client->SendPacket(reply);
            }
            break;
        case LogoutCode_t::LOGOUT_CODE_SWITCH:
            {
                // Tell the world we're performing a channel switch and
                // wait for the message to be responded to
                auto account = client->GetClientState()->GetAccountLogin()->GetAccount();

                libcomp::Packet p;
                p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
                p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_CHANNEL_SWITCH);
                p.WriteString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_UTF8, account->GetUsername());
                p.WriteS8((int8_t)channelIdx);
                mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(p);
            }
            break;
        default:
            break;
    }
}

void AccountManager::Logout(const std::shared_ptr<
    channel::ChannelClientConnection>& client, bool delay)
{
    auto server = mServer.lock();
    auto state = client->GetClientState();
    auto account = state->GetAccountLogin()->GetAccount().Get();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(nullptr == account || nullptr == character)
    {
        return;
    }

    auto zone = cState->GetZone();
    if(nullptr != zone)
    {
        character->SetLogoutZone(zone->GetDefinition()->GetID());
        character->SetLogoutX(cState->GetCurrentX());
        character->SetLogoutY(cState->GetCurrentY());
        character->SetLogoutRotation(cState->GetCurrentRotation());
        server->GetZoneManager()->LeaveZone(client, true);
    }

    if(!delay)
    {
        if(!LogoutCharacter(state))
        {
            LOG_ERROR(libcomp::String("Character %1 failed to save on account"
                " %2.\n").Arg(character->GetUUID().ToString())
                .Arg(account->GetUUID().ToString()));
        }

        LOG_DEBUG(libcomp::String("Logged out user: '%1'\n").Arg(
            account->GetUsername()));

        // Remove the connection if it hasn't been removed already.
        server->GetManagerConnection()->RemoveClientConnection(client);

        libcomp::ObjectReference<
            objects::Account>::Unload(account->GetUUID());

        // Remove all secondary caching
        server->GetTokuseiManager()->RemoveTrackingEntities(
            state->GetWorldCID());
    }
}

void AccountManager::Authenticate(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_AUTH);

    if(nullptr != state)
    {
        state->SetAuthenticated(true);
        reply.WriteU32Little(0);
    }
    else
    {
        reply.WriteU32Little(static_cast<uint32_t>(-1));
    }

    client->SendPacket(reply);
}

bool AccountManager::IncreaseCP(const std::shared_ptr<
    objects::Account>& account, int64_t addAmount)
{
    if(addAmount <= 0)
    {
        return false;
    }

    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();

    auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();
    auto expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
    expl->Add<int64_t>("CP", addAmount);
    opChangeset->AddOperation(expl);

    if(lobbyDB->ProcessChangeSet(opChangeset))
    {
        auto syncManager = server->GetChannelSyncManager();
        if(syncManager->UpdateRecord(account, "Account"))
        {
            syncManager->SyncOutgoing();
        }

        return true;
    }

    return false;
}

void AccountManager::SendCPBalance(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();

    // Always reload the account to get the latest CP value
    auto account = libcomp::PersistentObject::LoadObjectByUUID<objects::Account>(
        mServer.lock()->GetLobbyDatabase(), state->GetAccountUID(), true);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CASH_BALANCE);
    reply.WriteS32Little((int32_t)account->GetCP());
    reply.WriteS32Little(0);

    client->SendPacket(reply);
}

bool AccountManager::InitializeCharacter(libcomp::ObjectReference<
    objects::Character>& character, channel::ClientState* state)
{
    auto server = mServer.lock();
    auto db = server->GetWorldDatabase();

    if(character.IsNull() || !character.Get(db) ||
        !character->LoadCoreStats(db))
    {
        LOG_ERROR(libcomp::String("Character or character stats could"
            " not be initialized for account: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    auto account = character->GetAccount();
    bool newCharacter = character->GetCoreStats()->GetLevel() == -1;
    if(newCharacter && !InitializeNewCharacter(character.Get()))
    {
        LOG_ERROR(libcomp::String("Failed to initialize new character"
            " for account: %1\n").Arg(state->GetAccountUID().ToString()));
        return false;
    }

    // Load or create the account world data
    auto worldData = objects::AccountWorldData
        ::LoadAccountWorldDataByAccount(db, account);
    if(worldData == nullptr)
    {
        worldData = libcomp::PersistentObject::New<
            objects::AccountWorldData>(true);

        worldData->SetAccount(account);

        auto itemDepo = libcomp::PersistentObject::New<
            objects::ItemBox>(true);

        itemDepo->SetType(objects::ItemBox::Type_t::ITEM_DEPO);
        itemDepo->SetAccount(account);
        worldData->SetItemBoxes(0, itemDepo);

        auto demonDepo = libcomp::PersistentObject::New<
            objects::DemonBox>(true);

        demonDepo->SetAccount(account);
        demonDepo->SetBoxID(1);
        worldData->SetDemonBoxes(0, demonDepo);

        if(!itemDepo->Insert(db) || !demonDepo->Insert(db) ||
            !worldData->Insert(db))
        {
            LOG_ERROR(libcomp::String("AccountWorldData could not be"
                " created during character initialization for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }
    }

    state->SetAccountWorldData(worldData);

    // Bazaar
    if(!worldData->GetBazaarData().IsNull())
    {
        if(!worldData->LoadBazaarData(db))
        {
            LOG_ERROR(libcomp::String("BazaarData could"
                " not be initialized for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }

        auto bazaarData = worldData->GetBazaarData().Get();

        // Load all bazaar items together
        auto allBazaarItems = objects::BazaarItem::
            LoadBazaarItemListByAccount(db, account);

        // Check to make sure all items in slots in BazaarData are valid
        std::set<size_t> openSlots;
        std::set<std::shared_ptr<objects::BazaarItem>> loaded;
        for(size_t i = 0; i < 15; i++)
        {
            auto bItem = bazaarData->GetItems(i);

            if(bItem.IsNull())
            {
                openSlots.insert(i);
                continue;
            }

            if(!bItem.Get())
            {
                LOG_WARNING(libcomp::String("Clearing invalid"
                    " BazaarItem saved on BazaarData for account: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                bazaarData->SetItems(i, NULLUUID);
                openSlots.insert(i);
                continue;
            }

            state->SetObjectID(bItem->GetItem().GetUUID(),
                server->GetNextObjectID());

            loaded.insert(bItem.Get());
        }

        // Recover any orphaned items
        if(openSlots.size() > 0)
        {
            uint8_t recovered = 0;
            for(auto bItem : allBazaarItems)
            {
                if(loaded.find(bItem) == loaded.end())
                {
                    size_t idx = *openSlots.begin();
                    openSlots.erase(idx);

                    bazaarData->SetItems(idx, bItem);
                    recovered++;

                    if(openSlots.size() == 0) break;
                }
            }

            if(recovered > 0)
            {
                LOG_WARNING(libcomp::String("Recovered %1 orphaned"
                    " BazaarItem(s) from account: %2\n").Arg(recovered)
                    .Arg(state->GetAccountUID().ToString()));
            }
        }
    }

    // Progress
    if(!character->LoadProgress(db))
    {
        LOG_ERROR(libcomp::String("CharacterProgress could"
            " not be initialized for account: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    // Friend Settings
    if(!character->LoadFriendSettings(db))
    {
        LOG_ERROR(libcomp::String("FriendSettings could"
            " not be initialized for account: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    // Item boxes and items
    std::list<libcomp::ObjectReference<objects::ItemBox>> allBoxes;
    for(auto itemBox : character->GetItemBoxes())
    {
        allBoxes.push_back(itemBox);
    }

    for(auto itemBox : worldData->GetItemBoxes())
    {
        allBoxes.push_back(itemBox);
    }

    for(auto itemBox : allBoxes)
    {
        if(itemBox.IsNull()) continue;

        if(!itemBox.Get(db))
        {
            LOG_ERROR(libcomp::String("ItemBox could"
                " not be initialized for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }

        // Load all items together
        auto allBoxItems = objects::Item::LoadItemListByItemBox(db,
            itemBox.GetUUID());

        // Check to make sure all items in slots in the ItemBox are valid
        std::set<size_t> openSlots;
        std::set<std::shared_ptr<objects::Item>> loaded;
        for(size_t i = 0; i < 50; i++)
        {
            auto item = itemBox->GetItems(i);

            if(item.IsNull())
            {
                openSlots.insert(i);
                continue;
            }

            if(!item.Get(db) || item->GetItemBox() != itemBox.GetUUID())
            {
                LOG_WARNING(libcomp::String("Clearing invalid"
                    " Item saved on ItemBox for account: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                itemBox->SetItems(i, NULLUUID);
                openSlots.insert(i);
                continue;
            }

            state->SetObjectID(item->GetUUID(),
                server->GetNextObjectID());

            loaded.insert(item.Get());
        }

        // Recover any orphaned items
        if(openSlots.size() > 0)
        {
            uint8_t recovered = 0;
            for(auto item : allBoxItems)
            {
                if(loaded.find(item) == loaded.end())
                {
                    size_t idx = *openSlots.begin();
                    openSlots.erase(idx);

                    itemBox->SetItems(idx, item);
                    item->SetBoxSlot((int8_t)idx);
                    recovered++;

                    if(openSlots.size() == 0) break;
                }
            }

            if(recovered > 0)
            {
                LOG_WARNING(libcomp::String("Recovered %1 orphaned"
                    " Item(s) from account: %2\n").Arg(recovered)
                    .Arg(state->GetAccountUID().ToString()));
            }
        }
    }

    // Equipment
    for(auto equip : character->GetEquippedItems())
    {
        if(equip.IsNull()) continue;

        //If we already have an object ID, it's already loaded
        if(!state->GetObjectID(equip.GetUUID()))
        {
            if(!equip.Get(db))
            {
                LOG_ERROR(libcomp::String("Equipped Item could"
                    " not be initialized for account: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                return false;
            }

            state->SetObjectID(equip->GetUUID(),
                server->GetNextObjectID());
        }
    }

    // Expertises
    for(auto expertise : character->GetExpertises())
    {
        if(!expertise.IsNull() && !expertise.Get(db))
        {
            LOG_ERROR(libcomp::String("Expertise could"
                " not be initialized for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }
    }

    // Character status effects
    if(character->StatusEffectsCount() > 0)
    {
        int32_t seCount = (int32_t)character->StatusEffectsCount();
        for(int32_t i = seCount - 1; i >= 0; i--)
        {
            auto effect = character->GetStatusEffects((size_t)i);
            if(effect.IsNull() || !effect.Get(db))
            {
                LOG_WARNING(libcomp::String("Removing invalid"
                    " character StatusEffect saved for account: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                character->RemoveStatusEffects((size_t)i);
            }
        }
    }

    // Demon boxes, demons and stats
    std::list<libcomp::ObjectReference<objects::DemonBox>> demonBoxes;
    demonBoxes.push_back(character->GetCOMP());
    for(auto box : worldData->GetDemonBoxes())
    {
        demonBoxes.push_back(box);
    }

    for(auto box : demonBoxes)
    {
        if(box.IsNull()) continue;

        if(!box.Get(db))
        {
            LOG_ERROR(libcomp::String("DemonBox could"
                " not be initialized for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }

        for(auto demon : box->GetDemons())
        {
            if(demon.IsNull()) continue;

            if(!demon.Get(db) || !demon->LoadCoreStats(db))
            {
                LOG_ERROR(libcomp::String("Demon or demon stats could"
                    " not be initialized for account: %1\n")
                    .Arg(state->GetAccountUID().ToString()));
                return false;
            }

            for(auto iSkill : demon->GetInheritedSkills())
            {
                if(!iSkill.Get(db))
                {
                    LOG_ERROR(libcomp::String("InheritedSkill could"
                        " not be initialized for account: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                    return false;
                }
            }

            state->SetObjectID(demon->GetUUID(),
                server->GetNextObjectID());

            // Demon status effects
            if(demon->StatusEffectsCount() > 0)
            {
                int32_t seCount = (int32_t)demon->StatusEffectsCount();
                for(int32_t i = seCount - 1; i >= 0; i--)
                {
                    auto effect = demon->GetStatusEffects((size_t)i);
                    if(effect.IsNull() || !effect.Get(db))
                    {
                        LOG_WARNING(libcomp::String("Removing invalid"
                            " demon StatusEffect saved for account: %1\n")
                            .Arg(state->GetAccountUID().ToString()));
                        demon->RemoveStatusEffects((size_t)i);
                    }
                }
            }
        }
    }

    // If the active demon is somehow not valid, clear it
    if(!character->GetActiveDemon().IsNull() &&
        character->GetActiveDemon().Get() == nullptr)
    {
        LOG_WARNING(libcomp::String("Unassigning unknown"
            " active demon from character: %1\n")
            .Arg(character->GetUUID().ToString()));
        character->SetActiveDemon(NULLUUID);
    }

    // Hotbar
    for(auto hotbar : character->GetHotbars())
    {
        if(!hotbar.IsNull() && !hotbar.Get(db))
        {
            LOG_ERROR(libcomp::String("Hotbar could"
                " not be initialized for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }
    }

    // Quests
    for(auto qPair : character->GetQuests())
    {
        if(!qPair.second.IsNull() && !qPair.second.Get(db))
        {
            LOG_ERROR(libcomp::String("Quest could"
                " not be initialized for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }
    }

    // Clan
    if(!character->GetClan().IsNull())
    {
        if(!character->LoadClan(db))
        {
            LOG_ERROR(libcomp::String("Clan could"
                " not be initialized for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
            return false;
        }
    }

    return !newCharacter || character->Update(db);
}

bool AccountManager::InitializeNewCharacter(std::shared_ptr<
    objects::Character> character)
{
    auto cs = character->GetCoreStats().Get();
    if(cs->GetLevel() != -1)
    {
        return false;
    }

    auto server = mServer.lock();
    auto db = server->GetWorldDatabase();

    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto defaultObjs = server->GetDefaultCharacterObjectMap();

    auto objIter = defaultObjs.find("Character");
    auto dCharacter = std::dynamic_pointer_cast<objects::Character>
        ((objIter != defaultObjs.end())
            ? objIter->second.front() : nullptr);
    if(dCharacter)
    {
        // Set (selective) custom character values
        character->SetLNC(dCharacter->GetLNC());
        character->SetPoints(dCharacter->GetPoints());
        character->SetExpertiseExtension(
            dCharacter->GetExpertiseExtension());
        character->SetHomepointZone(dCharacter->GetHomepointZone());
        character->SetHomepointSpotID(dCharacter->GetHomepointSpotID());
        character->SetLoginPoints(dCharacter->GetLoginPoints());
        character->SetLearnedSkills(dCharacter->GetLearnedSkills());
        character->SetEquippedVA(dCharacter->GetEquippedVA());
        character->SetMaterials(dCharacter->GetMaterials());
        character->SetVACloset(dCharacter->GetVACloset());

        // Set expertise defaults
        for(size_t i = 0; i < dCharacter->ExpertisesCount(); i++)
        {
            auto dExp = dCharacter->GetExpertises(i).Get();
            if(dExp)
            {
                auto exp = std::make_shared<objects::Expertise>(*dExp);
                exp->Register(exp, libobjgen::UUID::Random());

                if(!exp->Insert(db) || !character->SetExpertises(i, exp))
                {
                    return false;
                }
            }
        }
    }

    // Generate stats
    auto dStats = dCharacter
        ? dCharacter->GetCoreStats().Get() : nullptr;
    if(dStats)
    {
        // Using custom stats
        cs->SetSTR(dStats->GetSTR());
        cs->SetMAGIC(dStats->GetMAGIC());
        cs->SetVIT(dStats->GetVIT());
        cs->SetINTEL(dStats->GetINTEL());
        cs->SetSPEED(dStats->GetSPEED());
        cs->SetLUCK(dStats->GetLUCK());

        // Correct level
        int8_t level = dStats->GetLevel();
        if(level < 1)
        {
            level = 1;
        }
        else if(level > 99)
        {
            level = 99;
        }
        cs->SetLevel(level);
    }
    else
    {
        // Using normal stats
        cs->SetLevel(1);
    }

    // Calculate secondary stats and set default HP
    characterManager->CalculateCharacterBaseStats(cs);
    cs->SetHP(cs->GetMaxHP());
    cs->SetMP(cs->GetMaxMP());

    // Create the character progress
    std::shared_ptr<objects::CharacterProgress> progress;

    objIter = defaultObjs.find("CharacterProgress");
    auto dProgress = std::dynamic_pointer_cast<objects::CharacterProgress>
        ((objIter != defaultObjs.end())
            ? objIter->second.front() : nullptr);
    if(dProgress)
    {
        // Using custom progress
        progress = std::make_shared<objects::CharacterProgress>(
            *dProgress);
        progress->Register(progress, libobjgen::UUID::Random());
    }
    else
    {
        // Using normal progress
        progress = libcomp::PersistentObject::New<
            objects::CharacterProgress>(true);
    }

    progress->SetCharacter(character->GetUUID());

    if(!progress->Insert(db) ||
        !character->SetProgress(progress))
    {
        return false;
    }

    // Create the inventory item box (the others can be lazy loaded later)
    auto box = libcomp::PersistentObject::New<
        objects::ItemBox>(true);
    box->SetAccount(character->GetAccount());
    box->SetCharacter(character->GetUUID());

    // Load and (properly) initialize equipment
    size_t inventorySlotUsed = 0;
    for(auto equip : character->GetEquippedItems())
    {
        if(equip.IsNull()) continue;

        if(!equip.Get(db))
        {
            return false;
        }

        auto def = definitionManager->GetItemData(equip->GetType());
        auto poss = def->GetPossession();
        equip->SetDurability((uint16_t)
            ((uint16_t)poss->GetDurability() * 1000));
        equip->SetMaxDurability((int8_t)poss->GetDurability());

        size_t slot = inventorySlotUsed++;
        equip->SetItemBox(box->GetUUID());
        equip->SetBoxSlot((int8_t)slot);

        if(!equip->Update(db) || !box->SetItems(slot, equip))
        {
            return false;
        }
    }

    // Add any custom equipment
    std::set<std::shared_ptr<objects::Item>> itemsAdded;
    if(dCharacter)
    {
        for(size_t i = 0; i < 15; i++)
        {
            auto dEquip = dCharacter->GetEquippedItems(i).Get();
            if(!dEquip) continue;

            itemsAdded.insert(dEquip);

            // Generate equipment then modify from custom
            auto equipCopy = characterManager->GenerateItem(
                dEquip->GetType(), 1);
            equipCopy->SetTarot(dEquip->GetTarot());
            equipCopy->SetSoul(dEquip->GetSoul());
            equipCopy->SetBasicEffect(dEquip->GetBasicEffect());
            equipCopy->SetSpecialEffect(dEquip->GetSpecialEffect());
            equipCopy->SetModSlots(dEquip->GetModSlots());
            equipCopy->SetFuseBonuses(dEquip->GetFuseBonuses());
            equipCopy->SetRentalExpiration(dEquip->GetRentalExpiration());

            auto def = definitionManager->GetItemData(equipCopy->GetType());

            size_t slot = inventorySlotUsed++;
            equipCopy->SetItemBox(box->GetUUID());
            equipCopy->SetBoxSlot((int8_t)slot);

            if(!equipCopy->Insert(db) || !box->SetItems(slot, equipCopy) ||
                !character->SetEquippedItems(
                    (size_t)def->GetBasic()->GetEquipType(), equipCopy))
            {
                return false;
            }
        }
    }

    // Add any additional items
    for(auto dObj : defaultObjs["Item"])
    {
        auto dItem = std::dynamic_pointer_cast<objects::Item>(dObj);

        if(itemsAdded.find(dItem) != itemsAdded.end()) continue;

        if(inventorySlotUsed >= 50) break;

        itemsAdded.insert(dItem);

        // Generate item then modify from custom
        auto itemCopy = characterManager->GenerateItem(
            dItem->GetType(), dItem->GetStackSize());

        itemCopy->SetTarot(dItem->GetTarot());
        itemCopy->SetSoul(dItem->GetSoul());
        itemCopy->SetBasicEffect(dItem->GetBasicEffect());
        itemCopy->SetSpecialEffect(dItem->GetSpecialEffect());
        itemCopy->SetModSlots(dItem->GetModSlots());
        itemCopy->SetFuseBonuses(dItem->GetFuseBonuses());
        itemCopy->SetRentalExpiration(dItem->GetRentalExpiration());

        size_t slot = inventorySlotUsed++;
        itemCopy->SetItemBox(box->GetUUID());
        itemCopy->SetBoxSlot((int8_t)slot);

        if(!itemCopy->Insert(db) || !box->SetItems(slot, itemCopy))
        {
            return false;
        }
    }

    // Insert/set the inventory
    if(!box->Insert(db) || !character->SetItemBoxes(0, box))
    {
        return false;
    }

    // Create the COMP
    auto comp = libcomp::PersistentObject::New<
        objects::DemonBox>(true);
    comp->SetAccount(character->GetAccount());
    comp->SetCharacter(character->GetUUID());

    // Generate demons and add to the COMP
    uint8_t compSlotUsed = 0;
    for(auto dObj : defaultObjs["Demon"])
    {
        auto dDemon = std::dynamic_pointer_cast<objects::Demon>(dObj);

        if(compSlotUsed >= progress->GetMaxCOMPSlots()) break;

        auto devilData = definitionManager->GetDevilData(
            dDemon->GetType());

        if(!devilData) continue;

        // Generate demon then modify from custom
        auto demonCopy = characterManager->GenerateDemon(devilData);

        demonCopy->SetSoulPoints(dDemon->GetSoulPoints());
        demonCopy->SetFamiliarity(dDemon->GetFamiliarity());
        demonCopy->SetAcquiredSkills(dDemon->GetAcquiredSkills());

        // Override learned skills if any are specified
        for(auto skillID : dDemon->GetLearnedSkills())
        {
            if(skillID)
            {
                demonCopy->SetLearnedSkills(dDemon->GetLearnedSkills());
                break;
            }
        }

        // If an explicit level is set, recalc (do not set stats too
        // because these are calculated per level)
        dStats = dDemon->GetCoreStats().Get();
        auto copyStats = demonCopy->GetCoreStats().Get();
        if(dStats && copyStats->GetLevel() != dStats->GetLevel())
        {
            // Correct level
            int8_t level = dStats->GetLevel();
            if(level < 1)
            {
                level = 1;
            }
            else if(level > 99)
            {
                level = 99;
            }
            copyStats->SetLevel(level);

            // Recalc
            copyStats->SetLevel(dStats->GetLevel());
            characterManager->CalculateDemonBaseStats(demonCopy,
                copyStats, devilData);
        }

        uint8_t slot = compSlotUsed++;
        demonCopy->SetDemonBox(comp->GetUUID());
        demonCopy->SetBoxSlot((int8_t)slot);

        if(!demonCopy->Insert(db) || !copyStats->Insert(db) ||
           !comp->SetDemons((uint8_t)slot, demonCopy))
        {
            return false;
        }
    }

    // Insert/set the COMP
    if(!comp->Insert(db) || !character->SetCOMP(comp))
    {
        return false;
    }

    if(dCharacter)
    {
        // Set hotbar defaults
        for(size_t i = 0; i < 10; i++)
        {
            auto dBar = dCharacter->GetHotbars(i).Get();
            if(dBar)
            {
                auto bar = std::make_shared<objects::Hotbar>(*dBar);
                bar->Register(bar, libobjgen::UUID::Random());

                if(!bar->Insert(db) || !character->SetHotbars(i, bar))
                {
                    return false;
                }
            }
        }
    }

    // Set (non-customizable) friend settings
    auto fSettings = libcomp::PersistentObject::New<
        objects::FriendSettings>(true);
    fSettings->SetCharacter(character->GetUUID());

    if(!fSettings->Insert(db) ||
        !character->SetFriendSettings(fSettings))
    {
        return false;
    }

    // Lastly update the core stats and character to signify that
    // initialization has completed
    if(!cs->Update(db) || !character->Update(db))
    {
        return false;
    }

    return true;
}

bool AccountManager::LogoutCharacter(channel::ClientState* state)
{
    // If something failed and the state should not save on logout
    // quit here
    if(!state->GetLogoutSave())
    {
        return true;
    }

    // Retrieve the character from the character login as it will
    // not be set on the character state unless a successul login
    // has already occurred
    auto character = state->GetAccountLogin()->GetCharacterLogin()
        ->GetCharacter().Get();

    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        character->GetAccount());

    std::list<std::shared_ptr<objects::ItemBox>> allBoxes;
    if(character)
    {
        dbChanges->Update(character->GetCoreStats().Get());
        dbChanges->Update(character->GetProgress().Get());
        dbChanges->Update(character->GetFriendSettings().Get());

        for(auto itemBox : character->GetItemBoxes())
        {
            allBoxes.push_back(itemBox.Get());
        }
    }

    // Save items and boxes
    auto accountWorldData = state->GetAccountWorldData().Get();
    if(accountWorldData)
    {
        for(auto itemBox : accountWorldData->GetItemBoxes())
        {
            allBoxes.push_back(itemBox.Get());
        }
    }

    for(auto itemBox : allBoxes)
    {
        if(!itemBox) continue;

        for(auto item : itemBox->GetItems())
        {
            dbChanges->Update(item.Get());
        }

        dbChanges->Update(itemBox);
    }

    std::list<std::shared_ptr<objects::DemonBox>> demonBoxes;
    if(character)
    {
        // Save expertises
        for(auto expertise : character->GetExpertises())
        {
            dbChanges->Update(expertise.Get());
        }

        demonBoxes.push_back(character->GetCOMP().Get());
    }

    // Save demon boxes, demons and stats
    if(accountWorldData)
    {
        for(auto box : accountWorldData->GetDemonBoxes())
        {
            demonBoxes.push_back(box.Get());
        }
    }

    for(auto box : demonBoxes)
    {
        if(!box) continue;

        for(auto demon : box->GetDemons())
        {
            if(demon.IsNull()) continue;

            for(auto iSkill : demon->GetInheritedSkills())
            {
                dbChanges->Update(iSkill.Get());
            }

            dbChanges->Update(demon->GetCoreStats().Get());
            dbChanges->Update(demon.Get());
        }

        dbChanges->Update(box);
    }

    // Save world data
    dbChanges->Update(accountWorldData);

    // Do not save status effects as those handled uniquely elsewhere

    // Do not save clan information as it is managed by the server

    if(character)
    {
        // Save hotbars
        for(auto hotbar : character->GetHotbars())
        {
            dbChanges->Update(hotbar.Get());
        }

        // Save quests
        for(auto qPair : character->GetQuests())
        {
            dbChanges->Update(qPair.second.Get());
        }

        dbChanges->Update(character);
    }

    // Save all records at once
    return mServer.lock()->GetWorldDatabase()
        ->ProcessChangeSet(dbChanges);
}
