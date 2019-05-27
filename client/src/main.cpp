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
#include <LobbyClient.h>

// libcomp Includes
#include <Decrypt.h>
#include <Exception.h>
#include <Log.h>
#include <ScriptEngine.h>

// Standard C++ Includes
#include <iostream>

// Standard C Includes
#include <cstdlib>
#include <cstdio>

static bool gRunning = true;
static int gReturnCode = EXIT_SUCCESS;
static libcomp::ScriptEngine *gEngine = nullptr;

static void ScriptExit(int returnCode)
{
    gReturnCode = returnCode;
    gRunning = false;
}

static void ScriptInclude(const char *szPath)
{
    std::vector<char> file = libcomp::Decrypt::LoadFile(szPath);

    if(file.empty())
    {
        std::cerr << "Failed to include script file: "
            << szPath << std::endl;

        return;
    }

    file.push_back(0);

    if(!gEngine->Eval(&file[0], szPath))
    {
        std::cerr << "Failed to run script file: "
            << szPath << std::endl;
    }
}

static void ScriptSleep(int seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

static void ScriptSleepMS(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int8_t SystemHour, SystemMin, Min, Hour, MoonPhase;

size_t Hash()
{
    // System time carries the most weight, then moon phase, then game time
    return (size_t)((SystemHour < 0 || SystemMin < 0 ||
        (((int)SystemHour * 100 + (int)SystemMin) > 2400)) ? 0ULL
        : ((size_t)(10000 + (int)SystemHour * 100 + (int)SystemMin) * (size_t)100000000ULL)) +
        (size_t)((MoonPhase < 0 || MoonPhase >= 16)
            ? (size_t)0ULL : ((size_t)(100 + (int)MoonPhase) * (size_t)100000ULL)) +
        (size_t)((Hour < 0 || Min < 0 || (((int)Hour * 100 + (int)Min) > 2400))
            ? (size_t)0ULL : (size_t)(10000 + (int)Hour * 100 + (int)Min));
}

void RunInteractive(libcomp::ScriptEngine& engine)
{
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
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    libcomp::Exception::RegisterSignalHandler();

    // Enable the log so it prints to the console.
    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    // Create the script engine.
    libcomp::ScriptEngine engine(true);

    // Register the exit function.
    Sqrat::RootTable(engine.GetVM()).Func("exit", ScriptExit);
    Sqrat::RootTable(engine.GetVM()).Func("include", ScriptInclude);
    Sqrat::RootTable(engine.GetVM()).Func("sleep", ScriptSleep);
    Sqrat::RootTable(engine.GetVM()).Func("sleep_ms", ScriptSleepMS);

    // Set the global for the engine.
    gEngine = &engine;

    // Register the client testing classes.
    engine.Using<libtester::LobbyClient>();
    engine.Using<libtester::ChannelClient>();

    if(1 >= argc)
    {
        RunInteractive(engine);
    }
    else
    {
        for(int i = 1; i < argc; ++i)
        {
            std::vector<char> file = libcomp::Decrypt::LoadFile(argv[i]);

            if(file.empty())
            {
                std::cerr << "Failed to open script file: "
                    << argv[i] << std::endl;

                return EXIT_FAILURE;
            }

            file.push_back(0);

            if(!engine.Eval(&file[0], argv[i]))
            {
                std::cerr << "Failed to run script file: "
                    << argv[i] << std::endl;

                return EXIT_FAILURE;
            }
        }
    }

    return gReturnCode;
}
