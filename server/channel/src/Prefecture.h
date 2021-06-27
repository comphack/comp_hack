/**
 * @file server/channel/src/Prefecture.h
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Oversees a collection of zones assigned to a worker thread.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2021 COMP_hack Team <compomega@tutanota.com>
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

#ifndef SERVER_CHANNEL_SRC_PREFECTURE_H
#define SERVER_CHANNEL_SRC_PREFECTURE_H

// channel Includes
#include "ChannelServer.h"

namespace channel {

class ChannelServer;

/**
 * Manager to handle zone focused actions.
 */
class Prefecture {
 public:
  /**
   * Create a new Prefecture.
   * @param server Pointer back to the channel server this prefecture resides
   * in.
   */
  Prefecture(const std::weak_ptr<ChannelServer> &server);

  /**
   * Get the server this prefecture resides in.
   * @return Pointer to the server this prefecture resides in.
   */
  std::shared_ptr<ChannelServer> GetServer() const;

 private:
  std::weak_ptr<ChannelServer> mServer;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_PREFECTURE_H
