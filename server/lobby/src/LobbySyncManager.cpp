/**
 * @file server/lobby/src/LobbySyncManager.h
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Lobby specific implementation of the DataSyncManager in
 *  charge of performing server side update operations.
 *
 * This file is part of the Lobby Server (lobby).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "LobbySyncManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <algorithm>

// object Includes
#include <Account.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>

// lobby Includes
#include "AccountManager.h"
#include "LobbyServer.h"
#include "ManagerConnection.h"

using namespace lobby;

LobbySyncManager::LobbySyncManager(const std::weak_ptr<
    LobbyServer>& server) : mServer(server)
{
}

LobbySyncManager::~LobbySyncManager()
{
}

bool LobbySyncManager::Initialize()
{
    auto lobbyDB = mServer.lock()->GetMainDatabase();

    // Build the configs
    auto cfg = std::make_shared<ObjectConfig>(
        "Account", true, lobbyDB);
    cfg->UpdateHandler = &DataSyncManager::Update<LobbySyncManager,
        objects::Account>;
    cfg->DynamicHandler = true;

    mRegisteredTypes["Account"] = cfg;

    cfg = std::make_shared<ObjectConfig>(
        "Character", false, nullptr);
    cfg->UpdateHandler = &DataSyncManager::Update<LobbySyncManager,
        objects::Character>;
    cfg->DynamicHandler = true;

    mRegisteredTypes["Character"] = cfg;

    cfg = std::make_shared<ObjectConfig>(
        "CharacterProgress", false, nullptr);
    cfg->UpdateHandler = &DataSyncManager::Update<LobbySyncManager,
        objects::CharacterProgress>;
    cfg->DynamicHandler = true;

    mRegisteredTypes["CharacterProgress"] = cfg;

    return true;
}

namespace lobby
{
template<>
int8_t LobbySyncManager::Update<objects::Account>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;
    (void)isRemove;
    (void)source;

    auto entry = std::dynamic_pointer_cast<objects::Account>(obj);

    SyncAccount(entry);

    return SYNC_HANDLED;
}

template<>
int8_t LobbySyncManager::Update<objects::Character>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;
    (void)source;

    auto entry = std::dynamic_pointer_cast<objects::Character>(obj);

    SyncCharacter(entry, isRemove);

    return SYNC_HANDLED;
}

template<>
int8_t LobbySyncManager::Update<objects::CharacterProgress>(
    const libcomp::String& type, const std::shared_ptr<libcomp::Object>& obj,
    bool isRemove, const libcomp::String& source)
{
    (void)type;
    (void)source;

    // Do nothing with removes
    if(!isRemove)
    {
        auto entry = std::dynamic_pointer_cast<objects::CharacterProgress>(obj);

        SyncCharacterProgress(entry);
    }

    return SYNC_HANDLED;
}
}

void LobbySyncManager::SyncAccount(const std::shared_ptr<objects::Account>& account)
{
    auto server = mServer.lock();
    auto accountManager = server->GetAccountManager();

    auto accountLogin = accountManager->GetUserLogin(account->GetUsername());
    auto cLogin = accountLogin ? accountLogin->GetCharacterLogin() : nullptr;
    int8_t worldID = cLogin ? cLogin->GetWorldID() : -1;

    // If the account is currently logged into a world, sync the account with it
    if(worldID >= 0)
    {
        auto world = server->GetManagerConnection()->GetWorldByID(
            (uint8_t)worldID);
        if(world)
        {
            libcomp::Packet p;

            WriteOutgoingRecord(p, true, "Account", account);

            world->GetConnection()->SendPacket(p);
        }
    }
}

void LobbySyncManager::SyncCharacter(const std::shared_ptr<
    objects::Character>& character, bool isRemove)
{
    auto server = mServer.lock();

    uint8_t worldID = character->GetWorldID();

    auto world = server->GetManagerConnection()->GetWorldByID(
        worldID);
    if(world)
    {
        auto connection = world->GetConnection();

        std::set<std::shared_ptr<libcomp::Object>> updates;
        std::set<std::shared_ptr<libcomp::Object>> removes;
        if(isRemove)
        {
            removes.insert(character);
        }
        else
        {
            updates.insert(character);
        }

        QueueOutgoing("Character", connection, updates, removes);

        connection->FlushOutgoing();
    }
}

void LobbySyncManager::SyncCharacterProgress(const std::shared_ptr<
    objects::CharacterProgress>& progress)
{
    auto server = mServer.lock();

    auto character = progress ? std::dynamic_pointer_cast<objects::Character>(
        libcomp::PersistentObject::GetObjectByUUID(progress->GetCharacter()))
        : nullptr;
    if(character)
    {
        uint8_t worldID = character->GetWorldID();

        auto world = server->GetManagerConnection()->GetWorldByID(
            worldID);
        if(world)
        {
            auto connection = world->GetConnection();

            std::set<std::shared_ptr<libcomp::Object>> updates;
            std::set<std::shared_ptr<libcomp::Object>> removes;
            updates.insert(progress);

            QueueOutgoing("CharacterProgress", connection, updates, removes);

            connection->FlushOutgoing();
        }
    }
    else
    {
        LogDataSyncManagerDebug([&]()
        {
            return libcomp::String("CharacterProgress failed to sync for"
                " character: %1\n").Arg(progress->GetCharacter().ToString());
        });
    }
}
