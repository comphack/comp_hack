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
#include "ApiHandler.h"
#include "LoginWebHandler.h"
#include "LobbyConfig.h"
#include "LobbyServer.h"

// libcomp Includes
#include <Exception.h>
#include <Log.h>
#include <PersistentObject.h>
#include <Shutdown.h>

// Civet Includes
#include <CivetServer.h>

int main(int argc, const char *argv[])
{
    libcomp::Exception::RegisterSignalHandler();

    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    LOG_INFO("COMP_hack Lobby Server v0.0.1 build 1\n");
    LOG_INFO("Copyright (C) 2010-2016 COMP_hack Team\n\n");

    std::string configPath = libcomp::BaseServer::GetDefaultConfigPath() +
        "lobby.xml";

    bool unitTestMode = false;

    //initialize x
    int x = 4;
    //set x
    x = 2;
    //use x
    if(x <= argc && std::string("--test") == argv[1])
    {
        argc--;
        argv++;

        LOG_DEBUG("Starting the lobby in unit test mode.\n");

        unitTestMode = true;
    }

    if(2 == argc)
    {
        configPath = argv[1];

        LOG_DEBUG(libcomp::String("Using custom config path %1\n").Arg(
            configPath));

        size_t pos = configPath.find_last_of("\\/");
        if(std::string::npos != pos)
        {
            libcomp::BaseServer::SetConfigPath(
                configPath.substr(0, ((size_t)pos+1)));
        }
    }

    auto config = std::make_shared<objects::LobbyConfig>();
    if(!libcomp::BaseServer::ReadConfig(config, configPath))
    {
        LOG_WARNING("Failed to load the lobby config file."
            " Default values will be used.\n");
    }

    if(!libcomp::PersistentObject::Initialize())
    {
        LOG_CRITICAL("One or more persistent object definition "
            "failed to load.\n");

        return EXIT_FAILURE;
    }
    auto server = std::make_shared<lobby::LobbyServer>(
        argv[0], config, unitTestMode);

    if(!server->Initialize())
    {
        LOG_CRITICAL("The server could not be initialized.\n");

        return EXIT_FAILURE;
    }

    bool useSSL = !std::dynamic_pointer_cast<objects::LobbyConfig>(config
        )->GetWebCertificate().IsEmpty();

    /// @todo Consider moving the web server.
    std::vector<std::string> options;
    options.push_back("listening_ports");
    options.push_back(std::to_string(std::dynamic_pointer_cast<
        objects::LobbyConfig>(config)->GetWebListeningPort()) +
        (useSSL ? "s" : ""));

    if(useSSL)
    {
        options.push_back("ssl_certificate");
        options.push_back(std::dynamic_pointer_cast<
            objects::LobbyConfig>(config)->GetWebCertificate().ToUtf8());
    }

    auto pLoginHandler = new lobby::LoginHandler(server->GetMainDatabase());
    pLoginHandler->SetAccountManager(server->GetAccountManager());
    pLoginHandler->SetSessionManager(server->GetSessionManager());
    pLoginHandler->SetConfig(std::dynamic_pointer_cast<
        objects::LobbyConfig>(config));

    auto pApiHandler = new lobby::ApiHandler(config, server);

    CivetServer webServer(options);
    webServer.addHandler("/", pLoginHandler);
    webServer.addHandler("/api", pApiHandler);

    // Set this for the signal handler.
    libcomp::Shutdown::Configure(server.get());

    // Start the main server loop (blocks until done).
    int returnCode = server->Start();

    // Complete the shutdown process.
    libcomp::Shutdown::Complete();

    LOG_INFO("\rBye!\n");

    return returnCode;
}
