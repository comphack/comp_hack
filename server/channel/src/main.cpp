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
#include "ChannelConfig.h"
#include "ChannelServer.h"

// libcomp Includes
#include <Exception.h>
#include <Log.h>
#include <PersistentObject.h>
#include <Shutdown.h>

int main(int argc, const char *argv[])
{
    libcomp::Exception::RegisterSignalHandler();

    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    LOG_INFO("COMP_hack Channel Server v0.0.1 build 1\n");
    LOG_INFO("Copyright (C) 2010-2016 COMP_hack Team\n\n");

    std::string configPath = libcomp::BaseServer::GetDefaultConfigPath() + "channel.xml";

    if(argc == 2)
    {
        configPath = argv[1];
        LOG_DEBUG(libcomp::String("Using custom config path "
            "%1\n").Arg(configPath));

        size_t pos = configPath.find_last_of("\\/");
        if(std::string::npos != pos)
        {
            libcomp::BaseServer::SetConfigPath(
                configPath.substr(0, ((size_t)pos+1)));
        }
    }

    auto config = std::make_shared<objects::ChannelConfig>();
    if(!libcomp::BaseServer::ReadConfig(config, configPath))
    {
        LOG_WARNING("Failed to load the channel config file."
            " Default values will be used.\n");
    }

    if(!libcomp::PersistentObject::Initialize())
    {
        LOG_CRITICAL("One or more persistent object definition failed to load.\n");
        return EXIT_FAILURE;
    }

    auto server = std::make_shared<channel::ChannelServer>(
        argv[0], config);

    if(!server->Initialize())
    {
        LOG_CRITICAL("The server could not be initialized.\n");
        return EXIT_FAILURE;
    }

    // Set this for the signal handler.
    libcomp::Shutdown::Configure(server.get());

    // Start the main server loop (blocks until done).
    int returnCode = server->Start();

    // Complete the shutdown process.
    libcomp::Shutdown::Complete();

    LOG_INFO("\rBye!\n");

    return returnCode;
}
