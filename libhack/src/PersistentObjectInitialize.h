/**
 * @file libhack/src/PersistentObjectInitialize.h
 * @ingroup libhack
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Initialization of PersistentObject derived types.
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

#ifndef LIBHACK_SRC_PERSISTENTOBJECTINITIALIZE_H
#define LIBHACK_SRC_PERSISTENTOBJECTINITIALIZE_H

// libcomp Includes
#include <PersistentObject.h>

#ifndef EXOTIC_PLATFORM

namespace libhack {

/**
 * Register all derived types in libhack to the TypeMap.
 * Persisted types needed in other databases should derive from this class
 * to register their own as well.
 * @return true on success, false on failure
 */
bool PersistentObjectInitialize();

}  // namespace libhack

#endif  // !EXOTIC_PLATFORM

#endif  // LIBHACK_SRC_PERSISTENTOBJECTINITIALIZE_H
