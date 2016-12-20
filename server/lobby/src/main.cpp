/**
 * @file server/lobby/src/main.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main lobby server file.
 *
 * This file is part of the Lobby Server (lobby).
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

// lobby Includes
#include "LoginWebHandler.h"
#include "LobbyConfig.h"
#include "LobbyServer.h"

// libcomp Includes
#include <Log.h>
#include <PersistentObject.h>
#include <Shutdown.h>

// Civet Includes
#include <CivetServer.h>

int main(int argc, const char *argv[])
{
    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    LOG_INFO("COMP_hack Lobby Server v0.0.1 build 1\n");
    LOG_INFO("Copyright (C) 2010-2016 COMP_hack Team\n\n");

    std::string configPath = libcomp::BaseServer::GetDefaultConfigPath() + "lobby.xml";

    if(argc == 2)
    {
        configPath = argv[1];
        LOG_DEBUG(libcomp::String("Using custom config path "
            "%1\n").Arg(configPath));
    }

    libcomp::PersistentObject::Initialize();

    auto config = std::shared_ptr<objects::ServerConfig>(new objects::LobbyConfig());
    auto server = std::shared_ptr<lobby::LobbyServer>(new lobby::LobbyServer(config, configPath));
    
    auto wkServer = std::weak_ptr<libcomp::BaseServer>(server);
    if(!server->Initialize(wkServer))
    {
        LOG_DEBUG("The server could not be initialized.\n");
        return EXIT_FAILURE;
    }

    std::vector<std::string> options;
    options.push_back("listening_ports");
    options.push_back(std::to_string(std::dynamic_pointer_cast<objects::LobbyConfig>(config)->GetWebListeningPort()));

    CivetServer webServer(options);
    webServer.addHandler("/", new lobby::LoginHandler);

    // Set this for the signal handler.
    libcomp::Shutdown::Configure(server.get());

    // Start the main server loop (blocks until done).
    int returnCode = server->Start();

    // Complete the shutdown process.
    libcomp::Shutdown::Complete();

    LOG_INFO("Bye!\n");

    return returnCode;
}
