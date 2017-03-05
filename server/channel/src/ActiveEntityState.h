/**
 * @file server/channel/src/ActiveEntityState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an active entity on the channel.
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

#ifndef SERVER_CHANNEL_SRC_ACTIVEENTITYSTATE_H
#define SERVER_CHANNEL_SRC_ACTIVEENTITYSTATE_H

// objects Includes
#include <ActiveEntityStateObject.h>
#include <EntityStats.h>

namespace libcomp
{
class DefinitionManager;
}

namespace channel
{

class ActiveEntityState : public objects::ActiveEntityStateObject
{
public:
    /**
     * Recalculate the entity's current stats, adjusted by equipment and
     * effects.
     * @param Pointer to the server's definition manager to use for calculations
     * @return true if the calculation succeeded, false if it errored
     */
    virtual bool RecalculateStats(libcomp::DefinitionManager* definitionManager) = 0;

    /**
     * Get the core stats associated to the active entity.
     * @return Pointer to the active entity's core stats
     */
    virtual std::shared_ptr<objects::EntityStats> GetCoreStats() = 0;

    /**
     * Check if the character state has everything needed to start
     * being used.
     * @return true if the state is ready to use, otherwise false
     */
    virtual bool Ready() = 0;
};

/**
 * Contains the state of an active entity related to a channel.
 */
template<typename T>
class ActiveEntityStateImp : public ActiveEntityState
{
public:
    /**
     * Create a new active entity state.
     */
    ActiveEntityStateImp();

    /**
     * Clean up the active entity state.
     */
    virtual ~ActiveEntityStateImp() { }

    /**
     * Get the active entity
     * @return Pointer to the active entity
     */
    std::shared_ptr<T> GetEntity()
    {
        return mEntity;
    }

    /**
     * Set the active entity
     * @param entity Pointer to the active entity
     */
    void SetEntity(const std::shared_ptr<T>& entity)
    {
        mEntity = entity;
    }

    virtual std::shared_ptr<objects::EntityStats> GetCoreStats()
    {
        return mEntity->GetCoreStats().Get();
    }

    virtual bool RecalculateStats(libcomp::DefinitionManager* definitionManager);

    virtual bool Ready()
    {
        return mEntity != nullptr;
    }

private:
    /// Pointer to the active entity
    std::shared_ptr<T> mEntity;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ACTIVEENTITYSTATE_H
