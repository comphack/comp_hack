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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include "AccountManager.h"
#include "ApiHandler.h"
#include "Config.h"
#include "ImportHandler.h"
#include "LobbyConfig.h"
#include "LobbyServer.h"
#include "LoginWebHandler.h"

// libcomp Includes
#include <Constants.h>
#include <Exception.h>
#include <Log.h>
#include <PersistentObject.h>
#include <ServerCommandLineParser.h>
#include <Shutdown.h>

// Civet Includes
#include <CivetServer.h>

#if defined(_WIN32) && defined(WIN32_SERV)
int ApplicationMain(int argc, const char *argv[])
#else
int main(int argc, const char *argv[])
#endif  // defined(_WIN32) && defined(WIN32_SERV)
{
  libcomp::Exception::RegisterSignalHandler();

  libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

  libcomp::Config::LogVersion("COMP_hack Lobby Server");

  std::string configPath =
      libcomp::BaseServer::GetDefaultConfigPath() + "lobby.xml";

  bool unitTestMode = false;

  // Command line argument parser.
  auto parser = std::make_shared<libcomp::ServerCommandLineParser>();

  // Parse the command line arguments.
  if (!parser->Parse(argc, argv)) {
    return EXIT_FAILURE;
  }

  // initialize x
  int x = 4;
  // set x
  x = 2;
  // use x
  if (x <= argc && parser->GetTestingEnabled()) {
    argc--;
    argv++;

    LogGeneralDebugMsg("Starting the lobby in unit test mode.\n");

    unitTestMode = true;
  }

  auto arguments = parser->GetStandardArguments();

  if (!arguments.empty()) {
    configPath = arguments.front().ToUtf8();

    LogGeneralDebug([&]() {
      return libcomp::String("Using custom config path %1\n").Arg(configPath);
    });

    size_t pos = configPath.find_last_of("\\/");
    if (std::string::npos != pos) {
      libcomp::BaseServer::SetConfigPath(
          configPath.substr(0, ((size_t)pos + 1)));
    }
  }

  auto config = std::make_shared<objects::LobbyConfig>();
  if (!libcomp::BaseServer::ReadConfig(config, configPath)) {
    LogGeneralWarningMsg(
        "Failed to load the lobby config file. Default values will be used.\n");
  }

  if (!libcomp::PersistentObject::Initialize()) {
    LogGeneralCriticalMsg(
        "One or more persistent object definition failed to load.\n");

    return EXIT_FAILURE;
  }
  auto server = std::make_shared<lobby::LobbyServer>(argv[0], config, parser,
                                                     unitTestMode);

  if (!server->Initialize()) {
    LogGeneralCriticalMsg("The server could not be initialized.\n");

    return EXIT_FAILURE;
  }

  bool useSSL = !std::dynamic_pointer_cast<objects::LobbyConfig>(config)
                     ->GetWebCertificate()
                     .IsEmpty();

  /// @todo Consider moving the web server.
  std::vector<std::string> options;
  options.push_back("listening_ports");
  options.push_back(
      std::to_string(std::dynamic_pointer_cast<objects::LobbyConfig>(config)
                         ->GetWebListeningPort()) +
      (useSSL ? "s" : ""));

  if (useSSL) {
    options.push_back("ssl_certificate");
    options.push_back(std::dynamic_pointer_cast<objects::LobbyConfig>(config)
                          ->GetWebCertificate()
                          .ToUtf8());
  }

  auto pLoginHandler = new lobby::LoginHandler(server->GetMainDatabase());
  pLoginHandler->SetAccountManager(server->GetAccountManager());
  pLoginHandler->SetConfig(
      std::dynamic_pointer_cast<objects::LobbyConfig>(config));

  auto pApiHandler = new lobby::ApiHandler(config, server);
  pApiHandler->SetAccountManager(server->GetAccountManager());

  auto pImportHandler = new lobby::ImportHandler(config, server);

  CivetServer *pWebServer = nullptr;

  try {
    pWebServer = new CivetServer(options);
    pWebServer->addHandler("/", pLoginHandler);
    pWebServer->addHandler("/api", pApiHandler);
    pWebServer->addHandler("/import", pImportHandler);
  } catch (const CivetException &e) {
    LogGeneralCritical([&]() {
      return libcomp::String(
                 "The lobby API server failed to start with the following "
                 "message: %1\n")
          .Arg(e.what());
    });

    return EXIT_FAILURE;
  }

  // Set this for the signal handler.
  libcomp::Shutdown::Configure(server.get());

  // Start the main server loop (blocks until done).
  int returnCode = server->Start();

  // Shut down the web server.
  delete pWebServer;
  pWebServer = nullptr;

  // Complete the shutdown process.
  libcomp::Shutdown::Complete();

  LogGeneralInfoMsg("Bye!\n");

#ifndef EXOTIC_PLATFORM
  // Stop the logger
  delete libcomp::Log::GetSingletonPtr();
#endif  // !EXOTIC_PLATFORM

  return returnCode;
}
