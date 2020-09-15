/**
 * @file server/channel/src/CultureMachineState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a culture machine on the channel.
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

#ifndef SERVER_CHANNEL_SRC_CULTUREMACHINESTATE_H
#define SERVER_CHANNEL_SRC_CULTUREMACHINESTATE_H

// objects Includes
#include <CultureData.h>
#include <EntityState.h>
#include <ServerCultureMachineSet.h>

namespace libcomp {
class DatabaseChangeSet;
}

namespace channel {

/**
 * Contains the state of a culture machine related to a channel.
 */
class CultureMachineState
    : public EntityState<objects::ServerCultureMachineSet> {
 public:
  /**
   * Create a culture machine state
   * @param machineID Machine ID
   * @param cmSet Pointer to the machine set definition
   */
  CultureMachineState(
      uint32_t machineID,
      const std::shared_ptr<objects::ServerCultureMachineSet>& cmSet);

  /**
   * Clean up the culture machine state
   */
  virtual ~CultureMachineState() {}

  /**
   * Get the machine ID of the entity
   * @return Machine ID
   */
  uint32_t GetMachineID();

  /**
   * Get the culture data associated to the person renting the machine
   * @return Pointer to the culture data representing the rented machine
   */
  std::shared_ptr<objects::CultureData> GetRentalData();

  /**
   * Set the current market mapped to the supplied market ID
   * @param data Pointer to the culture data representing the rented machine
   * @return true if there was not already a rental active and the supplied
   *  rental information will be used
   */
  bool SetRentalData(const std::shared_ptr<objects::CultureData>& data);

 private:
  /// Machine ID that exists in the defined machine set
  uint32_t mMachineID;

  /// Pointer to the culture data representing the rented machine
  std::shared_ptr<objects::CultureData> mRentalData;

  /// Lock for shared resources
  std::mutex mLock;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_CULTUREMACHINESTATE_H
