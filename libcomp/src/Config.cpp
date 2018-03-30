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
