/**
 * @file server/channel/src/EntityState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a non-active entity on the channel.
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

#ifndef SERVER_CHANNEL_SRC_ENTITYSTATE_H
#define SERVER_CHANNEL_SRC_ENTITYSTATE_H

// objects Includes
#include <EntityStateObject.h>

namespace channel
{

/**
 * Contains the state of a non-active entity related to a channel.
 */
template<typename T>
class EntityState : public objects::EntityStateObject
{
public:
    /**
     * Create a new non-active entity state.
     */
    EntityState(const std::shared_ptr<T>& entity);

    /**
     * Clean up the non-active entity state.
     */
    virtual ~EntityState() { }

    /**
     * Get the entity
     * @return Pointer to the entity
     */
    std::shared_ptr<T> GetEntity()
    {
        return mEntity;
    }

private:
    std::shared_ptr<T> mEntity;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ENTITYSTATE_H
