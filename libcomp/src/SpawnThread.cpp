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

#include "SpawnThread.h"
#include "DayCare.h"

#include <signal.h>

using namespace libcomp;

pthread_t gSelf;

SpawnThread::SpawnThread(DayCare *pJuvy, bool printDetails,
    std::function<void()> onDetain) : mPrintDetails(printDetails),
    mDayCare(pJuvy), mOnDetain(onDetain)
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

    gSelf = pthread_self();

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
                int timeout = child->GetBootTimeout();

                if(child->Start(true))
                {
                    if(mPrintDetails)
                    {
                        printf("Started with PID %d: %s\n", child->GetPID(),
                            child->GetCommandLine().c_str());
                    }

                    if(0 != timeout)
                    {
                        sigset_t set;
                        int sig;
                        struct timespec tm;

                        tm.tv_sec = timeout / 1000;
                        tm.tv_nsec = (timeout % 1000) * 1000;

                        sigemptyset(&set);
                        sigaddset(&set, SIGUSR2);

                        pthread_sigmask(SIG_BLOCK, &set, NULL);

                        if(0 > (sig = sigtimedwait(&set, NULL, &tm)))
                        {
                            printf("Failed to start: %s\n",
                                child->GetCommandLine().c_str());
                        }

                        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
                    }
                }
                else
                {
                    printf("Failed to start: %s\n",
                        child->GetCommandLine().c_str());
                }
            }

            if(mOnDetain)
            {
                mOnDetain();
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
