/**
 * @file server/channel/src/ChannelServer.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel server class.
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

#ifndef SERVER_CHANNEL_SRC_CHANNELSERVER_H
#define SERVER_CHANNEL_SRC_CHANNELSERVER_H

// libcomp Includes
#include <InternalConnection.h>
#include <BaseServer.h>
#include <ChannelDescription.h>
#include <ManagerConnection.h>
#include <Worker.h>

namespace channel
{

class ChannelServer : public libcomp::BaseServer
{
public:
    ChannelServer(std::shared_ptr<objects::ServerConfig> config, const libcomp::String& configPath);
    virtual ~ChannelServer();

    virtual void Shutdown();

    objects::ChannelDescription GetDescription();

    objects::WorldDescription GetWorldDescription();

    void SetWorldDescription(objects::WorldDescription& worldDescription);

protected:
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /// @todo Make a bunch of these.
    libcomp::Worker mWorker;

    std::shared_ptr<ManagerConnection> mManagerConnection;

    objects::WorldDescription mWorldDescription;

    objects::ChannelDescription mDescription;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELSERVER_H
