/**
 * @file server/world/src/packets/DescribeWorld.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle describing the world for the lobby.
 *
 * This file is part of the World Server (world).
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
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// world Includes
#include "WorldConfig.h"
#include "WorldServer.h"

using namespace world;

bool Parsers::DescribeWorld::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
    auto config = std::dynamic_pointer_cast<objects::WorldConfig>(server->GetConfig());
    auto databaseType = config->GetDatabaseType();

    bool fromLobby = server->GetLobbyConnection() == connection;
    if(fromLobby)
    {
        // The lobby will pass its database connection configurations
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
            LOG_CRITICAL("The lobby did not supply a valid database connection configuration"
                " for the current database type.\n");
            return false;
        }

        libcomp::EnumMap<objects::ServerConfig::DatabaseType_t,
            std::shared_ptr<objects::DatabaseConfig>> configMap;
        configMap[databaseType] = dbConfig;

        auto lobbyDatabase = server->GetDatabase(configMap, false);
        if(nullptr == lobbyDatabase)
        {
            return false;
        }

        server->SetLobbyDatabase(lobbyDatabase);
    }

    // Reply with a packet containing the world description and the database
    // connection configuration for the world.  If the packet was received from
    // a channel instead, the reply will contain the lobby database connection
    // information as well.
    libcomp::Packet reply;

    reply.WritePacketCode(InternalPacketCode_t::PACKET_SET_WORLD_DESCRIPTION);
    server->GetDescription()->SavePacket(reply);
    
    switch(databaseType)
    {
        case objects::ServerConfig::DatabaseType_t::CASSANDRA:
            config->GetCassandraConfig().Get()->SavePacket(reply, false);
            break;
        case objects::ServerConfig::DatabaseType_t::SQLITE3:
            config->GetSQLite3Config().Get()->SavePacket(reply, false);
            break;
    }

    if(!fromLobby)
    {
        server->GetLobbyDatabase()->GetConfig()->SavePacket(reply, false);
    }

    connection->SendPacket(reply);

    return true;
}
