/**
 * @file server/channel/src/PerformanceTimer.h
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

#ifndef SERVER_CHANNEL_SRC_PERFORMANCETIMER_H
#define SERVER_CHANNEL_SRC_PERFORMANCETIMER_H

// Standard C++11 Includes
#include <stdint.h>

// libcomp Includes
#include <CString.h>

namespace channel
{

class ChannelServer;

#ifndef ServerTime
typedef uint64_t ServerTime;
#endif // ServerTime

/**
 * Timer to measure performance of a task.
 */
class PerformanceTimer
{
protected:
    /// Channel server pointer. Should stay valid while the object exists.
    ChannelServer *mServer;

    /// Start time of the performance measurement.
    ServerTime mStart;

    /// If the performance monitor is enabled.
    bool mEnabled;

public:
    /**
     * Create the performance timer.
     * @param pServer Channel server to create the timer for.
     */
    PerformanceTimer(ChannelServer *pServer);

    /**
     * Start a performance measurement.
     */
    void Start();

    /**
     * Stop a performance measurement and log it.
     * @param metric Name of the task that was measured.
     */
    void Stop(const libcomp::String& metric);
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_PERFORMANCETIMER_H
