/**
 * @file server/channel/src/WorldClock.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief World clock time representation with all fields optional
 *  for selective comparison.
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

#ifndef SERVER_CHANNEL_SRC_WORLDCLOCK_H
#define SERVER_CHANNEL_SRC_WORLDCLOCK_H

#include <stddef.h>
#include <stdint.h>

namespace libcomp
{
class String;
}

namespace channel
{

/**
 * Multi-number representation of the time in the current world.
 */
class WorldClockTime
{
public:
    /**
     * Construct a new time with default un-set values
     */
    WorldClockTime();

    /// Current numeric moon phase representation
    /// (0 = new moon, 8 = full moon, -1 = not set)
    int8_t MoonPhase;

    /// Game time hours (-1 for not set)
    int8_t Hour;

    /// Game time minutes (-1 for not set)
    int8_t Min;

    /// System time hours (-1 for not set)
    int8_t SystemHour;

    /// System time minutes (-1 for not set)
    int8_t SystemMin;

    /**
     * Check if the time set equals another time
     * @param other Other time to compare to
     * @return true if they are the same, false if they are not
     */
    bool operator==(const WorldClockTime& other) const;

    /**
     * Check if the time set is less than another time. A time
     * is less than another if its most significant parts are
     * higher with system time being the most significant, moon
     * phase next and game time last.
     * @param other Other time to compare to
     * @return true if the time is less than the other, false if
     * it is not
     */
    bool operator<(const WorldClockTime& other) const;

    /**
     * Check if any values on the time are set
     * @return true if any values are set
     */
    bool IsSet() const;

    /**
     * Return a combined hash representation of the time
     * @return Hash representation of the time
     */
    size_t Hash() const;

    /**
     * Calculate the complete moon phase cycle offset based on the supplied
     * system time and configured game offset.
     * @param systemTime System time
     * @param gameOffset System configured offset in seconds
     * @return Complete moon phase cycle offset for the supplied times
     */
    static uint32_t GetCycleOffset(uint32_t systemTime, uint32_t gameOffset);

    /**
     * Calculate the moon phase relative time most recently passed by the
     * supplied system time and offset. Used for calculating moon based RNG.
     * System time can be adjusted by server offset or not.
     * @param systemTime System time past the moon phase time to calculate
     * @param gameOffset System configured offset in seconds set on each world
     *  clock calculation
     * @return Last moon phase system time
     */
    static uint32_t ToLastMoonPhaseTime(uint32_t systemTime,
        uint32_t gameOffset);
};

/**
 * Multi-number representation of the time in the current world
 * containing more time information than WorldClockTime as well
 * as an adjustable offset and calculation info.
 */
class WorldClock : public WorldClockTime
{
public:
    /**
     * Construct a new clock with default un-set values
     */
    WorldClock();

    /**
     * Construct a new clock based off a system time and game offset
     * @param systemTime System time
     * @param gameOffset System configured offset in seconds
     * @param serverOffset Server adjusted offset in seconds
     */
    WorldClock(time_t systemTime, uint32_t gameOffset,
        int32_t serverOffset);

    /**
     * Determine if the game time recorded is considered night which
     * is active between 1800 and 0599
     * @return true if it is night, false if it is day or unspecified
     */
    bool IsNight() const;

    /**
     * Get the world clock as a string in the format [hh:mm pp/16 (HH:MM)]
     * with hh:mm as world time, pp as moon phase and HH:MM as system time.
     * @return World clock in string format
     */
    libcomp::String ToString() const;

    /// Week day numeric respresentation
    /// (1 = Sunday, 7 = Saturday, -1 = not set)
    int8_t WeekDay;

    /// Month numeric respresentation
    /// (1 = January, 12 = December, -1 = not set)
    int8_t Month;

    /// Day of the month numeric respresentation (-1 for not set)
    int8_t Day;

    /// System time seconds (-1 for not set)
    int8_t SystemSec;

    /// System timestamp used to calculate the clock (0 for not set)
    uint32_t SystemTime;

    /// Custom offset in seconds to offset all calculations by
    uint32_t GameOffset;

    /// Number of seconds into the current time complete moon phase
    /// cycle, used to calculate both moon phase and game time
    uint32_t CycleOffset;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_WORLDCLOCK_H
