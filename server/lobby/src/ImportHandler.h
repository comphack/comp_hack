/**
 * @file server/lobby/src/ImportHandler.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Civet API handler for account import.
 *
 * This file is part of the Lobby Server (lobby).
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

#ifndef SERVER_LOBBY_SRC_IMPORTHANDLER_H
#define SERVER_LOBBY_SRC_IMPORTHANDLER_H

// lobby Includes
#include "LobbyConfig.h"
#include "LobbyServer.h"

// Civet Includes
#include <CivetServer.h>

// libcomp Includes
#include <CString.h>
#include <Database.h>

// JsonBox Includes
#include <JsonBox.h>

namespace lobby {

class ImportHandler : public CivetHandler {
 public:
  ImportHandler(const std::shared_ptr<objects::LobbyConfig>& config,
                const std::shared_ptr<lobby::LobbyServer>& server);

  virtual ~ImportHandler();

  virtual bool handlePost(CivetServer* pServer,
                          struct mg_connection* pConnection);

 private:
  libcomp::String ExtractFile(const libcomp::String& contentType,
                              const libcomp::String& contentData);

  std::shared_ptr<objects::LobbyConfig> mConfig;
  std::shared_ptr<lobby::LobbyServer> mServer;
};

}  // namespace lobby

#endif  // SERVER_LOBBY_SRC_IMPORTHANDLER_H
