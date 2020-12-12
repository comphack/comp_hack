/**
 * @file server/channel/src/EnemyState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an enemy on the channel.
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

#ifndef SERVER_CHANNEL_SRC_ENEMYSTATE_H
#define SERVER_CHANNEL_SRC_ENEMYSTATE_H

// objects Includes
#include <ActiveEntityState.h>
#include <Enemy.h>

namespace channel {

/**
 * Contains the state of an enemy related to a channel as well
 * as functionality to be used by the scripting engine for AI.
 */
class EnemyState : public ActiveEntityStateImp<objects::Enemy> {
 public:
  /**
   * Create a new enemy state.
   */
  EnemyState();

  /**
   * Clean up the enemy state.
   */
  virtual ~EnemyState() {}

  /**
   * Get the current negotiation point value associated to the
   * the enemy contextual to the supplied player character entity ID
   * @param entityID ID of the player character entity talking to the
   *  enemy
   * @param exists true if there are points associated to the entity,
   *  false if there are not
   * @return Current affability and fear points associated to
   *  the player character
   */
  std::pair<int8_t, int8_t> GetTalkPoints(int32_t entityID, bool& exists);

  /**
   * Set the current negotiation point value associated to the
   * the enemy contextual to the supplied player character entity ID
   * @param entityID ID of the player entity talking to the enemy
   * @param points Current affability and fear points associated to
   *  the player character
   */
  void SetTalkPoints(int32_t entityID, const std::pair<int8_t, int8_t>& points);

  virtual std::shared_ptr<objects::EnemyBase> GetEnemyBase() const;

  virtual uint8_t RecalculateStats(
      libcomp::DefinitionManager* definitionManager,
      std::shared_ptr<objects::CalculatedEntityState> calcState = nullptr,
      std::shared_ptr<objects::MiSkillData> contextSkill = nullptr);

  virtual std::set<uint32_t> GetAllSkills(
      libcomp::DefinitionManager* definitionManager, bool includeTokusei);

  virtual uint8_t GetLNCType();

  virtual int8_t GetGender();

  virtual libobjgen::UUID GetResponsibleEntity() const;

  /**
   * Set the UUID of the entity responsible for creating this entity.
   * @param uuid UUID of entity responsible for creating this entity.
   */
  void SetResponsibleEntity(const libobjgen::UUID& uuid);

  /**
   * Cast an EntityStateObject into an EnemyState. Useful for script
   * bindings.
   * @return Pointer to the casted EnemyState
   */
  static std::shared_ptr<EnemyState> Cast(
      const std::shared_ptr<EntityStateObject>& obj);

 private:
  /// Entity responsible for creating this entity.
  libobjgen::UUID mResponsibleEntity;

  /// Player local entity IDs mapped to the enemy's current talk skill
  /// related points: affability then fear. If either of these
  /// exceeds the demon's set threshold, negotiation will end.
  std::unordered_map<int32_t, std::pair<int8_t, int8_t>> mTalkPoints;
};

}  // namespace channel

#endif  // SERVER_CHANNEL_SRC_ENEMYSTATE_H
