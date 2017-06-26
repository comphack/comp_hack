/**
 * @file libcomp/src/WatchThread.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Thread to monitor child processes for exit.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#include "WatchThread.h"
#include "DayCare.h"

// Linux Includes
#include <sys/types.h>
#include <sys/wait.h>

using namespace libcomp;

WatchThread::WatchThread(DayCare *pJuvy) : mDayCare(pJuvy)
{
    mThread = new std::thread([](WatchThread *pThread){
        pThread->Run();
    }, this);
}

WatchThread::~WatchThread()
{
    try
    {
        mThread->join();
    }
    catch(const std::system_error& e)
    {
    }

    delete mThread;
    mThread = nullptr;
}

void WatchThread::Run()
{
    while(mDayCare->IsRunning() || mDayCare->HaveChildren())
    {
        int status;
        pid_t pid = wait(&status);

        mDayCare->NotifyExit(pid, status);

        // If we have no children, wait a bit for more to spawn.
        if(!mDayCare->HaveChildren())
        {
            usleep(1000000); // 1 second
        }
    }
}

void WatchThread::WaitForExit()
{
    try
    {
        mThread->join();
    }
    catch(const std::system_error& e)
    {
    }
}
