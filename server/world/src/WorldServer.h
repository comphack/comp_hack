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

// libcomp Includes
#include <TcpServer.h>

namespace world
{

class WorldServer : public libcomp::TcpServer
{
public:
    WorldServer(libcomp::String listenAddress, int16_t port);
    virtual ~WorldServer();

protected:
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);
};

} // namespace world

#endif // SERVER_WORLD_SRC_WORLDSERVER_H
