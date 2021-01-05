/**
 * @file libhack/src/Server.h
 * @ingroup libhack
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base server class.
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

#ifndef LIBHACK_SRC_SERVER_H
#define LIBHACK_SRC_SERVER_H

#ifndef EXOTIC_PLATFORM

// libhack Includes
#include <Constants.h>

// libcomp Includes
#include <BaseServer.h>

namespace libhack {

/**
 * Base class for all servers that run workers to handle
 * incoming messages in the message queue.  Each of these
 * servers is instantiated via a dedicated config file and
 * are responsible for choosing which of the workers it
 * manages will be assigned to each incoming connection.
 */
class Server : public libcomp::BaseServer {
 public:
  /**
   * Create a new base server.
   * @param szProgram First command line argument for the application.
   * @param config Pointer to a config container that will hold properties
   *   every server has in common.
   */
  Server(const char* szProgram, std::shared_ptr<objects::ServerConfig> config,
         std::shared_ptr<libcomp::ServerCommandLineParser> commandLine);

  /**
   * Clean up the server workers and send out the the server shutdown message.
   */
  ~Server() override;

  /**
   * Create a script engine that can be used for migrations, workers, etc.
   * @param useRawPrint Set this to not prefix messages with "SQUIRREL: ".
   * @returns Script engine object.
   */
  std::shared_ptr<libcomp::BaseScriptEngine> CreateScriptEngine(
      bool useRawPrint = false) const override;

 protected:
  /**
   * Initialize the server constants.
   * @param constantsPath Path to the server constants to load.
   * @returns true if the server constants were initialized, false otherwise.
   */
  bool InitializeConstants(const libcomp::String& constantsPath) override;

  /**
   * Process an object that is loaded by @ref LoadDataFromFile
   * @param name Name of the object.
   * @param record Object that is to be processed.
   * @returns true if the object was processed, false on error
   */
  bool ProcessDataLoadObject(
      const libcomp::String& name,
      std::shared_ptr<libcomp::PersistentObject>& record) override;
};

}  // namespace libhack

#endif  // !EXOTIC_PLATFORM

#endif  // LIBHACK_SRC_SERVER_H
