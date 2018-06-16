/**
 * @file libcomp/src/Config.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Built-in configuration settings.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include "Log.h"
#include "Git.h"

namespace libcomp
{

namespace Config
{

/// 4-byte magic at the beginning of an encrypted file.
const char *ENCRYPTED_FILE_MAGIC = "CHED"; // COMP_hack Encrypted Data

/// Blowfish key used by the file encryption.
const char *ENCRYPTED_FILE_KEY = "}]#Su?Y}q!^f*S5O";

/// Blowfish initialization vector used by the file encryption.
const char *ENCRYPTED_FILE_IV = "P[?jd6c4";

} // namespace Config

} // namespace libcomp

void libcomp::Config::LogVersion(const char *szServerName)
{
    LOG_INFO(libcomp::String("%1 v%2.%3.%4 (%5)\n").Arg(szServerName).Arg(
        VERSION_MAJOR).Arg(VERSION_MINOR).Arg(VERSION_PATCH).Arg(
        VERSION_CODENAME));
    LOG_INFO(libcomp::String("Copyright (C) 2010-%1 COMP_hack Team\n\n").Arg(
        VERSION_YEAR));

    LOG_INFO("This program is free software: you can redistribute it and/or modify\n");
    LOG_INFO("it under the terms of the GNU Affero General Public License as\n");
    LOG_INFO("published by the Free Software Foundation, either version 3 of the\n");
    LOG_INFO("License, or (at your option) any later version.\n");
    LOG_INFO("\n");
    LOG_INFO("This program is distributed in the hope that it will be useful,\n");
    LOG_INFO("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    LOG_INFO("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    LOG_INFO("GNU Affero General Public License for more details.\n");
    LOG_INFO("\n");
    LOG_INFO("You should have received a copy of the GNU Affero General Public License\n");
    LOG_INFO("along with this program.  If not, see <https://www.gnu.org/licenses/>.\n");
    LOG_INFO("\n");

#if 1 == HAVE_GIT
    LOG_INFO(libcomp::String("%1 on branch %2\n").Arg(
        szGitCommittish).Arg(szGitBranch));
    LOG_INFO(libcomp::String("Commit by %1 on %2\n").Arg(
        szGitAuthor).Arg(szGitDate));
    LOG_INFO(libcomp::String("%1\n").Arg(szGitDescription));
    LOG_INFO(libcomp::String("URL: %1\n\n").Arg(szGitRemoteURL));
#endif
}
