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
     * Set the entity's destination position based on the supplied
     * values and uses the current position values to set the origin.
     * Communicating that the move has taken place must be done elsewhere.
     * @param xPos X position to move to
     * @param yPos Y position to move to
     * @param now Server time to use as the origin ticks
     */
    void Move(float xPos, float yPos, uint64_t now);

    /**
     * Set the entity's destination rotation based on the supplied
     * values and uses the current rotation values to set the origin.
     * Communicating that the rotation has taken place must be done elsewhere.
     * @param rot New rotation value to set
     * @param now Server time to use as the origin ticks
     */
    void Rotate(float rot, uint64_t now);

    /**
     * Stop the entity's movement based on the current postion information.
     * Communicating that the movement has stopped must be done elsewhere.
     * @param now Server time to use as the origin ticks
     */
    void Stop(uint64_t now);

    /**
     * Check if the entity is currently not at their destination position
     * @return true if the entity is moving, false if they are not
     */
    bool IsMoving() const;

    /**
     * Check if the entity is currently not at their destination rotation
     * @return true if the entity is rotating, false if they are not
     */
    bool IsRotating() const;

    /**
     * Update the entity's current position and rotation values based
     * upon the source/destination ticks and the current time.  If now
     * matches the last refresh time, no work is done.
     * @param now Current timestamp of the server
     */
    void RefreshCurrentPosition(uint64_t now);

    /**
     * Check if the entity state has everything needed to start
     * being used.
     * @return true if the state is ready to use, otherwise false
     */
    virtual bool Ready() = 0;

private:
    /**
     * Corrects rotation values that have exceeded the minimum
     * or maximum allowed range.
     * @param rot Original rotation value to correct
     * @return Equivalent but corrected rotation value
     */
    float CorrectRotation(float rot) const;

    /// Last timestamp the entity's state was refreshed
    uint64_t mLastRefresh;
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
