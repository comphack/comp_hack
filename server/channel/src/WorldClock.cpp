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

#include "WorldClock.h"

// libcomp Includes
#include <CString.h>
#include <Constants.h>
#include <ScriptEngine.h>

using namespace channel;

namespace libcomp {
template <>
BaseScriptEngine& BaseScriptEngine::Using<WorldClockTime>() {
  if (!BindingExists("WorldClockTime", true)) {
    Sqrat::Class<WorldClockTime> binding(mVM, "WorldClockTime");
    binding.Var("MoonPhase", &WorldClockTime::MoonPhase)
        .Var("Hour", &WorldClockTime::Hour)
        .Var("Min", &WorldClockTime::Min)
        .Var("SystemHour", &WorldClockTime::SystemHour)
        .Var("SystemMin", &WorldClockTime::SystemMin)
        .StaticFunc("GetCycleOffset", &WorldClockTime::GetCycleOffset)
        .StaticFunc("ToLastMoonPhaseTime",
                    &WorldClockTime::ToLastMoonPhaseTime);

    Bind<WorldClockTime>("WorldClockTime", binding);
  }

  return *this;
}

template <>
BaseScriptEngine& BaseScriptEngine::Using<WorldClock>() {
  if (!BindingExists("WorldClock", true)) {
    Using<WorldClockTime>();

    Sqrat::DerivedClass<WorldClock, WorldClockTime> binding(mVM, "WorldClock");
    binding.Var("WeekDay", &WorldClock::WeekDay)
        .Var("Month", &WorldClock::Month)
        .Var("Day", &WorldClock::Day)
        .Var("SystemSec", &WorldClock::SystemSec)
        .Var("SystemTime", &WorldClock::SystemTime);

    Bind<WorldClock>("WorldClock", binding);
  }

  return *this;
}
}  // namespace libcomp

WorldClockTime::WorldClockTime()
    : MoonPhase(-1), Hour(-1), Min(-1), SystemHour(-1), SystemMin(-1) {}

bool WorldClockTime::operator==(const WorldClockTime& other) const {
  return Hash() == other.Hash();
}

bool WorldClockTime::operator<(const WorldClockTime& other) const {
  return Hash() < other.Hash();
}

bool WorldClockTime::IsSet() const {
  return MoonPhase != -1 || Hour != -1 || Min != -1 || SystemHour != -1 ||
         SystemMin != -1;
}

size_t WorldClockTime::Hash() const {
  // System time carries the most weight, then moon phase, then game time
  size_t sysPart =
      (size_t)((SystemHour < 0 || SystemMin < 0 ||
                ((SystemHour * 100 + SystemMin) > 2400))
                   ? (size_t)0
                   : ((size_t)(10000 + SystemHour * 100 + SystemMin) *
                      (size_t)100000000));
  size_t moonPart =
      (size_t)((MoonPhase < 0 || MoonPhase >= 16)
                   ? (size_t)0
                   : ((size_t)(100 + MoonPhase) * (size_t)100000));
  size_t timePart =
      (size_t)((Hour < 0 || Min < 0 || ((Hour * 100 + Min) > 2400))
                   ? (size_t)0
                   : (size_t)(10000 + Hour * 100 + Min));

  return (size_t)(sysPart + moonPart + timePart);
}

uint32_t WorldClockTime::GetCycleOffset(uint32_t systemTime,
                                        uint32_t gameOffset) {
  // Every 4 days, 15 full moon cycles will elapse and the same game time
  // will occur on the same time offset.
  return (uint32_t)((systemTime + gameOffset - BASE_WORLD_TIME) % 345600);
}

uint32_t WorldClockTime::ToLastMoonPhaseTime(uint32_t systemTime,
                                             uint32_t gameOffset) {
  uint32_t cycleOffset = GetCycleOffset(systemTime, gameOffset);

  // Get the number of seconds from cycle start for the calculated phase.
  // This differs from normal moon phase calculation in the sense that we
  // want to preserve number of sub-cycles (ex: second new moon is the 17th
  // occurence within the main cycle).
  uint8_t calcCyclePhase = (uint8_t)(cycleOffset / 1440);
  uint32_t phaseOffset = (uint32_t)(calcCyclePhase * 1440);

  // Calculate the last beginning of a cycle and add the offset
  return (uint32_t)((systemTime + gameOffset) - cycleOffset + phaseOffset);
}

WorldClock::WorldClock()
    : WeekDay(-1),
      Month(-1),
      Day(-1),
      SystemSec(-1),
      SystemTime(0),
      GameOffset(0),
      CycleOffset(0) {}

WorldClock::WorldClock(time_t systemTime, uint32_t gameOffset,
                       int32_t serverOffset) {
  // Set standard GMT system time and game offset
  SystemTime = (uint32_t)systemTime;
  GameOffset = gameOffset;

  // Adjust based on the server offset for the rest of the calculations
  if (serverOffset) {
    systemTime = (systemTime + serverOffset);
  }

  tm* t = gmtime(&systemTime);

  WeekDay = (int8_t)(t->tm_wday + 1);
  Month = (int8_t)(t->tm_mon + 1);
  Day = (int8_t)t->tm_mday;
  SystemHour = (int8_t)t->tm_hour;
  SystemMin = (int8_t)t->tm_min;
  SystemSec = (int8_t)t->tm_sec;

  // Get the cycle offset and calculate the game relative times
  CycleOffset = GetCycleOffset(SystemTime, GameOffset);

  // 24 minutes = 1 game phase (16 total)
  MoonPhase = (int8_t)((CycleOffset / 1440) % 16);

  // 2 minutes = 1 game hour
  Hour = (int8_t)((CycleOffset / 120) % 24);

  // 2 seconds = 1 game minute
  Min = (int8_t)((CycleOffset / 2) % 60);
}

bool WorldClock::IsNight() const {
  return Hour >= 0 && (Hour <= 5 || Hour >= 18);
}

libcomp::String WorldClock::ToString() const {
  std::vector<libcomp::String> time;
  for (int8_t num : {Hour, Min, MoonPhase, SystemHour, SystemMin}) {
    if (num < 0) {
      time.push_back("NA");
    } else {
      time.push_back(libcomp::String("%1%2").Arg(num < 10 ? "0" : "").Arg(num));
    }
  }

  return libcomp::String("%1:%2 %3/16 [%4:%5]")
      .Arg(time[0])
      .Arg(time[1])
      .Arg(time[2])
      .Arg(time[3])
      .Arg(time[4]);
}
