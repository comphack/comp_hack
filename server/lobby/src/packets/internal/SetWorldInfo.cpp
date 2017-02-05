/**
 * @file server/lobby/src/packets/internal/SetWorldInfo.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Response packet from the world detailing itself to the lobby.
 *
 * This file is part of the Lobby Server (lobby).
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
#include <DatabaseConfigCassandra.h>
#include <DatabaseConfigSQLite3.h>
#include <Decrypt.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// lobby Includes
#include "LobbyServer.h"

using namespace lobby;

bool SetWorldInfoFromPacket(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p)
{
    if(p.Size() == 0)
    {
        LOG_DEBUG("World Server connection sent an empty response."
            "  The connection will be closed.\n");
        return false;
    }

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto mainDB = server->GetMainDatabase();

    auto svr = objects::RegisteredWorld::LoadRegisteredWorldByID(mainDB, p.ReadU8());

    if(nullptr == svr)
    {
        return false;
    }

    auto databaseType = server->GetConfig()->GetDatabaseType();

    // Read the configuration for the world's database
    std::shared_ptr<objects::DatabaseConfig> dbConfig;
    switch(databaseType)
    {
        case objects::ServerConfig::DatabaseType_t::CASSANDRA:
            dbConfig = std::shared_ptr<objects::DatabaseConfig>(
                new objects::DatabaseConfigCassandra);
            break;
        case objects::ServerConfig::DatabaseType_t::SQLITE3:
            dbConfig = std::shared_ptr<objects::DatabaseConfig>(
                new objects::DatabaseConfigSQLite3);
            break;
    }

    if(!dbConfig->LoadPacket(p, false))
    {
        LOG_CRITICAL("World Server did not supply a valid database connection configuration"
            " that matches the configured type.\n");
        return false;
    }
    
    libcomp::EnumMap<objects::ServerConfig::DatabaseType_t,
        std::shared_ptr<objects::DatabaseConfig>> configMap;
    configMap[databaseType] = dbConfig;

    auto worldDatabase = server->GetDatabase(configMap, false);
    if(nullptr == worldDatabase)
    {
        LOG_CRITICAL("World Server's database could not be initialized.\n");
        return false;
    }

    auto iConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(connection);

    if(nullptr == iConnection)
    {
        return false;
    }

    LOG_DEBUG(libcomp::String("Updating World Server: (%1) %2\n")
        .Arg(svr->GetID()).Arg(svr->GetName()));

    auto world = server->GetWorldByConnection(iConnection);
    world->SetWorldDatabase(worldDatabase);
    world->RegisterWorld(svr);

    server->RegisterWorld(world);

    return true;
}

bool Parsers::SetWorldInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    // Since this is called exactly once per world connection, if at any point
    // the packet does not parse properly, the world's connection needs to be
    // closed as it is not valid.
    if(!SetWorldInfoFromPacket(pPacketManager, connection, p))
    {
        connection->Close();
        return false;
    }

    return true;
}
