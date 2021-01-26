/**
 * @file libhack/src/ScriptEngine.cpp
 * @ingroup libhack
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage Squirrel scripting.
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

#include "ScriptEngine.h"

#ifndef EXOTIC_PLATFORM

// libcomp Includes
#include "DefinitionManager.h"
#include "ServerDataManager.h"

// objects Includes
#include <Account.h>
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <BazaarItem.h>
#include <Character.h>
#include <Demon.h>
#include <RegisteredChannel.h>
#include <RegisteredWorld.h>

using namespace libcomp;
using namespace libhack;
using namespace Sqrat;

static std::shared_ptr<objects::Account> ToAccount(
    const std::shared_ptr<libcomp::PersistentObject>& obj) {
  return std::dynamic_pointer_cast<objects::Account>(obj);
}

static std::shared_ptr<objects::AccountWorldData> ToAccountWorldData(
    const std::shared_ptr<libcomp::PersistentObject>& obj) {
  return std::dynamic_pointer_cast<objects::AccountWorldData>(obj);
}

static std::shared_ptr<objects::Character> ToCharacter(
    const std::shared_ptr<libcomp::PersistentObject>& obj) {
  return std::dynamic_pointer_cast<objects::Character>(obj);
}

static std::shared_ptr<objects::Demon> ToDemon(
    const std::shared_ptr<libcomp::PersistentObject>& obj) {
  return std::dynamic_pointer_cast<objects::Demon>(obj);
}

ScriptEngine::ScriptEngine(bool useRawPrint)
    : libcomp::BaseScriptEngine(useRawPrint) {
  // Bind some root level object conversions
  Sqrat::RootTable(mVM).Func("ToAccount", ToAccount);
  Sqrat::RootTable(mVM).Func("ToAccountWorldData", ToAccountWorldData);
  Sqrat::RootTable(mVM).Func("ToCharacter", ToCharacter);
  Sqrat::RootTable(mVM).Func("ToDemon", ToDemon);
}

ScriptEngine::~ScriptEngine() {}

void ScriptEngine::InitializeServerBuiltins() {
  // Now register the common objects you might want to access
  // from the server.
  Using<DefinitionManager>();
  Using<ServerDataManager>();
}

void ScriptEngine::InitializeDatabaseBuiltins() {
  // Now register the common objects you might want to access
  // from the database.
  Using<objects::Account>();
  Using<objects::AccountWorldData>();
  Using<objects::BazaarData>();
  Using<objects::BazaarItem>();
  Using<objects::Character>();
  Using<objects::Demon>();
  Using<objects::RegisteredChannel>();
  Using<objects::RegisteredWorld>();
}

#endif  // !EXOTIC_PLATFORM
