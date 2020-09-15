/**
 * @file server/lobby/src/ManagerClientPacket.h
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Manager to handle lobby packet logic.
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

#ifndef SERVER_LOBBY_SRC_MANAGERCLIENTPACKET_H
#define SERVER_LOBBY_SRC_MANAGERCLIENTPACKET_H

// libcomp Includes
#include <ManagerPacket.h>

namespace lobby {

/**
 * Manager class responsible for handling client side packets.
 */
class ManagerClientPacket : public libcomp::ManagerPacket {
 public:
  /**
   * Create a new manager.
   * @param server Pointer to the server that uses this manager
   */
  ManagerClientPacket(std::weak_ptr<libcomp::BaseServer> server);

  /**
   * Clean up the manager.
   */
  virtual ~ManagerClientPacket();

 protected:
  virtual bool ValidateConnectionState(
      const std::shared_ptr<libcomp::TcpConnection>& connection,
      libcomp::CommandCode_t commandCode) const;
};

}  // namespace lobby

#endif  // SERVER_LOBBY_SRC_MANAGERCLIENTPACKET_H
