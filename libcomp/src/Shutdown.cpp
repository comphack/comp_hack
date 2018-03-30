/**
 * @file libcomp/src/Shutdown.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Shutdown signal handler.
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

#include "Shutdown.h"

// libcomp Includes
#include "BaseServer.h"
#include "Log.h"

// Standard C++11 Includes
#include <signal.h>
#include <thread>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif // HAVE_SYSTEMD

static libcomp::BaseServer *gServer = nullptr;

static std::list<std::thread*> gShutdownThreads;

void ShutdownSignalHandler(int sig)
{
    (void)sig;

    static int killCount = 0;

#ifdef HAVE_SYSTEMD
    sd_notify(0, "STOPPING=1");
#endif // HAVE_SYSTEMD

    if(0 < killCount)
    {
        printf("Someone can't wait can they?\n");
        printf("Doing a hard kill... this could corrupt data >.<\n");

        signal(sig, SIG_DFL);
        raise(sig);

        return;
    }

    killCount++;

    // Start a thread to handle the shutdown so it may use a mutex.
    gShutdownThreads.push_back(new std::thread([](){
        // Send the server the shutdown signal.
        if(nullptr != gServer)
        {
            gServer->Shutdown();
        }
    }));
}

void libcomp::Shutdown::Configure(libcomp::BaseServer *pServer)
{
    gServer = pServer;

    signal(SIGINT,  &ShutdownSignalHandler);
    signal(SIGTERM, &ShutdownSignalHandler);
}

void libcomp::Shutdown::Complete()
{
    // Clear this for the signal handler since we already left the thread.
    gServer = nullptr;

    // Join all shutdown threads to make sure they completed.
    for(auto thread : gShutdownThreads)
    {
        thread->join();

        delete thread;
    }
}
