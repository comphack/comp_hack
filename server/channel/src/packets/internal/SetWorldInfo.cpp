/**
 * @file server/channel/src/packets/SetWorldInfo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Response packet from the world detailing itself to the channel.
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
#include <DatabaseConfigCassandra.h>
#include <DatabaseConfigSQLite3.h>
#include <Decrypt.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <WorldDescription.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

std::shared_ptr<libcomp::Database> ParseDatabase(const std::shared_ptr<ChannelServer>& server,
    libcomp::ReadOnlyPacket& p)
{
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
        LOG_CRITICAL("No valid database connection configuration was found"
            " that matches the configured type.\n");
        return nullptr;
    }
    
    libcomp::EnumMap<objects::ServerConfig::DatabaseType_t,
        std::shared_ptr<objects::DatabaseConfig>> configMap;
    configMap[databaseType] = dbConfig;

    return server->GetDatabase(configMap, false);
}

bool Parsers::SetWorldInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    auto desc = server->GetWorldDescription();

    if(!desc->LoadPacket(p))
    {
        return false;
    }

    auto worldDatabase = ParseDatabase(server, p);
    if(nullptr == worldDatabase)
    {
        LOG_CRITICAL("World Server supplied database configuration could not"
            " be initialized as a valid database.\n");
        return false;
    }
    server->SetWorldDatabase(worldDatabase);
    
    auto lobbyDatabase = ParseDatabase(server, p);
    if(nullptr == lobbyDatabase)
    {
        LOG_CRITICAL("World Server supplied lobby database configuration could not"
            " be initialized as a database.\n");
        return false;
    }
    server->SetLobbyDatabase(lobbyDatabase);

    LOG_DEBUG(libcomp::String("Updating World Server description: (%1) %2\n")
        .Arg(desc->GetID()).Arg(desc->GetName()));

    //Reply with the channel information
    libcomp::Packet reply;

    reply.WritePacketCode(
        InternalPacketCode_t::PACKET_SET_CHANNEL_INFO);
    server->GetDescription()->SavePacket(reply);

    connection->SendPacket(reply);

    return true;
}
