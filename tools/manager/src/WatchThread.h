/**
 * @file tools/manager/src/WatchThread.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Thread to monitor child processes for exit.
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

#ifndef TOOLS_MANAGER_SRC_WATCHTHREAD_H
#define TOOLS_MANAGER_SRC_WATCHTHREAD_H

// manager Includes
#include "Child.h"

// Standard C++11 Includes
#include <thread>

// libcomp Includes
#include <MessageQueue.h>

namespace manager
{

class DayCare;

class WatchThread
{
public:
    explicit WatchThread(DayCare *pJuvy);
    ~WatchThread();

    void Run();
    void WaitForExit();

private:
    DayCare *mDayCare;
    std::thread *mThread;
};

} // namespace manager

#endif // TOOLS_MANAGER_SRC_WATCHTHREAD_H
