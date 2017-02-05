/**
 * @file server/channel/src/DemonState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a demon on the channel.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef SERVER_CHANNEL_SRC_DEMONSTATE_H
#define SERVER_CHANNEL_SRC_DEMONSTATE_H

// objects Includes
#include <DemonStateObject.h>

namespace channel
{

/**
 * Contains the state of a demon from a game client currently connected
 * to the channel.
 */
class DemonState : public objects::DemonStateObject
{
public:
    /**
     * Create a new demon state.
     */
    DemonState();

    /**
     * Clean up the demon state.
     */
    virtual ~DemonState();

    /**
     * Recalculate the demon's current stats, adjusted by effects.
     * @return true if the calculation succeeded, false if it errored
     */
    bool RecalculateStats();

    /**
     * Check if the demon state has everything needed to start
     * being used.
     * @return true if the state is ready to use, otherwise false
     */
    bool Ready();
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_DEMONSTATE_H
