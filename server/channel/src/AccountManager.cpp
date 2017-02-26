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

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterProgress.h>
#include <Demon.h>
#include <EntityStats.h>
#include <Expertise.h>
#include <Hotbar.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemData.h>
#include <MiPossessionData.h>

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

    auto cid = login->GetCID();
    auto character = account->GetCharacters(cid);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOGIN);

    if(InitializeCharacter(character, state))
    {
        auto charState = state->GetCharacterState();
        charState->SetCharacter(character);
        charState->SetEntityID(server->GetNextEntityID());
        charState->RecalculateStats(server->GetDefinitionManager());

        // If we don't have an active demon, set up the state anyway
        auto demonState = state->GetDemonState();
        demonState->SetDemon(character->GetActiveDemon());
        demonState->SetEntityID(server->GetNextEntityID());
        demonState->RecalculateStats(server->GetDefinitionManager());

        reply.WriteU32Little(1);
    }
    else
    {
        LOG_ERROR(libcomp::String("User account could not be logged in:"
            " %1\n").Arg(account->GetUsername()));
        reply.WriteU32Little(static_cast<uint32_t>(-1));
    }

    client->SendPacket(reply);
}

void AccountManager::HandleLogoutRequest(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    LogoutCode_t code, uint8_t channel)
{
    std::list<libcomp::Packet> replies;
    switch(code)
    {
        case LogoutCode_t::LOGOUT_CODE_QUIT:
            {
                libcomp::Packet reply;
                reply.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_LOGOUT);
                reply.WriteU32Little((uint32_t)10);
                replies.push_back(reply);

                reply = libcomp::Packet();
                reply.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_LOGOUT);
                reply.WriteU32Little((uint32_t)13);
                replies.push_back(reply);
            }
            break;
        case LogoutCode_t::LOGOUT_CODE_SWITCH:
            (void)channel;
            /// @todo: handle switching code
            break;
        default:
            break;
    }

    for(libcomp::Packet& reply : replies)
    {
        client->SendPacket(reply);
    }
}

void AccountManager::Logout(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto managerConnection = server->GetManagerConnection();
    auto state = client->GetClientState();
    auto account = state->GetAccountLogin()->GetAccount().Get();
    auto character = state->GetCharacterState()->GetCharacter().Get();

    if(nullptr == account || nullptr == character)
    {
        return;
    }

    if(!LogoutCharacter(state))
    {
        LOG_ERROR(libcomp::String("Character %1 failed to save on account"
            " %2.\n").Arg(character->GetUUID().ToString())
            .Arg(account->GetUUID().ToString()));
    }
    else
    {
        LOG_DEBUG(libcomp::String("Logged out user: '%1'\n").Arg(
            account->GetUsername()));
    }

    //Remove the connection if it hasn't been removed already.
    managerConnection->RemoveClientConnection(client);

    libcomp::ObjectReference<
        objects::Account>::Unload(account->GetUUID());

    libcomp::Packet p;
    p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
    p.WriteString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, account->GetUsername());
    managerConnection->GetWorldConnection()->SendPacket(p);
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

    bool newCharacter = character->GetCoreStats()->GetLevel() == -1;
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    if(newCharacter)
    {
        auto cs = character->GetCoreStats().Get();
        cs->SetLevel(1);
        characterManager->CalculateCharacterBaseStats(cs);
        cs->SetHP(cs->GetMaxHP());
        cs->SetMP(cs->GetMaxMP());

        // Create the character progress
        auto progress = libcomp::PersistentObject::New<
            objects::CharacterProgress>();
        progress->SetCharacter(character);

        if(!progress->Register(progress) ||
            !progress->Insert(db) ||
            !character->SetProgress(progress))
        {
            return false;
        }

        // Create the inventory item box (the others can be lazy loaded later)
        auto box = libcomp::PersistentObject::New<
            objects::ItemBox>();
        box->SetAccount(character->GetAccount());
        box->SetCharacter(character);

        if(!box->Register(box) ||
            !box->Insert(db) || !character->SetItemBoxes(0, box))
        {
            return false;
        }

        //Hacks to add MAG, a test demon and some starting skills
        auto mag = libcomp::PersistentObject::New<
            objects::Item>();

        mag->SetType(800);
        mag->SetStackSize(5000);
        mag->SetItemBox(box);
        mag->SetBoxSlot(49);

        if(!mag->Register(mag) || !mag->Insert(db) ||
            !box->SetItems(49, mag))
        {
            return false;
        }

        auto demon = characterManager->ContractDemon(character.Get(),
            definitionManager->GetDevilData(0x0239),    //Jack Frost
            nullptr);
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
    }

    // Progress
    if(!character->LoadProgress(db))
    {
        return false;
    }

    // Item boxes
    for(auto itemBox : character->GetItemBoxes())
    {
        if(!itemBox.IsNull())
        {
            if(!itemBox.Get(db))
            {
                return false;
            }

            state->SetObjectID(itemBox->GetUUID(),
                server->GetNextObjectID());

            for(auto item : itemBox->GetItems())
            {
                if(!item.IsNull())
                {
                    if(!item.Get(db))
                    {
                        return false;
                    }

                    state->SetObjectID(item->GetUUID(),
                        server->GetNextObjectID());
                }
            }
        }
    }

    // Equipment
    size_t equipmentBoxSlot = 0;
    auto defaultBox = character->GetItemBoxes(0);
    for(auto equip : character->GetEquippedItems())
    {
        if(!equip.IsNull())
        {
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
                equip->SetMaxDurability(poss->GetDurability());

                auto slot = equipmentBoxSlot++;
                equip->SetItemBox(defaultBox);
                equip->SetBoxSlot((int8_t)slot);
                defaultBox->SetItems(slot, equip);
            }
        }
    }

    // Materials
    for(auto material : character->GetMaterials())
    {
        if(!material.IsNull())
        {
            if(!material.Get(db))
            {
                return false;
            }
        }
    }

    // Expertises
    for(auto expertise : character->GetExpertises())
    {
        if(!expertise.IsNull())
        {
            if(!expertise.Get(db))
            {
                return false;
            }
        }
    }

    // COMP
    for(auto demon : character->GetCOMP())
    {
        if(!demon.IsNull())
        {
            if(!demon.Get(db) || !demon->LoadCoreStats(db))
            {
                return false;
            }

            state->SetObjectID(demon->GetUUID(),
                server->GetNextObjectID());
        }
    }

    // Hotbar
    for(auto hotbar : character->GetHotbars())
    {
        if(!hotbar.IsNull())
        {
            if(!hotbar.Get(db))
            {
                return false;
            }
        }
    }

    return !newCharacter ||
        (character->Update(db) && defaultBox->Update(db));
}

bool AccountManager::LogoutCharacter(channel::ClientState* state)
{
    auto character = state->GetCharacterState()->GetCharacter().Get();

    /// @todo: detach character from anything using it

    bool ok = true;
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();

    ok &= Cleanup<objects::Character>(character, worldDB);
    ok &= Cleanup<objects::EntityStats>(character
        ->GetCoreStats().Get(), worldDB);
    ok &= Cleanup<objects::CharacterProgress>(character
        ->GetProgress().Get(), worldDB);
    /// @todo: logout information

    // Save items
    for(auto itemBox : character->GetItemBoxes())
    {
        if(!itemBox.IsNull())
        {
            for(auto item : itemBox->GetItems())
            {
                ok &= Cleanup<objects::Item>(item.Get(), worldDB);
            }
            ok &= Cleanup<objects::ItemBox>(itemBox.Get(), worldDB);
        }
    }

    // Save materials
    for(auto material : character->GetMaterials())
    {
        ok &= Cleanup<objects::Item>(material.Get(), worldDB);
    }

    // Save expertises
    for(auto expertise : character->GetExpertises())
    {
        ok &= Cleanup<objects::Expertise>(expertise.Get(), worldDB);
    }

    // Save demons
    for(auto demon : character->GetCOMP())
    {
        if(!demon.IsNull())
        {
            ok &= Cleanup<objects::EntityStats>(demon
                ->GetCoreStats().Get(), worldDB);
            ok &= Cleanup<objects::Demon>(demon.Get(), worldDB);
        }
    }

    // Save hotbars
    for(auto hotbar : character->GetHotbars())
    {
        ok &= Cleanup<objects::Hotbar>(hotbar.Get(), worldDB);
    }

    return ok;
}
