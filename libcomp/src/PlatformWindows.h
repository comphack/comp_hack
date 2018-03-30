/**
 * @file libcomp/src/PlatformWindows.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Windows specific utility functions.
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

#ifndef LIBCOMP_SRC_PLATFORMWINDOWS_H
#define LIBCOMP_SRC_PLATFORMWINDOWS_H

#include "CString.h"

namespace libcomp
{

namespace Platform
{

/**
 * Convert the last Windows error into a string.
 * This calls the Windows method GetLastError and converts the error code into
 * a human readable string.
 * @returns String for the last WIndows error.
 * @ingroup Platform
 */
libcomp::String GetLastErrorString();

} // namespace Platform

} // namespace libcomp

#endif // LIBCOMP_SRC_PLATFORMWINDOWS_H
