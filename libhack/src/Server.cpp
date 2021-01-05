/**
 * @file libhack/src/Server.cpp
 * @ingroup libhack
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base server class.
 *
 * This file is part of the COMP_hack Library (libhack).
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

#include "Server.h"

#ifndef EXOTIC_PLATFORM

// libcomp Includes
#include <Log.h>

// libhack Includes
#include <ScriptEngine.h>
#include <ServerConstants.h>

// object Includes
#include <Account.h>

using namespace libhack;

Server::Server(const char* szProgram,
               std::shared_ptr<objects::ServerConfig> config,
               std::shared_ptr<libcomp::ServerCommandLineParser> commandLine)
    : libcomp::BaseServer(szProgram, config, commandLine) {}

Server::~Server() {}

std::shared_ptr<libcomp::BaseScriptEngine> Server::CreateScriptEngine(
    bool useRawPrint) const {
  return std::make_shared<ScriptEngine>(useRawPrint);
}

bool Server::InitializeConstants(const libcomp::String& constantsPath) {
  if (!libhack::ServerConstants::Initialize(constantsPath)) {
    LogServerCritical([&]() {
      return libcomp::String(
                 "Server side constants failed to load from file path: %1\n")
          .Arg(constantsPath);
    });

    return false;
  }

  return true;
}

bool Server::ProcessDataLoadObject(
    const libcomp::String& name,
    std::shared_ptr<libcomp::PersistentObject>& record) {
  if (name == "Account") {
    // Check that there is a password and salt it
    auto account = std::dynamic_pointer_cast<objects::Account>(record);

    if (account->GetUsername().IsEmpty() || account->GetPassword().IsEmpty()) {
      LogServerErrorMsg(
          "Attempted to create an account with no username or no "
          "password.\n");

      return false;
    }

    libcomp::String salt = libcomp::Crypto::GenerateRandom(10);
    account->SetSalt(salt);
    account->SetPassword(
        libcomp::Crypto::HashPassword(account->GetPassword(), salt));
  }

  return true;
}

#endif  // !EXOTIC_PLATFORM
