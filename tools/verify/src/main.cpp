/**
 * @file tools/verify/src/main.cpp
 * @ingroup tools
 *
 * @author HACKforst
 *
 * @brief Tool to verify files used by the servers.
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

// Standard C++11 Includes
#include <fstream>
#include <iostream>

// libcomp Includes
#include <DataStore.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <ServerDataManager.h>

int UsageMisc(const char *szAppName) {
  std::cerr << "USAGE: " << szAppName << " MODE ..." << std::endl;
  std::cerr << std::endl;
  std::cerr
      << "MODE indicates execution mode. Valid modes contain: server_data."
      << std::endl;
  std::cerr << std::endl;
  std::cerr << "server_data mode verifies data loaded by the channel server "
               "from the binary data and xml files in the datastore."
            << std::endl;

  return EXIT_FAILURE;
}

int Usage(const char *szAppName, const char *mode) {
  if (mode == std::string("server_data")) {
    std::cerr << "USAGE: " << szAppName << " server_data MODE LEVEL STORE"
              << std::endl;
    std::cerr << std::endl;
    std::cerr << "MODE indicates if (0) startup errors only should be checked "
                 "or (1) data integrity errors should be checked too."
              << std::endl;
    std::cerr
        << "LEVEL indicates the log levels to print. Levels include DEBUG, "
           "INFO, WARNING and ERROR. CRITICAL levels will always print."
        << std::endl;
    std::cerr
        << "STORE indicates a list of paths to use when loading the datastore."
        << std::endl;
  }

  return EXIT_FAILURE;
}

int VerifyServerData(int argc, char *argv[]) {
  if (argc < 5) {
    return Usage(argv[0], argv[1]);
  }

  auto log = libhack::Log::GetSingletonPtr();

  libcomp::BaseLog::Level_t logLevel;

  if (argv[3] == std::string("DEBUG")) {
    logLevel = libcomp::BaseLog::LOG_LEVEL_DEBUG;
  } else if (argv[3] == std::string("INFO")) {
    logLevel = libcomp::BaseLog::LOG_LEVEL_INFO;
  } else if (argv[3] == std::string("WARNING")) {
    logLevel = libcomp::BaseLog::LOG_LEVEL_WARNING;
  } else if (argv[3] == std::string("ERROR")) {
    logLevel = libcomp::BaseLog::LOG_LEVEL_ERROR;
  } else {
    return Usage(argv[0], argv[1]);
  }

  log->SetLogLevel(to_underlying(libcomp::BaseLogComponent_t::General),
                   logLevel);
  log->SetLogLevel(to_underlying(libhack::LogComponent_t::DefinitionManager),
                   logLevel);
  log->SetLogLevel(to_underlying(libhack::LogComponent_t::ServerDataManager),
                   logLevel);

  log->AddStandardOutputHook();

  bool fail = false;

  libcomp::DataStore datastore(argv[0]);

  for (int i = 4; i < argc; i++) {
    if (!datastore.AddSearchPath(argv[i])) {
      fail = true;
    }
  }

  if (!fail) {
    libhack::DefinitionManager definitionManager;
    libhack::ServerDataManager serverDataManager;

    if (!definitionManager.LoadAllData(&datastore) ||
        !serverDataManager.LoadData(&datastore, &definitionManager)) {
      fail = true;
    } else if (argv[2] == std::string("1") &&
               !serverDataManager.VerifyDataIntegrity(&definitionManager)) {
      fail = true;
    }
  }

#ifndef EXOTIC_PLATFORM
  // Stop the logger
  delete libcomp::BaseLog::GetBaseSingletonPtr();
#endif  // !EXOTIC_PLATFORM

  return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    return UsageMisc(argv[0]);
  }

  if (argv[1] == std::string("server_data")) {
    return VerifyServerData(argc, argv);
  } else {
    UsageMisc(argv[0]);
  }

  return EXIT_FAILURE;
}
