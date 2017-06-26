/**
 * @file libcomp/src/SpawnThread.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Thread to spawn new child processes.
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

#include "SpawnThread.h"
#include "DayCare.h"

using namespace libcomp;

SpawnThread::SpawnThread(DayCare *pJuvy, bool printDetails) :
    mPrintDetails(printDetails), mDayCare(pJuvy)
{
    mThread = new std::thread([](SpawnThread *pThread){
        pThread->Run();
    }, this);
}

SpawnThread::~SpawnThread()
{
    QueueChild(nullptr);

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

void SpawnThread::QueueChild(const std::shared_ptr<Child>& child)
{
    mRestartQueue.Enqueue(child);
}

void SpawnThread::Run()
{
    bool running = true;
   
    while(running)
    {
        std::list<std::shared_ptr<Child>> children;
        mRestartQueue.DequeueAll(children);

        for(auto child : children)
        {
            if(!child)
            {
                running = false;
            }
        }

        children = mDayCare->OrderChildren(children);

        if(running)
        {
            for(auto child : children)
            {
                if(child->Start())
                {
                    if(mPrintDetails)
                    {
                        printf("Started with PID %d: %s\n", child->GetPID(),
                            child->GetCommandLine().c_str());
                    }

                    int timeout = child->GetBootTimeout();

                    if(0 != timeout)
                    {
                        usleep((uint32_t)(timeout * 1000));
                    }
                }
                else
                {
                    printf("Failed to start: %s\n",
                        child->GetCommandLine().c_str());
                }
            }
        }
    }
}

void SpawnThread::WaitForExit()
{
    try
    {
        mThread->join();
    }
    catch(const std::system_error& e)
    {
    }
}

void SpawnThread::RequestExit()
{
    QueueChild(nullptr);
}
