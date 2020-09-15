/**
 * @file server/channel/src/ZoneGeometryLoader.h
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Loads the geometry for the zones.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_ZONEGEOMETRYLOADER_H
#define SERVER_CHANNEL_SRC_ZONEGEOMETRYLOADER_H

// Standard C++11 Includes
#include <mutex>

// channel Includes
#include "ChannelServer.h"
#include "ZoneGeometry.h"

namespace channel {

/**
 * Loader for QMP zone geometry.
 */
class ZoneGeometryLoader {
 public:
  /**
   * Load all QMP zone geometry files.
   * @param localZoneIDs IDs of the zones to load the geometry for.
   * @param server Pointer to the channel server.
   * @returns Loaded zone geometry.
   */
  std::unordered_map<std::string, std::shared_ptr<ZoneGeometry>> LoadQMP(
      std::unordered_map<uint32_t, std::set<uint32_t>> localZoneIDs,
      const std::shared_ptr<ChannelServer>& server);

 private:
  /**
   * Load a QMP for the next zone in the list.
   * @param server Pointer to the channel server.
   */
  bool LoadZoneQMP(const std::shared_ptr<ChannelServer>& server);

  /// Mutex to lock access to the input and output data by threads.
  std::mutex mDataLock;

  /// List of zone pairs for the QMP loading process.
  std::list<std::pair<uint32_t, std::set<uint32_t>>> mZonePairs;

  /// Map of QMP filenames to the geometry structures built from them
  std::unordered_map<std::string, std::shared_ptr<ZoneGeometry>> mZoneGeometry;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_ZONEGEOMETRYLOADER_H
