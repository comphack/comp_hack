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

#include "AccountManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <EventState.h>
#include <Expertise.h>
#include <FriendSettings.h>
#include <Hotbar.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <ServerZone.h>
#include <TradeSession.h>

// channel Includes
#include "ChannelServer.h"

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

        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN);
        request.WriteU32(sessionKey);
        request.WriteString16Little(libcomp::Convert::ENCODING_UTF8, username);

        worldConnection->SendPacket(request);
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
    auto character = login->GetCharacterLogin()->GetCharacter();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOGIN);

    if(InitializeCharacter(character, state))
    {
        // Set current session info to null
        state->GetEventState()->SetCurrent(nullptr);
        state->GetTradeSession()->SetOtherCharacterState(nullptr);

        // Get entity IDs for the character and demon
        auto charState = state->GetCharacterState();
        charState->SetEntity(character.Get());
        charState->SetEntityID(server->GetNextEntityID());
        charState->RecalculateStats(server->GetDefinitionManager());

        // If we don't have an active demon, set up the state anyway
        auto demonState = state->GetDemonState();
        demonState->SetEntity(character->GetActiveDemon().Get());
        demonState->SetEntityID(server->GetNextEntityID());
        demonState->RecalculateStats(server->GetDefinitionManager());

        state->Register();

        reply.WriteU32Little(1);

        state->SetLoggedIn(true);
    }
    else
    {
        LOG_ERROR(libcomp::String("User account could not be logged in:"
            " %1\n").Arg(account->GetUsername()));
        reply.WriteU32Little(static_cast<uint32_t>(-1));

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
    auto zoneManager = server->GetZoneManager();
    auto managerConnection = server->GetManagerConnection();
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
        zoneManager->LeaveZone(client, true);
    }

    if(!LogoutCharacter(state, delay))
    {
        LOG_ERROR(libcomp::String("Character %1 failed to save on account"
            " %2.\n").Arg(character->GetUUID().ToString())
            .Arg(account->GetUUID().ToString()));
    }
    else if(!delay)
    {
        LOG_DEBUG(libcomp::String("Logged out user: '%1'\n").Arg(
            account->GetUsername()));
    }

    if(!delay)
    {
        //Remove the connection if it hasn't been removed already.
        managerConnection->RemoveClientConnection(client);

        libcomp::ObjectReference<
            objects::Account>::Unload(account->GetUUID());

        libcomp::Packet p;
        p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
        p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
        p.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, account->GetUsername());
        managerConnection->GetWorldConnection()->SendPacket(p);
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

bool AccountManager::InitializeCharacter(libcomp::ObjectReference<
    objects::Character>& character, channel::ClientState* state)
{
    auto server = mServer.lock();
    auto db = server->GetWorldDatabase();

    if(character.IsNull() || !character.Get(db) ||
        !character->LoadCoreStats(db))
    {
        return false;
    }

    auto account = character->GetAccount();
    bool newCharacter = character->GetCoreStats()->GetLevel() == -1;
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    if(newCharacter)
    {
        bool isGM = account->GetIsGM();

        auto cs = character->GetCoreStats().Get();
        cs->SetLevel(1);
        characterManager->CalculateCharacterBaseStats(cs);
        cs->SetHP(cs->GetMaxHP());
        cs->SetMP(cs->GetMaxMP());

        // Create the character progress
        auto progress = libcomp::PersistentObject::New<
            objects::CharacterProgress>(true);
        progress->SetCharacter(character);
        
        // Max COMP slots if the account is a GM
        if(isGM)
        {
            progress->SetMaxCOMPSlots(10);
        }

        if(!progress->Insert(db) ||
            !character->SetProgress(progress))
        {
            return false;
        }

        auto fSettings = libcomp::PersistentObject::New<
            objects::FriendSettings>(true);
        fSettings->SetCharacter(character);

        if(!fSettings->Insert(db) ||
            !character->SetFriendSettings(fSettings))
        {
            return false;
        }

        // Create the COMP
        auto comp = libcomp::PersistentObject::New<
            objects::DemonBox>(true);
        comp->SetAccount(account);
        comp->SetCharacter(character);

        if(!comp->Insert(db) || !character->SetCOMP(comp))
        {
            return false;
        }

        // Create the inventory item box (the others can be lazy loaded later)
        auto box = libcomp::PersistentObject::New<
            objects::ItemBox>(true);
        box->SetAccount(account);
        box->SetCharacter(character);

        if(!box->Insert(db) || !character->SetItemBoxes(0, box))
        {
            return false;
        }

        //Hacks to add MAG, a test demon and some starting skills
        auto mag = characterManager->GenerateItem(SVR_CONST.ITEM_MAGNETITE,
            5000);
        mag->SetItemBox(box);
        mag->SetBoxSlot(49);

        if(!mag->Insert(db) || !box->SetItems(49, mag))
        {
            return false;
        }

        auto demon = characterManager->ContractDemon(character.Get(),
            definitionManager->GetDevilData(0x0239));    //Jack Frost
        if(nullptr == demon)
        {
            return false;
        }

        //Equip
        character->AppendLearnedSkills(0x00001654);

        //Summon demon
        character->AppendLearnedSkills(0x00001648);

        //Store demon
        character->AppendLearnedSkills(0x00001649);

        for(auto skillID : definitionManager->GetDefaultCharacterSkills())
        {
            character->AppendLearnedSkills(skillID);
        }

        if(!cs->Update(db))
        {
            return false;
        }
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
            return false;
        }
    }

    state->SetAccountWorldData(worldData);

    // Progress
    if(!character->LoadProgress(db))
    {
        return false;
    }

    // Friend Settings
    if(!character->LoadFriendSettings(db))
    {
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
            return false;
        }

        for(auto item : itemBox->GetItems())
        {
            if(item.IsNull()) continue;

            if(!item.Get(db))
            {
                return false;
            }

            state->SetObjectID(item->GetUUID(),
                server->GetNextObjectID());
        }
    }

    // Equipment
    size_t equipmentBoxSlot = 0;
    auto defaultBox = character->GetItemBoxes(0);
    for(auto equip : character->GetEquippedItems())
    {
        if(equip.IsNull()) continue;

        //If we already have an object ID, it's already loaded
        if(!state->GetObjectID(equip.GetUUID()))
        {
            if(!equip.Get(db))
            {
                return false;
            }

            state->SetObjectID(equip->GetUUID(),
                server->GetNextObjectID());
        }

        if(newCharacter)
        {
            auto def = definitionManager->GetItemData(equip->GetType());
            auto poss = def->GetPossession();
            equip->SetDurability(poss->GetDurability());
            equip->SetMaxDurability((int8_t)poss->GetDurability());

            auto slot = equipmentBoxSlot++;
            equip->SetItemBox(defaultBox);
            equip->SetBoxSlot((int8_t)slot);
            defaultBox->SetItems(slot, equip);
        }
    }

    // Materials
    for(auto material : character->GetMaterials())
    {
        if(!material.IsNull() && !material.Get(db))
        {
            return false;
        }
    }

    // Expertises
    for(auto expertise : character->GetExpertises())
    {
        if(!expertise.IsNull() && !expertise.Get(db))
        {
            return false;
        }
    }

    std::list<std::list<
        libcomp::ObjectReference<objects::StatusEffect>>> statusEffectSets;
    statusEffectSets.push_back(character->GetStatusEffects());
    
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
            return false;
        }
            
        for(auto demon : box->GetDemons())
        {
            if(demon.IsNull()) continue;

            if(!demon.Get(db) || !demon->LoadCoreStats(db))
            {
                return false;
            }

            for(auto iSkill : demon->GetInheritedSkills())
            {
                if(!iSkill.Get(db))
                {
                    return false;
                }
            }

            state->SetObjectID(demon->GetUUID(),
                server->GetNextObjectID());

            statusEffectSets.push_back(demon->GetStatusEffects());
        }
    }
    
    // Status effects
    for(auto seSet : statusEffectSets)
    {
        for(auto effect : seSet)
        {
            if(!effect.IsNull() && !effect.Get(db))
            {
                return false;
            }
        }
    }

    // Hotbar
    for(auto hotbar : character->GetHotbars())
    {
        if(!hotbar.IsNull() && !hotbar.Get(db))
        {
            return false;
        }
    }

    return !newCharacter ||
        (character->Update(db) && defaultBox->Update(db));
}

bool AccountManager::LogoutCharacter(channel::ClientState* state,
    bool delay)
{
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    bool ok = true;
    bool doSave = state->GetLogoutSave();
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();

    ok &= Cleanup<objects::EntityStats>(character
        ->GetCoreStats().Get(), worldDB, doSave, !delay);
    ok &= Cleanup<objects::CharacterProgress>(character
        ->GetProgress().Get(), worldDB, doSave, !delay);
    ok &= Cleanup<objects::FriendSettings>(character
        ->GetFriendSettings().Get(), worldDB, doSave, !delay);

    // Save items and boxes
    std::list<std::shared_ptr<objects::ItemBox>> allBoxes;
    for(auto itemBox : character->GetItemBoxes())
    {
        allBoxes.push_back(itemBox.Get());
    }

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
            ok &= Cleanup<objects::Item>(item.Get(), worldDB,
                doSave, !delay);
        }
        ok &= Cleanup<objects::ItemBox>(itemBox, worldDB,
            doSave, !delay);
    }

    // Save materials
    for(auto material : character->GetMaterials())
    {
        ok &= Cleanup<objects::Item>(material.Get(), worldDB,
            doSave, !delay);
    }

    // Save expertises
    for(auto expertise : character->GetExpertises())
    {
        ok &= Cleanup<objects::Expertise>(expertise.Get(),
            worldDB, doSave, !delay);
    }

    std::list<std::list<
        libcomp::ObjectReference<objects::StatusEffect >> > statusEffectSets;
    statusEffectSets.push_back(character->GetStatusEffects());

    // Save demon boxes, demons and stats
    std::list<std::shared_ptr<objects::DemonBox>> demonBoxes;
    demonBoxes.push_back(character->GetCOMP().Get());

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

            statusEffectSets.push_back(demon->GetStatusEffects());

            for(auto iSkill : demon->GetInheritedSkills())
            {
                ok &= Cleanup<objects::InheritedSkill>(iSkill.Get(),
                    worldDB, doSave, !delay);
            }

            ok &= Cleanup<objects::EntityStats>(demon
                ->GetCoreStats().Get(), worldDB, doSave,
                !delay);
            ok &= Cleanup<objects::Demon>(demon.Get(),
                worldDB, doSave, !delay);
        }

        ok &= Cleanup<objects::DemonBox>(box, worldDB, doSave,
            !delay);
    }
    
    // Status effects
    for(auto seSet : statusEffectSets)
    {
        for(auto effect : seSet)
        {
            if(!effect.IsNull())
            {
                // Unregister but do not save. Updating status
                // effects requires special handling
                ok &= Cleanup<objects::StatusEffect>(effect.Get(),
                    worldDB, false, !delay);
            }
        }
    }

    // Save hotbars
    for(auto hotbar : character->GetHotbars())
    {
        ok &= Cleanup<objects::Hotbar>(hotbar.Get(),
            worldDB, doSave, !delay);
    }

    // Save world data
    ok &= Cleanup<objects::AccountWorldData>(
        accountWorldData, worldDB, doSave, !delay);

    ok &= Cleanup<objects::Character>(character, worldDB, doSave,
        !delay);

    return ok;
}
