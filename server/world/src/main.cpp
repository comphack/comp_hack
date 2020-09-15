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

// world Includes
#include "WorldConfig.h"
#include "WorldServer.h"

// libcomp Includes
#include <Config.h>
#include <Constants.h>
#include <Exception.h>
#include <Log.h>
#include <PersistentObject.h>
#include <ServerCommandLineParser.h>
#include <Shutdown.h>

#if defined(_WIN32) && defined(WIN32_SERV)
int ApplicationMain(int argc, const char *argv[])
#else
int main(int argc, const char *argv[])
#endif  // defined(_WIN32) && defined(WIN32_SERV)
{
  libcomp::Exception::RegisterSignalHandler();

  libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

  libcomp::Config::LogVersion("COMP_hack World Server");

  std::string configPath =
      libcomp::BaseServer::GetDefaultConfigPath() + "world.xml";

  // Command line argument parser.
  auto parser = std::make_shared<libcomp::ServerCommandLineParser>();

  // Parse the command line arguments.
  if (!parser->Parse(argc, argv)) {
    return EXIT_FAILURE;
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

  auto config = std::make_shared<objects::WorldConfig>();
  if (!libcomp::BaseServer::ReadConfig(config, configPath)) {
    LogGeneralWarningMsg(
        "Failed to load the world config file. Default values will be used.\n");
  }

  if (!libcomp::PersistentObject::Initialize()) {
    LogGeneralCriticalMsg(
        "One or more persistent object definition failed to load.\n");

    return EXIT_FAILURE;
  }

  auto server = std::make_shared<world::WorldServer>(argv[0], config, parser);

  if (!server->Initialize()) {
    LogGeneralCriticalMsg("The server could not be initialized.\n");

    return EXIT_FAILURE;
  }

  // Set this for the signal handler.
  libcomp::Shutdown::Configure(server.get());

  // Start the main server loop (blocks until done).
  int returnCode = server->Start();

  // Complete the shutdown process.
  libcomp::Shutdown::Complete();

  LogGeneralInfoMsg("Bye!\n");

#ifndef EXOTIC_PLATFORM
  // Stop the logger
  delete libcomp::Log::GetSingletonPtr();
#endif  // !EXOTIC_PLATFORM

  return returnCode;
}
