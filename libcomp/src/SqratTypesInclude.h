/**
 * @file libcomp/src/SqratTypesInclude.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Extra Squirrel binding types.
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

#ifndef LIBCOMP_SRC_SQRATTYPESINCLUDE_H
#define LIBCOMP_SRC_SQRATTYPESINCLUDE_H

// Standard C++11 Includes
#include <list>

// libcomp Includes
#include "CString.h"
#include "SqratInt64.h"

// We want to wrap 64-bit integers a different way to preserve the value.
// #define SQRAT_WRAP_INTEGER64 1

#ifdef SQRAT_WRAP_INTEGER64
#define SQRAT_OMIT_INT64 1
#endif // SQRAT_WRAP_INTEGER64

#endif // LIBCOMP_SRC_SQRATTYPESINCLUDE_H
