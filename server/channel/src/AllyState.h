/**
 * @file server/channel/src/AllyState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an ally entity on the channel.
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

#ifndef SERVER_CHANNEL_SRC_ALLYSTATE_H
#define SERVER_CHANNEL_SRC_ALLYSTATE_H

// objects Includes
#include <ActiveEntityState.h>
#include <Ally.h>

namespace channel {

/**
 * Contains the state of an ally entity related to a channel as well
 * as functionality to be used by the scripting engine for AI.
 */
class AllyState : public ActiveEntityStateImp<objects::Ally> {
 public:
  /**
   * Create a new ally state.
   */
  AllyState();

  /**
   * Clean up the ally state.
   */
  virtual ~AllyState() {}

  virtual std::shared_ptr<objects::EnemyBase> GetEnemyBase() const;

  virtual uint8_t RecalculateStats(
      libhack::DefinitionManager* definitionManager,
      std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr,
      std::shared_ptr<objects::MiSkillData> contextSkill = nullptr);

  virtual std::set<uint32_t> GetAllSkills(
      libhack::DefinitionManager* definitionManager, bool includeTokusei);

  virtual uint8_t GetLNCType();

  virtual int8_t GetGender();

  /**
   * Cast an EntityStateObject into an AllyState. Useful for script
   * bindings.
   * @return Pointer to the casted AllyState
   */
  static std::shared_ptr<AllyState> Cast(
      const std::shared_ptr<EntityStateObject>& obj);
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_ALLYSTATE_H
