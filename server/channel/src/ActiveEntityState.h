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

// libcomp Includes
#include <EnumMap.h>

// objects Includes
#include <ActiveEntityStateObject.h>
#include <EntityStats.h>
#include <MiCorrectTbl.h>
#include <StatusEffect.h>

// Standard C++11 includes
#include <map>

/// Effect cancelled upon logout
const uint8_t EFFECT_CANCEL_LOGOUT = 0x01;

/// Effect cancelled upon leaving a zone
const uint8_t EFFECT_CANCEL_ZONEOUT = 0x04;

/// Effect cancelled upon death
const uint8_t EFFECT_CANCEL_DEATH = 0x08;

/// Effect cancelled upon being hit
const uint8_t EFFECT_CANCEL_HIT = 0x10;

/// Effect cancelled upon receiving any damage
const uint8_t EFFECT_CANCEL_DAMAGE = 0x20;

/// Effect cancelled upon being knocked back
const uint8_t EFFECT_CANCEL_KNOCKBACK = 0x40;

/// Effect cancelled upon performing a skill
const uint8_t EFFECT_CANCEL_SKILL = 0x80;

/// Recalculation resulted in a locally visible stat change
const uint8_t ENTITY_CALC_STAT_LOCAL = 0x01;

/// Recalculation resulted in a stat change visible to the world
const uint8_t ENTITY_CALC_STAT_WORLD = 0x02;

/// Recalculation resulted in a modified skill set (characters only)
const uint8_t ENTITY_CALC_SKILL = 0x04;

namespace libcomp
{
class DefinitionManager;
}

typedef objects::MiCorrectTbl::ID_t CorrectTbl;
typedef std::unordered_map<uint32_t, std::pair<uint8_t, bool>> AddStatusEffectMap;

namespace channel
{

class Zone;

/**
 * Represents an active entity on the channel server. An entity is
 * active if it can move or perform actions independent of other entities.
 * Active entities have stats and status effects in addition to the usual
 * current zone position shared with non-active entities.
 */
class ActiveEntityState : public objects::ActiveEntityStateObject
{
public:
    /**
     * Create a new active entity state
     */
    ActiveEntityState();

    /**
     * Explicitly defined copy constructor necessary due to removal
     * of implicit constructor from non-copyable mutex member. This should
     * never actually be used.
     * @param other The other entity to copy
     */
    ActiveEntityState(const ActiveEntityState& other);

    /**
     * Get the adjusted correct table value associated to the entity.
     * @param tableID ID of the correct table value to retrieve
     * @return Adjusted correct table value
     */
    int16_t GetCorrectValue(CorrectTbl tableID);

    /**
     * Recalculate the entity's stats, adjusted by equipment and
     * effects.
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how effects and items interact with the entity
     * @return Flags indicating if the calculation resulted in a change that
     *  should be communicated to the client or world:
     *  0x01) ENTITY_CALC_STAT_LOCAL = locallly visible stats were changed
     *  0x02) ENTITY_CALC_STAT_WORLD = stats changed that are visible to the world
     *  0x04) ENTITY_CALC_SKILL = skill set has changed (character only)
     */
    virtual uint8_t RecalculateStats(libcomp::DefinitionManager* definitionManager) = 0;

    /**
     * Get the core stats associated to the active entity.
     * @return Pointer to the active entity's core stats
     */
    virtual std::shared_ptr<objects::EntityStats> GetCoreStats() = 0;

    /**
     * Get the the entity UUID associated to the entity this state represents.
     * @return Entity UUID or null UUID if not specified
     */
    virtual const libobjgen::UUID GetEntityUUID();

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
     * Set the entity's destination position at a distace directly away or
     * directly towards the specified point. Communicating that the move
     * has taken place must be done elsewhere.
     * @param targetX X coordinate to move relative to
     * @param targetY Y coordinate to move relative to
     * @param distance Distance to move
     * @param away true if the entity should move away from the point,
     *  false if it should move toward it
     * @param now Server time to use as the origin ticks
     * @param endTime Server time to use as the destination ticks
     */
    void MoveRelative(float targetX, float targetY, float distance, bool away,
        uint64_t now, uint64_t endTime);

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
     * Check if the entity is currently alive
     * @return true if the entity is alive, false if they are not
     */
    bool IsAlive() const;

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
     * Calculate the distance between the entity and the specified X
     * and Y coordiates
     * @param x X coordinate to calculate the distance for
     * @param y Y coordinate to calculate the distance for
     * @param squared Optional parameter to return the distance squared
     *  for faster radius comparisons
     * @return Distance between the entity and the specified point
     */
    float GetDistance(float x, float y, bool squared = false);

    /**
     * Update the entity's current position and rotation values based
     * upon the source/destination ticks and the current time.  If now
     * matches the last refresh time, no work is done.
     * @param now Current timestamp of the server
     */
    void RefreshCurrentPosition(uint64_t now);

    /**
     * Update the entity's current knockback value based on the last
     * ticks associated to the value and the current time. If the value
     * reaches or exceeds the maximum knockback resistance, the max
     * value will be used and the last update tick will be cleared.
     * @param now Current timestamp of the server
     */
    void RefreshKnockback(uint64_t now);

    /**
     * Refresh and then reduce the entity's knockback value. If the value
     * goes under zero, it will be set to zero.
     * @param now Current timestamp of the server
     * @param decrease Value to decrease the knockback value by
     */
    float UpdateKnockback(uint64_t now, float decrease);

    /**
     * Check if the entity state has everything needed to start
     * being used.
     * @return true if the state is ready to use, otherwise false
     */
    virtual bool Ready() = 0;

    /**
     * Get the zone the entity currently exists in.
     * @return Pointer to the entity's current zone, nullptr if they
     *  are not currently in a zone
     */
    std::shared_ptr<Zone> GetZone() const;

    /**
     * Set the entity's current zone.
     * @param zone Pointer to the zone the entity has entered
     * @param updatePrevious true if the previous zone should be notified
     *  of the change, false if this is being handled elsewhere
     */
    void SetZone(const std::shared_ptr<Zone>& zone, bool updatePrevious = true);

    /**
     * Set the HP and/or MP of the entity to either a specified or adjusted
     * value. Use this instead of a raw EntityStats function to avoid
     * conflicting updats running at the same time.
     * @param hp Specified or adjusted HP value to apply
     * @param mp Specified or adjusted MP value to apply
     * @param adjust If true the supplied HP and MP values will be added to
     *  the current value instead of being set explicitly
     * @param canOverflow true if the entity can be changed from alive to
     *  dead and vice-versa
     * @return If adjust and canOverflow are both true, the returned value
     *  will be whether the entity's HP has overflowed.  If one of those params
     *  is false, the returned value represents if the HP or MP were changed
     */
    bool SetHPMP(int16_t hp, int16_t mp, bool adjust, bool canOverflow = false);

    /**
     * Set the HP and/or MP of the entity to either a specified or adjusted
     * value. Use this instead of a raw EntityStats function to avoid
     * conflicting updats running at the same time.
     * @param hp Specified or adjusted HP value to apply
     * @param mp Specified or adjusted MP value to apply
     * @param adjust If true the supplied HP and MP values will be added to
     *  the current value instead of being set explicitly
     * @param canOverflow true if the entity can be changed from alive to
     *  dead and vice-versa
     * @param hpAdjusted If not adjusting, this will be the HP damage dealt,
     *  otherwise it matches the value supplied
     * @param mpAdjusted If not adjusting, this will be the MP damage dealt,
     *  otherwise it matches the value supplied
     * @return If adjust and canOverflow are both true, the returned value
     *  will be whether the entity's HP has overflowed.  If one of those params
     *  is false, the returned value represents if the HP or MP were changed
     */
    bool SetHPMP(int16_t hp, int16_t mp, bool adjust, bool canOverflow,
        int16_t& hpAdjusted, int16_t& mpAdjusted);

    /**
     * Get the current status effect map
     * @return Map of current status effects by type ID
     */
    const std::unordered_map<uint32_t,
        std::shared_ptr<objects::StatusEffect>>& GetStatusEffects() const;

    /**
     * Set the status effects currently on the entity
     * @param List of status effects currently on the entity
     */
    void SetStatusEffects(
        const std::list<std::shared_ptr<objects::StatusEffect>>& effects);

    /**
     * Add new status effects to the entity and activate them. If there are
     * status effects that are replaced by the application of the new ones
     * they will be expired and returned.
     * @param effects Map of status effect stacks and whether they are replaces
     *  or not by effect type ID
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how the effects behave
     * @param now Current system timestamp to use when activating the effects,
     *  defaults to the current time if not specified
     * @param queueChanges If true and the entity is in an effect active state,
     *  the effect inserts, updates and removes will be queued as the next entity
     *  time-based events to be processed by the next server tick. Set to false
     *  if the changes will be communicated by some other means
     * @return Set of expired effect IDs if any added effects cancel existing
     *  effects
     */
    std::set<uint32_t> AddStatusEffects(const AddStatusEffectMap& effects,
        libcomp::DefinitionManager* definitionManager, uint32_t now = 0,
        bool queueChanges = true);

    /**
     * Expire existing status effects by effect type ID. The expire event
     * will be queued up for processing on the next server tick.
     * @param effectTypes Set of effect type IDs to expire
     */
    void ExpireStatusEffects(const std::set<uint32_t>& effectTypes);

    /**
     * Cancel existing status effects via cancel event flags. The expire
     * event will be queued up for processing on the next server tick.
     * @param cancelFlags Flags indicating conditions that can cause status
     *  effects to be cancelled. The set of valid status conditions are listed
     *  as constants on ActiveEntityState
     */
    void CancelStatusEffects(uint8_t cancelFlags);

    /**
     * Activate or deactivate the entity's status effect states. By activating
     * effects, they will be registered with the ActiveEntityState and its current
     * zone via their absolute server time expiration in addition to setting any
     * quick access properties on the entity. By deactivating effects, they will
     * be unregistered from the zone, the quick access properties will be cleared
     * and the expiration time of relative effects will be updated.
     * @param activate true if the effects will activated, else they will be
     *  deactivated
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how the effects behave
     * @param now Current system timestamp to use when activating the effects,
     *  defaults to the current time if not specified
     */
    void SetStatusEffectsActive(bool activate,
        libcomp::DefinitionManager* definitionManager, uint32_t now = 0);

    /**
     * Pop effect events that have occurred past the specified time off the
     * event mapping for the entity and their current zone.
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how the effects behave
     * @param time System timestamp to use for comparing effect events that
     *  have passed
     * @param hpTDamage Output parameter to store the amount of HP time
     *  damage that should be dealt
     * @param mpTDamage Output parameter to store the amount of MP time
     *  damage that should be dealt
     * @param added Output set to store the effect type IDs of newly added
     *  effects that have been queued since the specified time
     * @param updated Output set to store the effect type IDs of updated
     *  effects that have been queued since the specified time
     * @param removed Output set to store the effect type IDs of removed
     *  effects that have been queued since the specified time
     * @return true if any events have passed, false if none have
     */
    bool PopEffectTicks(libcomp::DefinitionManager* definitionManager,
        uint32_t time, int32_t& hpTDamage, int32_t& mpTDamage,
        std::set<uint32_t>& added, std::set<uint32_t>& updated,
        std::set<uint32_t>& removed);

    /**
     * Get a snapshot of status effects currently on the entity with their
     * corresponding expiration time which is based upon he supplied time
     * for relative duration effects
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how the effects behave
     * @param now Current system timestamp to use for caculating relative duration
     *  expirations remaining, if nothing is specified, the current time will be used
     * @return List of current status effects paired with their expiration time
     *  remaining
     */
    std::list<std::pair<std::shared_ptr<objects::StatusEffect>, uint32_t>>
        GetCurrentStatusEffectStates(libcomp::DefinitionManager* definitionManager,
        uint32_t now = 0);

    /**
     * Get the entity IDs of opponents this entity is in combat against.
     * @return Set of opponent entity IDs
     */
    std::set<int32_t> GetOpponentIDs() const;

    /**
     * Check if the entity has an opponent with the specified entity ID.
     * @param opponentID Entity ID of an opponent to find
     * @return true if the entity ID is associated as an opponent to this entity
     */
    bool HasOpponent(int32_t opponentID);

    /**
     * Add or remove an opponent with the specified entity ID.
     * @param add true if the opponent will be added, false if it will be removed
     * @param opponentID Entity ID of an opponent to add or remove
     * @return Number of opponents associated to the entity after the operation
     */
    size_t AddRemoveOpponent(bool add, int32_t opponentID);

    /**
     * Get the entity's chance to null, reflect or absorb the specified affinity.
     * @param nraIdx Correct table index for affinity NRA
     *  1) Null
     *  2) Reflect
     *  3) Absorb
     *  Defines exist in the Constants header for each of these.
     * @param type Correct table type of the affinity to retrieve
     * @return Integer representation of the chance to avoid affinity damage, 0
     *  if the supplied values are invalid
     */
    int16_t GetNRAChance(uint8_t nraIdx, CorrectTbl type);

protected:
    /**
     * Set the status effects currently on the entity
     * @param List of status effects currently on the entity as object references
     */
    void SetStatusEffects(
        const std::list<libcomp::ObjectReference<objects::StatusEffect>>& effects);

    /**
     * Activate a status effect added to the entity's current status effect set.
     * By activating an effect, it will be registered with the ActiveEntityState
     * and its current zone via their absolute server time expiration in addition
     * to setting any quick access properties on the entity.
     * @param effect Pointer to the effect to activate
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how the effect behaves
     * @param now Current system timestamp to use when activating the effect
     */
    void ActivateStatusEffect(
        const std::shared_ptr<objects::StatusEffect>& effect,
        libcomp::DefinitionManager* definitionManager, uint32_t now);

    /**
     * Set the next effect event time for a specified effect type for the entity's
     * event map. The zone itself must be updated using RegisterNextEffectTime after
     * each effect is set using this function.
     * @param effectType Effect type ID to register an event for
     * @param time Absolute system time to register for the event
     */
    void SetNextEffectTime(uint32_t effectType, uint32_t time);

    /**
     * Register the entity's next effect event time with the current zone.
     */
    void RegisterNextEffectTime();

    /**
     * Calculate the relative or absolute expiration for an effect based upon
     * the current system time and an absolute system time.
     * @param effect Pointer to the effect to calculate the expiration of
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how the effect behaves
     * @param nextTime Absolute system time to use when calculating the expiration
     * @param now Current system timestamp to use when activating the effect
     * @return Calculated expiration for the effect
     */
    uint32_t GetCurrentExpiration(
        const std::shared_ptr<objects::StatusEffect>& effect,
        libcomp::DefinitionManager* definitionManager, uint32_t nextTime,
        uint32_t now);

    /**
     * Corrects rotation values that have exceeded the minimum
     * or maximum allowed range.
     * @param rot Original rotation value to correct
     * @return Equivalent but corrected rotation value
     */
    float CorrectRotation(float rot) const;

    /**
     * Adjust the supplied correct table stat values based upon adjustments from
     * equipment or status effects.
     * @param adjustments List of adjustments to the correct table values supplied
     * @param stats Output map parameter of base or calculated stats to adjust for
     *  the current entity
     * @param baseMode If true only base stat correct table types will be adjusted,
     *  if false only the non-base stat correct table types will be adjusted and NRA
     *  values will be updated immediately.
     */
    void AdjustStats(const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments,
        libcomp::EnumMap<CorrectTbl, int16_t>& stats, bool baseMode);

    /**
     * Update the entity's calculated NRA chances for each affinity from base and
     * equipment values. NRA chances will further be modified for status effects
     * within AdjustStats during the calculated stat mode.
     * @param stats Map containing the base NRA values
     * @param adjustments List of adjustments to the correct table values supplied
     *  by equipment
     */
    void UpdateNRAChances(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
        const std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments = {});

    /**
     * Get the correct table value adjustments from the entity's current skills
     * and status effects.
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how the skills and effects behave
     * @param adjustments Output list parameter to add the adjustments to
     */
    void GetAdditionalCorrectTbls(libcomp::DefinitionManager* definitionManager,
        std::list<std::shared_ptr<objects::MiCorrectTbl>>& adjustments);

    /**
     * Recalculate a demon or enemy entity's stats.
     * @param definitionManager Pointer to the DefinitionManager to use when
     *  determining how effects and items interact with the entity
     * @param demonID Demon type ID of the entity
     * @return 1 if the calculation resulted in a change to the stats that should
     *  be sent to the client, 2 if one of the changes should be communicated to
     *  the world (for party members etc), 0 otherwise
     */
    uint8_t RecalculateDemonStats(libcomp::DefinitionManager* definitionManager,
        uint32_t demonID);

    /**
     * Calculate the numeric representation (also stored in constants)
     * of the entity's alignment based off the supplied LNC point value
     * @param lncPoints LNC points representing degrees of alignment
     * @return Numeric representation of the entity's current alignment
     */
    uint8_t CalculateLNCType(int16_t lncPoints) const;

    /**
     * Remove any switch skills marked as active that are no longer available
     * to the entity.
     */
    void RemoveInactiveSwitchSkills();

    /**
     * Compare and set the entity's current stats and also keep track of if
     * a change occurred.
     * @param stats Map of correct table IDs to calculated stats to set on the
     *  entity
     * @return 1 if the calculation resulted in a change to the stats that should
     *  be sent to the client, 2 if one of the changes should be communicated to
     *  the world (for party members etc), 0 if no change resulted from the
     *  recalculation
     */
    uint8_t CompareAndResetStats(libcomp::EnumMap<CorrectTbl, int16_t>& stats);

    /// Map of active status effects by effect type ID
    std::unordered_map<uint32_t,
        std::shared_ptr<objects::StatusEffect>> mStatusEffects;

    /// IDs of status effects currently active that deal time damage for
    /// quick access
    std::set<uint32_t> mTimeDamageEffects;

    /// Active effect type IDs mapped to cancel condition flags for quick
    /// access
    std::unordered_map<uint8_t, std::set<uint32_t>> mCancelConditions;

    /// Map of server system times mapped to the next event time associated
    /// to each active status effect. Natural HP/MP regen is stored here as
    /// a 0. Actual effects will be stored here as reserved values 1
    /// (inticating a new effect was added), 2 (indicating an efect has
    /// been updated) or 3 (indicating an effect has expired). Any other
    /// value stored will be an absolute system time when the regen or
    /// T-Damage will be applied or the effect associated will be expired.
    std::map<uint32_t, std::set<uint32_t>> mNextEffectTimes;

    /// Pointer to the current zone the entity is in
    std::shared_ptr<Zone> mCurrentZone;

    /// Set of entity IDs representing opponents that the entity is currently
    /// fighting. If an entity is in this set, this entity should be in their
    /// set as well.
    std::set<int32_t> mOpponentIDs;

    /// Map of affinity null chances by correct table ID
    libcomp::EnumMap<CorrectTbl, int16_t> mNullMap;

    /// Map of affinity reflect chances by correct table ID
    libcomp::EnumMap<CorrectTbl, int16_t> mReflectMap;

    /// Map of affinity absorb chances by correct table ID
    libcomp::EnumMap<CorrectTbl, int16_t> mAbsorbMap;

    /// true if the status effects have been activated for the current zone
    bool mEffectsActive;

    /// Signifies that the entity is alive
    bool mAlive;

    /// false if the entity has been assigned but never calculated
    bool mInitialCalc;

    /// Last timestamp the entity's state was refreshed
    uint64_t mLastRefresh;

    /// Server lock for shared resources
    std::mutex mLock;
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
    void SetEntity(const std::shared_ptr<T>& entity);

    virtual const libobjgen::UUID GetEntityUUID();

    virtual std::shared_ptr<objects::EntityStats> GetCoreStats()
    {
        return mEntity ? mEntity->GetCoreStats().Get() : nullptr;
    }

    virtual uint8_t RecalculateStats(libcomp::DefinitionManager* definitionManager);

    /**
     * Get a numeric representation (also stored in constants) of the
     * entity's current alignment
     * @param definitionManager Pointer to the definition manager to use
     *  for entities with a static LNC value
     * @return Numeric representation of the entity's current alignment
     */
    uint8_t GetLNCType(libcomp::DefinitionManager* definitionManager = nullptr);

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
