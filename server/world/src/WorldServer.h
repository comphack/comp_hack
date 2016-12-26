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

    virtual bool Initialize(std::weak_ptr<BaseServer>& self);

    const std::shared_ptr<objects::WorldDescription> GetDescription() const;

    std::shared_ptr<objects::ChannelDescription> GetChannelDescriptionByConnection(
        const std::shared_ptr<libcomp::InternalConnection>& connection) const;

    const std::shared_ptr<libcomp::InternalConnection> GetLobbyConnection() const;

    void SetChannelDescription(const std::shared_ptr<objects::ChannelDescription>& channel,
        const std::shared_ptr<libcomp::InternalConnection>& connection);

    bool RemoveChannelDescription(const std::shared_ptr<libcomp::InternalConnection>& connection);

protected:
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    std::shared_ptr<objects::WorldDescription> mDescription;

    std::map<std::shared_ptr<libcomp::InternalConnection>,
        std::shared_ptr<objects::ChannelDescription>> mChannelDescriptions;

    std::shared_ptr<ManagerConnection> mManagerConnection;
};

} // namespace world

#endif // SERVER_WORLD_SRC_WORLDSERVER_H
