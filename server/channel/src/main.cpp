/**
 * @file server/channel/src/main.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Main channel server file.
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

// channel Includes
#include "ChannelHandler.h"
#include "ChannelServer.h"

// libcomp Includes
#include <Log.h>
#include <WorldConnection.h>

// Civet Includes
#include <CivetServer.h>

#include <thread>

int main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    std::vector<std::string> options;
    options.push_back("listening_ports");
    options.push_back("10998");

    CivetServer webServer(options);
    webServer.addHandler("/", new channel::ChannelHandler);

    LOG_INFO("COMP_hack Channel Server v0.0.1 build 1\n");
    LOG_INFO("Copyright (C) 2010-2016 COMP_hack Team\n\n");

	LOG_INFO("Connecting to the World Server...\n");

	asio::io_service service;

	std::thread serviceThread([&service]()
	{
		service.run();
	});

	libcomp::WorldConnection connection(service);
	connection.Connect("127.0.0.1", false, 10667);

	auto worldStatus = connection.GetStatus();
	if (worldStatus == libcomp::TcpConnection::STATUS_CONNECTED)
	{
		LOG_INFO("World Server connection successful\n");

		channel::ChannelServer server("any", 10665);
		return server.Start();
	}
	else
	{
		LOG_INFO("World Server connection failed\n");
		service.stop();
		return -1;
	}

    channel::ChannelServer server("any", 10665);

    return server.Start();
}
