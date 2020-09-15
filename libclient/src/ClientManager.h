/**
 * @file libclient/src/ClientManager.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base manager class to process client messages.
 *
 * This file is part of the COMP_hack Client Library (libclient).
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

#ifndef LIBCLIENT_SRC_CLIENTMANAGER_H
#define LIBCLIENT_SRC_CLIENTMANAGER_H

// libcomp Includes
#include <MessageClient.h>

namespace logic {

/**
 * Abstract base class used to represent a @ref MessageClient handler.
 */
class ClientManager {
 public:
  /**
   * Cleanup the manager.
   */
  virtual ~ClientManager() {}

  /**
   * Process a client message from the queue.
   * @param pMessage MessageClient to be processed
   * @return true on success, false on failure
   */
  virtual bool ProcessClientMessage(
      const libcomp::Message::MessageClient *pMessage) = 0;
};

}  // namespace logic

#endif  // LIBCLIENT_SRC_CLIENTMANAGER_H
