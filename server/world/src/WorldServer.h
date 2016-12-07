/**
 * @file server/world/src/WorldServer.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief World server class.
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

#ifndef SERVER_WORLD_SRC_WORLDSERVER_H
#define SERVER_WORLD_SRC_WORLDSERVER_H

 // Standard C++11 Includes
#include <map>

// libcomp Includes
#include <BaseServer.h>
#include <ChannelDescription.h>
#include <InternalConnection.h>
#include <ManagerConnection.h>
#include <WorldDescription.h>
#include <Worker.h>

namespace world
{

class WorldServer : public libcomp::BaseServer
{
public:
    WorldServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath);
    virtual ~WorldServer();

    objects::WorldDescription GetDescription();

    bool GetChannelDescriptionByConnection(std::shared_ptr<libcomp::InternalConnection> connection, objects::ChannelDescription& outChannel);

    std::shared_ptr<libcomp::InternalConnection> GetLobbyConnection();

    void SetChannelDescription(objects::ChannelDescription channel, std::shared_ptr<libcomp::InternalConnection> connection);

protected:
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /// @todo Replace this with many worker threads.
    libcomp::Worker mWorker;

    objects::WorldDescription mDescription;

    std::map<std::shared_ptr<libcomp::InternalConnection>, objects::ChannelDescription> mChannelDescriptions;

    std::shared_ptr<ManagerConnection> mManagerConnection;
};

} // namespace world

#endif // SERVER_WORLD_SRC_WORLDSERVER_H
