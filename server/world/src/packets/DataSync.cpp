/**
 * @file server/world/src/packets/DataSync.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Request from the lobby or channel servers to synchronize one or more
 *  data records between the servers.
 *
 * This file is part of the World Server (world).
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

#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>

// world Includes
#include "WorldServer.h"
#include "WorldSyncManager.h"

using namespace world;

bool Parsers::DataSync::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  auto server =
      std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
  auto syncManager = server->GetWorldSyncManager();

  libcomp::String source =
      server->GetLobbyConnection() == connection ? "lobby" : "channel";
  if (!syncManager->SyncIncoming(p, source)) {
    LogGeneralError([source]() {
      return libcomp::String("Data sync from '%1' failed to process.\n")
          .Arg(source);
    });
    return false;
  }

  // Sync any records that need to relay back
  syncManager->SyncOutgoing();

  return true;
}
