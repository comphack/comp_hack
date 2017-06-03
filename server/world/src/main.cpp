/**
 * @file server/world/src/main.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Main world server file.
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

// world Includes
#include "WorldConfig.h"
#include "WorldServer.h"

// libcomp Includes
#include <Exception.h>
#include <Log.h>
#include <PersistentObject.h>
#include <Shutdown.h>

int main(int argc, const char *argv[])
{
    libcomp::Exception::RegisterSignalHandler();

    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    LOG_INFO("COMP_hack World Server v0.0.1 build 1\n");
    LOG_INFO("Copyright (C) 2010-2016 COMP_hack Team\n\n");

    std::string configPath = libcomp::BaseServer::GetDefaultConfigPath() + "world.xml";

    if(argc == 2)
    {
        configPath = argv[1];
        LOG_DEBUG(libcomp::String("Using custom config path "
            "%1\n").Arg(configPath));
    }

    if(!libcomp::PersistentObject::Initialize())
    {
        LOG_CRITICAL("One or more persistent object definition failed to load.\n");
        return EXIT_FAILURE;
    }

    auto config = std::make_shared<objects::WorldConfig>();
    auto server = std::make_shared<world::WorldServer>(
        argv[0], config, configPath);

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
