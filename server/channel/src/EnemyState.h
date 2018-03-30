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
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

namespace channel
{

/**
 * Contains the state of an enemy related to a channel as well
 * as functionality to be used by the scripting engine for AI.
 */
class EnemyState : public ActiveEntityStateImp<objects::Enemy>
{
public:
    /**
     * Create a new enemy state.
     */
    EnemyState();

    /**
     * Clean up the enemy state.
     */
    virtual ~EnemyState() { }

    /**
     * Get the current negotiation point value associated to the
     * the enemy contextual to the supplied player character entity ID
     * @param entityID ID of the player character entity talking to the
     *  enemy
     * @return Current affability and fear points associated to
     *  the player character
     */
    std::pair<uint8_t, uint8_t> GetTalkPoints(int32_t entityID);

    /**
     * Set the current negotiation point value associated to the
     * the enemy contextual to the supplied player character entity ID
     * @param entityID ID of the player entity talking to the enemy
     * @param points Current affability and fear points associated to
     *  the player character
     */
    void SetTalkPoints(int32_t entityID, const std::pair<uint8_t, uint8_t>& points);

private:
    /// Player local entity IDs mapped to the enemy's current talk skill
    /// related points: affability then fear. If either of these
    /// exceeds the demon's set threshold, negotiation will end.
    std::unordered_map<int32_t,
        std::pair<uint8_t, uint8_t>> mTalkPoints;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ENEMYSTATE_H
