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
#include <Hotbar.h>
#include <Item.h>
#include <ItemBox.h>

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
        charState->RecalculateStats();

        // If we don't have an active demon, set up the state anyway
        auto demonState = state->GetDemonState();
        demonState->SetDemon(character->GetActiveDemon());
        demonState->SetEntityID(server->GetNextEntityID());
        demonState->RecalculateStats();

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

    bool updateCharacter = false;
    if(character->GetProgress().IsNull())
    {
        auto progress = libcomp::PersistentObject::New<
            objects::CharacterProgress>();

        if(!progress->Register(progress) ||
            !progress->Insert(db) ||
            !character->SetProgress(progress))
        {
            return false;
        }
        updateCharacter = true;
    }
    else if(!character->LoadProgress(db))
    {
        return false;
    }

    // Item boxes
    bool addEquipmentToBox = false;
    if(character->GetItemBoxes(0).IsNull())
    {
        addEquipmentToBox = true;

        auto box = libcomp::PersistentObject::New<
            objects::ItemBox>();

        if(!box->Register(box) ||
            !box->Insert(db) || !character->SetItemBoxes(0, box))
        {
            return false;
        }

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

        updateCharacter = true;
    }

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

            if(addEquipmentToBox)
            {
                auto slot = equipmentBoxSlot++;
                equip->SetItemBox(defaultBox);
                equip->SetBoxSlot((int8_t)slot);
                defaultBox->SetItems(slot, equip);
            }
        }
    }

    if(addEquipmentToBox && !defaultBox->Update(db))
    {
        return false;
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

    // COMP
    int demonCount = 0;
    for(auto demon : character->GetCOMP())
    {
        if(!demon.IsNull())
        {
            if(!demon.Get(db) || !demon->LoadCoreStats(db))
            {
                return false;
            }
            demonCount++;

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

    if(character->LearnedSkillsCount() == 0)
    {
        //Equip
        character->AppendLearnedSkills(0x00001654);

        //Summon demon
        character->AppendLearnedSkills(0x00001648);

        //Store demon
        character->AppendLearnedSkills(0x00001649);

        updateCharacter = true;
    }

    //Hack to add a test demon
    if(demonCount == 0)
    {
        auto demon = libcomp::PersistentObject::New<
            objects::Demon>();
        demon->SetType(0x0239); //Jack Frost
        demon->SetLocked(false);

        auto ds = libcomp::PersistentObject::New<
            objects::EntityStats>();
        ds->SetMaxHP(999);
        ds->SetMaxMP(666);
        ds->SetHP(999);
        ds->SetMP(666);
        ds->SetLevel(1);
        ds->SetSTR(1);
        ds->SetMAGIC(2);
        ds->SetVIT(3);
        ds->SetINTEL(4);
        ds->SetSPEED(5);
        ds->SetLUCK(6);
        ds->SetCLSR(7);
        ds->SetLNGR(8);
        ds->SetSPELL(9);
        ds->SetSUPPORT(10);
        ds->SetPDEF(11);
        ds->SetMDEF(12);

        if(!ds->Register(ds) || !ds->Insert(db) ||
            !demon->SetCoreStats(ds) || !demon->Register(demon) ||
            !demon->Insert(db) || !character->SetCOMP(0, demon))
        {
            return false;
        }
        updateCharacter = true;
    }

    return !updateCharacter || character->Update(db);
}

bool AccountManager::LogoutCharacter(channel::ClientState* state)
{
    auto character = state->GetCharacterState()->GetCharacter().Get();

    /// @todo: detach character from anything using it

    std::list<std::shared_ptr<libcomp::PersistentObject>> saveList;
    saveList.push_back(character);
    saveList.push_back(character->GetCoreStats().Get());
    saveList.push_back(character->GetProgress().Get());
    /// @todo: logout information

    // Save items
    for(auto itemBox : character->GetItemBoxes())
    {
        if(!itemBox.IsNull())
        {
            saveList.push_back(itemBox.Get());
            for(auto item : itemBox->GetItems())
            {
                saveList.push_back(item.Get());
            }
        }
    }

    // Save materials
    for(auto material : character->GetMaterials())
    {
        saveList.push_back(material.Get());
    }

    // Save demons
    for(auto demon : character->GetCOMP())
    {
        if(!demon.IsNull())
        {
            saveList.push_back(demon.Get());
            saveList.push_back(demon->GetCoreStats().Get());
        }
    }

    // Save hotbars
    for(auto hotbar : character->GetHotbars())
    {
        saveList.push_back(hotbar.Get());
    }

    bool ok = true;
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();
    for(auto obj : saveList)
    {
        ok &= obj == nullptr || obj->Update(worldDB);
    }

    return ok;
}
