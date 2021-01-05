/**
 * @file libhack/src/ScriptEngine.h
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

#ifndef LIBHACK_SRC_SCRIPTENGINE_H
#define LIBHACK_SRC_SCRIPTENGINE_H

// libcomp Includes
#include <BaseScriptEngine.h>

#ifndef EXOTIC_PLATFORM

namespace libhack {

/**
 * Represents a Sqrat based Squirrel virtual machine handler to facilitate
 * script execution and bind @ref Object instances to the VM.
 */
class ScriptEngine : public libcomp::BaseScriptEngine {
 public:
  /**
   * Create the VM.
   * @param useRawPrint Set this to not prefix messages with "SQUIRREL: ".
   */
  ScriptEngine(bool useRawPrint = false);

  /**
   * Clean up the VM.
   */
  virtual ~ScriptEngine();

 private:
  /**
   * Initialize the server specific database built-in script modules.
   */
  void InitializeDatabaseBuiltins() override;

  /**
   * Initialize the server specific server built-in script modules.
   */
  void InitializeServerBuiltins() override;
};

}  // namespace libhack

#endif  // !EXOTIC_PLATFORM

#endif  // LIBHACK_SRC_SCRIPTENGINE_H
