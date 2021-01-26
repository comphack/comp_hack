/**
 * @file libconfig/src/ConfigLogVersion.cpp
 * @ingroup libconfig
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Built-in configuration settings.
 *
 * This file is part of the COMP_hack Library (libconfig).
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

#include "Config.h"

// libcomp Includes
#include "Constants.h"
#include "Git.h"
#include "Log.h"

void libhack::Config::LogVersion(const char *szServerName) {
  LogGeneralInfo([&]() {
    return libcomp::String("%1 v%2.%3.%4 (%5)\n")
        .Arg(szServerName)
        .Arg(VERSION_MAJOR)
        .Arg(VERSION_MINOR)
        .Arg(VERSION_PATCH)
        .Arg(VERSION_CODENAME);
  });

  LogGeneralInfo([&]() {
    return libcomp::String("Copyright (C) 2010-%1 COMP_hack Team\n\n")
        .Arg(VERSION_YEAR);
  });

  LogGeneralInfoMsg(
      "This program is free software: you can redistribute it and/or modify\n");
  LogGeneralInfoMsg(
      "it under the terms of the GNU Affero General Public License as\n");
  LogGeneralInfoMsg(
      "published by the Free Software Foundation, either version 3 of the\n");
  LogGeneralInfoMsg("License, or (at your option) any later version.\n");
  LogGeneralInfoMsg("\n");
  LogGeneralInfoMsg(
      "This program is distributed in the hope that it will be useful,\n");
  LogGeneralInfoMsg(
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  LogGeneralInfoMsg(
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  LogGeneralInfoMsg("GNU Affero General Public License for more details.\n");
  LogGeneralInfoMsg("\n");
  LogGeneralInfoMsg(
      "You should have received a copy of the GNU Affero General Public "
      "License\n");
  LogGeneralInfoMsg(
      "along with this program.  If not, see "
      "<https://www.gnu.org/licenses/>.\n");
  LogGeneralInfoMsg("\n");

#if 1 == HAVE_GIT
  LogGeneralInfo([&]() {
    return libcomp::String("%1 on branch %2\n")
        .Arg(szGitCommittish)
        .Arg(szGitBranch);
  });

  LogGeneralInfo([&]() {
    return libcomp::String("Commit by %1 on %2\n")
        .Arg(szGitAuthor)
        .Arg(szGitDate);
  });

  LogGeneralInfo(
      [&]() { return libcomp::String("%1\n").Arg(szGitDescription); });

  LogGeneralInfo(
      [&]() { return libcomp::String("URL: %1\n\n").Arg(szGitRemoteURL); });
#endif
}
