/**
 * @file tools/manager/src/SpawnThread.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Thread to spawn new child processes.
 *
 * This tool will spawn and manage server processes.
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_MANAGER_SRC_SPAWNTHREAD_H
#define TOOLS_MANAGER_SRC_SPAWNTHREAD_H

// manager Includes
#include "Child.h"

// Standard C++11 Includes
#include <thread>

// libcomp Includes
#include <MessageQueue.h>

namespace manager
{

class DayCare;

class SpawnThread
{
public:
    explicit SpawnThread(DayCare *pJuvy);
    ~SpawnThread();

    void QueueChild(const std::shared_ptr<Child>& child);

    void Run();
    void WaitForExit();
    void RequestExit();

private:
    DayCare *mDayCare;
    std::thread *mThread;

    libcomp::MessageQueue<std::shared_ptr<Child>> mRestartQueue;
};

} // namespace manager

#endif // TOOLS_MANAGER_SRC_SPAWNTHREAD_H
