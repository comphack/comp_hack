/**
 * @file libcomp/src/TimerManager.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage timed events.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#ifndef LIBCOMP_SRC_TIMERMANAGER_H
#define LIBCOMP_SRC_TIMERMANAGER_H

// libcomp Includes
#include "MessageExecute.h"

#include <chrono>
#include <list>
#include <set>

#include <condition_variable>
#include <thread>

namespace libcomp
{

class TimerEvent;

class TimerEventComp
{
public:
    bool operator()(const TimerEvent *lhs, const TimerEvent *rhs) const;
};

class TimerManager
{
public:
    TimerManager();
    ~TimerManager();

    TimerEvent* RegisterEvent(
        const std::chrono::steady_clock::time_point& time,
        libcomp::Message::Execute *pMessage);
    TimerEvent* RegisterPeriodicEvent(
        const std::chrono::milliseconds& period,
        libcomp::Message::Execute *pMessage);
    void CancelEvent(TimerEvent *pEvent);

    /**
     * Executes code in the worker thread.
     * @param f Function (lambda) to execute in the worker thread.
     * @param args Arguments to pass to the function when it is executed.
     * @return true on success, false on failure
     */
    template<typename Function, typename... Args>
    TimerEvent* ScheduleEvent(
        const std::chrono::steady_clock::time_point& time,
        Function&& f, Args&&... args)
    {
        auto msg = new libcomp::Message::ExecuteImpl<Args...>(
            std::forward<Function>(f), std::forward<Args>(args)...);

        return RegisterEvent(time, msg);
    }

    /**
     * Executes code in the worker thread.
     * @param f Function (lambda) to execute in the worker thread.
     * @param args Arguments to pass to the function when it is executed.
     * @return true on success, false on failure
     */
    template<typename Function, typename... Args>
    TimerEvent* ScheduleEventIn(int seconds,
        Function&& f, Args&&... args)
    {
        auto msg = new libcomp::Message::ExecuteImpl<Args...>(
            std::forward<Function>(f), std::forward<Args>(args)...);

        return RegisterEvent(std::chrono::steady_clock::now() +
            std::chrono::seconds(seconds), msg);
    }

    /**
     * Executes code in the worker thread.
     * @param f Function (lambda) to execute in the worker thread.
     * @param args Arguments to pass to the function when it is executed.
     * @return true on success, false on failure
     */
    template<typename Function, typename... Args>
    TimerEvent* SchedulePeriodicEvent(
        const std::chrono::milliseconds& period,
        Function&& f, Args&&... args)
    {
        auto msg = new libcomp::Message::ExecuteImpl<Args...>(
            std::forward<Function>(f), std::forward<Args>(args)...);

        return RegisterPeriodicEvent(period, msg);
    }

private:
    void ProcessEvents(std::unique_lock<std::mutex>& lock);
    void WaitForEvent(std::unique_lock<std::mutex>& lock);

    volatile bool mRunning;
    volatile bool mProcessingEvents;
    std::multiset<TimerEvent*, TimerEventComp> mEvents;
    std::condition_variable mEventCondition;
    std::mutex mEventLock;
    std::thread mRunThread;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_TIMERMANAGER_H
