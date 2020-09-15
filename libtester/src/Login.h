/**
 * @file libtester/src/Login.h
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test functions to aid in login.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
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

#ifndef LIBTESTER_SRC_LOGIN_H
#define LIBTESTER_SRC_LOGIN_H

// libcomp Includes
#include <HttpConnection.h>

namespace libtester {

namespace Login {

bool WebLogin(const libcomp::String& username, const libcomp::String& password,
              const libcomp::String& clientVersion, libcomp::String& sid1,
              libcomp::String& sid2);

}  // namespace Login

}  // namespace libtester

#endif  // LIBTESTER_SRC_LOGIN_H
