/**
 * @file server/channel/src/PerformanceTimer.cpp
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Timer to measure performance of a task.
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

#include "PerformanceTimer.h"

// libcomp Includes
#include <Log.h>

// object Includes
#include <ChannelConfig.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

PerformanceTimer::PerformanceTimer(ChannelServer *pServer) : mServer(pServer),
    mStart(0)
{
    auto config = std::dynamic_pointer_cast<objects::ChannelConfig>(
        pServer->GetConfig());
    mEnabled = config->GetPerfMonitorEnabled();
}

void PerformanceTimer::Start()
{
    if(mEnabled)
    {
        mStart = mServer->GetServerTime();
    }
}

void PerformanceTimer::Stop(const libcomp::String& metric)
{
    if(mEnabled)
    {
        ServerTime diff = mServer->GetServerTime() - mStart;

        LogGeneralDebug([&]()
        {
            return libcomp::String("PERF: %1 in %2 us\n")
                .Arg(metric).Arg(diff);
        });
    }
}
