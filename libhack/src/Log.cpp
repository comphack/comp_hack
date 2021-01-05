/**
 * @file libhack/src/Log.cpp
 * @ingroup libhack
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Routines to log messages to the console and/or a file.
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

#include "Log.h"

using namespace libcomp;
using namespace libhack;

/// Mapping of log components to their string names
static EnumMap<LogComponent_t, String> gLogComponentMapping = {
    {LogComponent_t::AccountManager, "AccountManager"},
    {LogComponent_t::ActionManager, "ActionManager"},
    {LogComponent_t::AIManager, "AIManager"},
    {LogComponent_t::Barter, "Barter"},
    {LogComponent_t::Bazaar, "Bazaar"},
    {LogComponent_t::CharacterManager, "CharacterManager"},
    {LogComponent_t::ChatManager, "ChatManager"},
    {LogComponent_t::Clan, "Clan"},
    {LogComponent_t::DefinitionManager, "DefinitionManager"},
    {LogComponent_t::Demon, "Demon"},
    {LogComponent_t::EventManager, "EventManager"},
    {LogComponent_t::Friend, "Friend"},
    {LogComponent_t::FusionManager, "FusionManager"},
    {LogComponent_t::Item, "Item"},
    {LogComponent_t::MatchManager, "MatchManager"},
    {LogComponent_t::Party, "Party"},
    {LogComponent_t::ServerConstants, "ServerConstants"},
    {LogComponent_t::ServerDataManager, "ServerDataManager"},
    {LogComponent_t::SkillManager, "SkillManager"},
    {LogComponent_t::Team, "Team"},
    {LogComponent_t::TokuseiManager, "TokuseiManager"},
    {LogComponent_t::Trade, "Trade"},
    {LogComponent_t::WebAPI, "WebAPI"},
    {LogComponent_t::ZoneManager, "ZoneManager"},
};

LogComponent_t libhack::StringToLogComponent(const String& comp) {
  for (auto pair : gLogComponentMapping) {
    if (pair.second == comp) {
      return pair.first;
    }
  }

  return (LogComponent_t)BaseLogComponent_t::Invalid;
}

String libhack::LogComponentToString(GenericLogComponent_t comp) {
  auto match = gLogComponentMapping.find((LogComponent_t)comp);

  if (gLogComponentMapping.end() != match) {
    return match->second;
  }

  return "Unknown";
}

Log::Log() : BaseLog() {}

Log::~Log() {}

BaseLog* Log::GetSingletonPtr() {
  auto pBase = BaseLog::GetBaseSingletonPtr();

  if (!pBase) {
    return new Log();
  }

  return pBase;
}

GenericLogComponent_t Log::StringToLogComponent(const String& comp) const {
  auto res = to_underlying(libhack::StringToLogComponent(comp));

  if (to_underlying(libcomp::BaseLogComponent_t::Invalid) != res) {
    return res;
  } else {
    return (GenericLogComponent_t)libcomp::StringToBaseLogComponent(comp);
  }
}

String Log::LogComponentToString(GenericLogComponent_t comp) const {
  if (comp >= LOG_SERVER_SPECIFIC_START_ID) {
    return libhack::LogComponentToString(comp);
  } else {
    return libcomp::BaseLogComponentToString(comp);
  }
}
