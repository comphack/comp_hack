/**
 * @file libcomp/src/SpawnThread.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Thread to spawn new child processes.
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

#ifndef LIBCOMP_SRC_SPAWNTHREAD_H
#define LIBCOMP_SRC_SPAWNTHREAD_H

// libcomp Includes
#include "Child.h"
#include "MessageQueue.h"

// Standard C++11 Includes
#include <functional>
#include <thread>

namespace libcomp
{

class DayCare;

class SpawnThread
{
public:
    explicit SpawnThread(DayCare *pJuvy, bool printDetails = true,
        std::function<void()> onDetain = 0);
    ~SpawnThread();

    void QueueChild(const std::shared_ptr<Child>& child);

    void Run();
    void WaitForExit();
    void RequestExit();

private:
    bool mPrintDetails;

    DayCare *mDayCare;
    std::thread *mThread;

    libcomp::MessageQueue<std::shared_ptr<Child>> mRestartQueue;
    std::function<void()> mOnDetain;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_SPAWNTHREAD_H
