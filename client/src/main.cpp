/**
 * @file client/src/main.cpp
 * @ingroup client
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main client source file.
 *
 * This file is part of the Client (client).
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

// libtester Includes
#include <ChannelClient.h>

// libcomp Includes
#include <Log.h>
#include <ScriptEngine.h>

// Standard C++ Includes
#include <iostream>

// Standard C Includes
#include <cstdlib>
#include <cstdio>

static bool gRunning = true;
static int gReturnCode = EXIT_SUCCESS;

static void ScriptExit(int returnCode)
{
    gReturnCode = returnCode;
    gRunning = false;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    // Enable the log so it prints to the console.
    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    // Create the script engine.
    libcomp::ScriptEngine engine(true);

    // Register the exit function.
    Sqrat::RootTable(engine.GetVM()).Func("exit", ScriptExit);

    // Register the client testing classes.
    engine.Using<libtester::ChannelClient>();

    libcomp::String code, script;

    std::cout << "sq> ";

    int depth = 0;

    while(gRunning)
    {
        char c = (char)std::getchar();

        code += libcomp::String(&c, 1);

        switch(c)
        {
            case '{':
            {
                depth++;
                break;
            }
            case '}':
            {
                depth--;
                break;
            }
            case '\n':
            {
                if(0 == depth)
                {
                    engine.Eval(code);
                    script += code;
                    code.Clear();

                    if(gRunning)
                    {
                        std::cout << "sq> ";
                    }
                }

                break;
            }
        }
    }

    std::cout << "Final script: " << std::endl << script.C();

    return gReturnCode;
}
