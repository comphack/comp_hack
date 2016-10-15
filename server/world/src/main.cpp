/**
 * @file server/world/src/main.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Main world server file.
 *
 * This file is part of the World Server (world).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

// world Includes
#include "WorldServer.h"

// libcomp Includes
#include <Log.h>
#include <Shutdown.h>

int main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    LOG_INFO("COMP_hack World Server v0.0.1 build 1\n");
    LOG_INFO("Copyright (C) 2010-2016 COMP_hack Team\n\n");

    world::WorldServer server("any", 18666);

    // Set this for the signal handler.
    libcomp::Shutdown::Configure(&server);

    // Start the main server loop (blocks until done).
    int returnCode = server.Start();

    // Complete the shutdown process.
    libcomp::Shutdown::Complete();

    LOG_INFO("Bye!\n");

    return returnCode;
}
