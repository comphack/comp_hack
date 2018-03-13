/**
 * @file server/channel/src/WorldClock.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief World clock time representation with all fields optional
 *  for selective comparison.
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

#include "WorldClock.h"

using namespace channel;

WorldClockTime::WorldClockTime() :
    MoonPhase(-1), Hour(-1), Min(-1), SystemHour(-1), SystemMin(-1)
{
}

bool WorldClockTime::operator==(const WorldClockTime& other) const
{
    return Hash() == other.Hash();
}

bool WorldClockTime::operator<(const WorldClockTime& other) const
{
    return Hash() < other.Hash();
}

bool WorldClockTime::IsSet() const
{
    return MoonPhase != -1 ||
        Hour != -1 ||
        Min != -1 ||
        SystemHour != -1 ||
        SystemMin != -1;
}

size_t WorldClockTime::Hash() const
{
    // System time carries the most weight, then moon phase, then game time
    size_t sysPart = (size_t)((SystemHour < 0 || SystemMin < 0 ||
        ((SystemHour * 100 + SystemMin) > 2400)) ? (size_t)0
        : ((size_t)(10000 + SystemHour * 100 + SystemMin) * (size_t)100000000));
    size_t moonPart = (size_t)((MoonPhase < 0 || MoonPhase >= 16)
        ? (size_t)0 : ((size_t)(100 + MoonPhase) * (size_t)100000));
    size_t timePart = (size_t)((Hour < 0 || Min < 0 || ((Hour * 100 + Min) > 2400))
        ? (size_t)0 : (size_t)(10000 + Hour * 100 + Min));

    return (size_t)(sysPart + moonPart + timePart);
}

WorldClock::WorldClock() :
    WeekDay(-1), Month(-1), Day(-1), SystemSec(-1), SystemTime(0),
    GameOffset(0), CycleOffset(0)
{
}
