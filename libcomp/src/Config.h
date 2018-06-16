/**
 * @file libcomp/src/Config.h
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

#ifndef LIBCOMP_SRC_CONFIG_H
#define LIBCOMP_SRC_CONFIG_H

namespace libcomp
{

namespace Config
{

/// 4-byte magic at the beginning of an encrypted file.
extern const char *ENCRYPTED_FILE_MAGIC;

/// Blowfish key used by the file encryption.
extern const char *ENCRYPTED_FILE_KEY;

/// Blowfish initialization vector used by the file encryption.
extern const char *ENCRYPTED_FILE_IV;

/**
 * Log the version information for the server.
 * @param szServerName String to print as the server name.
 */
void LogVersion(const char *szServerName);

} // namespace Config

} // namespace libcomp

#endif // LIBCOMP_SRC_CONFIG_H
