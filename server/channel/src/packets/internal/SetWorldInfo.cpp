/**
 * @file server/channel/src/packets/internal/SetWorldInfo.cpp
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
#include <DatabaseConfigMariaDB.h>
#include <DatabaseConfigSQLite3.h>
#include <Decrypt.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <ChannelConfig.h>

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
        case objects::ServerConfig::DatabaseType_t::MARIADB:
            dbConfig = std::shared_ptr<objects::DatabaseConfig>(
                new objects::DatabaseConfigMariaDB);
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

    auto worldID = p.ReadU8();
    auto channelID = p.ReadU8();
    auto otherChannelsExist = p.ReadU8() == 1;

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(server->GetConfig());
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

    auto svr = objects::RegisteredWorld::LoadRegisteredWorldByID(lobbyDatabase, worldID);
    if(nullptr == svr)
    {
        LOG_CRITICAL("World Server could not be loaded from the database.\n");
        return false;
    }

    LOG_DEBUG(libcomp::String("Updating World Server: (%1) %2\n")
        .Arg(svr->GetID()).Arg(svr->GetName()));

    server->RegisterWorld(svr);

    if(!server->RegisterServer(channelID))
    {
        LOG_CRITICAL("The server failed to register with the world's database.\n");
        return false;
    }

    // Build all global zone instances now that we've connected properly
    server->GetZoneManager()->InstanceGlobalZones();

    if(otherChannelsExist)
    {
        server->LoadAllRegisteredChannels();
    }

    //Reply with the channel information
    libcomp::Packet reply;

    reply.WritePacketCode(
        InternalPacketCode_t::PACKET_SET_CHANNEL_INFO);
    reply.WriteU8(server->GetRegisteredChannel()->GetID());

    connection->SendPacket(reply);

    // Now that we've connected to the world successfully, hit the first server tick
    // to start the main loop in addition to any recurring scheduled work
    server->Tick();

    if(conf->GetTimeout() > 0)
    {
        server->GetManagerConnection()->ScheduleClientTimeoutHandler(conf->GetTimeout());
    }

    return true;
}

bool Parsers::SetWorldInfo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    // Since this is called exactly once, if at any point the packet does
    // not parse properly, the channel cannot start and should be shutdown.
    if(!SetWorldInfoFromPacket(pPacketManager, connection, p))
    {
        auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
        server->Shutdown();
        return false;
    }

    return true;
}
