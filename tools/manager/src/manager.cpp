/**
 * @file tools/manager/src/manager.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main application.
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

#include <signal.h>

#include <iostream>

#include "Child.h"
#include "DayCare.h"

using namespace manager;

static DayCare *gDayCare = nullptr;
static volatile bool gTerm = false;

void SignalHandler(int signum)
{
    static int killCount = 0;

    switch(signum)
    {
        case SIGUSR1:
        {
            printf("Got SIGUSR1. Printing status...\n");

            if(nullptr != gDayCare)
            {
                gDayCare->PrintStatus();
            }
            break;
        }
        case SIGINT:
        {
            printf("Got SIGINT. Interrupting applications...\n");

            gTerm = true;

            if(nullptr != gDayCare)
            {
                gDayCare->CloseDoors(false);
            }

            killCount++;
            break;
        }
        case SIGTERM:
        {
            printf("Got SIGTERM. Killing applications...\n");

            gTerm = true;

            if(nullptr != gDayCare)
            {
                gDayCare->CloseDoors(true);
            }

            killCount++;
            break;
        }
    }

    if(3 <= killCount)
    {
        fprintf(stderr, "Killing everything at request of user.\n");

        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    signal(SIGUSR1, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGINT, SignalHandler);

    printf("Manager started with PID %d\n", getpid());

    try
    {
        DayCare juvy;

        if(!juvy.DetainMonsters("programs.xml"))
        {
            fprintf(stderr, "Failed to load programs XML.\n");

            return EXIT_FAILURE;
        }

        gDayCare = &juvy;
        juvy.WaitForExit();
        gDayCare = nullptr;

    }
    catch(const std::system_error& e)
    {
        std::cerr << "Caught system_error with code " << e.code() 
            << " meaning " << e.what() << '\n';
    }

    printf("Manager stopped.\n");

    return EXIT_SUCCESS;
}
