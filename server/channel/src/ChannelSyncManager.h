/**
 * @file server/channel/src/ChannelSyncManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel specific implementation of the DataSyncManager in
 * charge of performing server side update operations.
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

#ifndef SERVER_CHANNEL_SRC_CHANNELSYNCMANAGER_H
#define SERVER_CHANNEL_SRC_CHANNELSYNCMANAGER_H

// libcomp Includes
#include <DataSyncManager.h>
#include <EnumMap.h>

// object Includes
#include <SearchEntry.h>

namespace objects {
class EventCounter;
}

namespace channel {

class ChannelServer;

/**
 * Channel specific implementation of the DataSyncManager in charge of
 * performing server side update operations.
 */
class ChannelSyncManager : public libcomp::DataSyncManager {
 public:
  /**
   * Create a new ChannelSyncManager
   * @param server Pointer back to the channel server this belongs to.
   */
  ChannelSyncManager(const std::weak_ptr<ChannelServer>& server);

  /**
   * Clean up the ChannelSyncManager
   */
  virtual ~ChannelSyncManager();

  /**
   * Initialize the ChannelSyncManager after the world connection has
   * been established.
   * @return false if any errors were encountered and the server should
   *  be shut down.
   */
  bool Initialize();

  /**
   * Get a map of all search entries by type.
   * @return Map of all search entries by type
   */
  libcomp::EnumMap<objects::SearchEntry::Type_t,
                   std::list<std::shared_ptr<objects::SearchEntry>>>
  GetSearchEntries() const;

  /**
   * Get a list of all search entries of a specified type.
   * @return List of all search entries of a specified type
   */
  std::list<std::shared_ptr<objects::SearchEntry>> GetSearchEntries(
      objects::SearchEntry::Type_t type);

  /**
   * Get the world level event counter of the specified type
   * @return Pointer to the world level event counter, can be null
   */
  std::shared_ptr<objects::EventCounter> GetWorldEventCounter(int32_t type);

  /**
   * Server specific handler for explicit types of non-persistent records
   * being updated.
   * @param type Type name of the object being updated
   * @param obj Pointer to the record definition
   * @param isRemove true if the record is being removed, false if it is
   *  either an insert or an update
   * @param source Identifier for the server sending the updates
   * @return Response codes matching the internal DataSyncManager set
   */
  template <class T>
  int8_t Update(const libcomp::String& type,
                const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
                const libcomp::String& source);

  /**
   * Server specific handler for an operation following explicit types of
   * non-persistent records being synced.
   * @param type Type name of the object being updated
   * @param objs List of pointers to the records being updated and a flag
   *  indicating if the object was being removed
   * @param source Source server identifier
   */
  template <class T>
  void SyncComplete(
      const libcomp::String& type,
      const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
      const libcomp::String& source);

 private:
  /// Map of all search entries on the world server by type
  libcomp::EnumMap<objects::SearchEntry::Type_t,
                   std::list<std::shared_ptr<objects::SearchEntry>>>
      mSearchEntries;

  /// Map of world level event counters by type
  std::unordered_map<int32_t, std::shared_ptr<objects::EventCounter>>
      mEventCounters;

  /// Pointer to the channel server.
  std::weak_ptr<ChannelServer> mServer;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_CHANNELSYNCMANAGER_H
