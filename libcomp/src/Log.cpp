/**
 * @file libcomp/src/Log.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Routines to log messages to the console and/or a file.
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

#include "Log.h"

#include <iostream>
#include <cassert>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <wincon.h>
#else
#include <unistd.h>
#endif // _WIN32

using namespace libcomp;

/**
 * @internal
 * Singleton pointer for the Log class.
 */
static Log *gLogInst = nullptr;

/*
 * Black       0;30     Dark Gray     1;30
 * Blue        0;34     Light Blue    1;34
 * Green       0;32     Light Green   1;32
 * Cyan        0;36     Light Cyan    1;36
 * Red         0;31     Light Red     1;31
 * Purple      0;35     Light Purple  1;35
 * Brown       0;33     Yellow        1;33
 * Light Gray  0;37     White         1;37
 */

/**
 * Log hook to send all log messages to standard output. This hook will color
 * all log messages depending on their log level.
 * @param level Numeric level representing the log level.
 * @param msg The message to write to standard output.
 * @param pUserData User defined data that was passed with the hook to
 * @ref Log::AddLogHook.
 */
static void LogToStandardOutput(Log::Level_t level,
    const String& msg, void *pUserData)
{
    // Console colors for each log level.
#ifdef _WIN32
    static const WORD gLogColors[Log::LOG_LEVEL_COUNT] = {
        // Debug
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        // Info
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
            FOREGROUND_INTENSITY,
        // Warning
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        // Error
        FOREGROUND_RED | FOREGROUND_INTENSITY,
        // Critical
        FOREGROUND_RED | FOREGROUND_INTENSITY,
    };
#else
    static const String gLogColors[Log::LOG_LEVEL_COUNT] = {
        "\e[1;32;40m", // Debug
        "\e[37;40m",   // Info
        "\e[1;33;40m", // Warning
        "\e[1;31;40m", // Error
        "\e[1;37;41m", // Critical
    };
#endif // _WIN32

    // This hook has no user data.
    (void)pUserData;

    if(0 > level || Log::LOG_LEVEL_COUNT <= level)
    {
        level = Log::LOG_LEVEL_CRITICAL;
    }

    // Split the message into lines. Each line will be individually colored.
    std::list<String> msgs = msg.Split("\n");
    String last = msgs.back();
    msgs.pop_back();

    // Each log level has a different color scheme.
    for(String m : msgs)
    {
#if _WIN32
        (void)SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
            gLogColors[level]);

        std::cout << m.ToUtf8();

        (void)SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        std::cout << std::endl;
#else
        if(isatty(fileno(stdout)))
        {
            std::cout << gLogColors[level] << m.ToUtf8()
                << "\e[0K\e[0m" << std::endl;
        }
        else
        {
            std::cout << m.ToUtf8() << std::endl;
        }
#endif // _WIN32
    }

    // If there is more on the last line, print it as well.
    if(!last.IsEmpty())
    {
#if _WIN32
        (void)SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
            gLogColors[level]);

        std::cout << last.ToUtf8();

        (void)SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
        if(isatty(fileno(stdout)))
        {
            std::cout << gLogColors[level] << last.ToUtf8() << "\e[0K\e[0m";
        }
        else
        {
            std::cout << last.ToUtf8();
        }
#endif // _WIN32
    }

    // Flush the output so the log messages are immediately avaliable.
    std::cout.flush();
}

Log::Log() : mLogFile(nullptr)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

    if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
        &consoleInfo))
    {
        mConsoleAttributes = consoleInfo.wAttributes;
    }
    else
    {
        mConsoleAttributes = FOREGROUND_RED | FOREGROUND_GREEN |
            FOREGROUND_BLUE;
    }

    (void)SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif // _WIN32

    // Default all log levels to enabled.
    for(int i = 0; i < LOG_LEVEL_COUNT; ++i)
    {
        mLogEnables[i] = true;
    }

    mLogFileTimestampEnabled = false;
}

Log::~Log()
{
    // Lock the muxtex.
    std::lock_guard<std::mutex> lock(mLock);

#ifdef _WIN32
    (void)SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        mConsoleAttributes);
#else
    // Clear the last line before the server exits.
    std::cout << "\e[0K\e[0m";
#endif // _WIN32

    // Close the log file.
    delete mLogFile;
    mLogFile = nullptr;

    // Remove the singleton pointer.
    gLogInst = nullptr;
}

Log* Log::GetSingletonPtr()
{
    // If the singleton does not exist, create it and ensure there is a valid
    // pointer before returning it.
    if(nullptr == gLogInst)
    {
        gLogInst = new Log;
    }

    assert(nullptr != gLogInst);

    return gLogInst;
}

void Log::LogMessage(Log::Level_t level, const String& msg)
{
    // Prepend these to messages.
    static const String gLogMessages[Log::LOG_LEVEL_COUNT] = {
        "DEBUG: %1",
        "%1",
        "WARNING: %1",
        "ERROR: %1",
        "CRITICAL: %1",
    };

    // Log a critical error message. If the configuration option is true, log
    // the message to the log file. Regardless, pass the message to all the
    // log hooks for processing. Critical messages have the text "CRITICAL: "
    // appended to them.
    if(0 > level || LOG_LEVEL_COUNT <= level || !mLogEnables[level])
        return;

    String final = String(gLogMessages[level]).Arg(msg);

    // Lock the muxtex.
    std::lock_guard<std::mutex> lock(mLock);

    if(nullptr != mLogFile)
    {
        if(mLogFileTimestampEnabled)
        {
            auto currentTime = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());

            std::stringstream ss;
            ss << std::put_time(std::localtime(&currentTime), "%Y/%m/%d %T");

            String formattedTime = String("[%1] ").Arg(ss.str());

            mLogFile->write(formattedTime.C(),
                (std::streamsize)(formattedTime.Length() * sizeof(char)));
        }

        mLogFile->write(final.C(),
            (std::streamsize)(final.Length() * sizeof(char)));
        mLogFile->flush();
    }

    // Call all hooks.
    for(auto i : mHooks)
    {
        (*i.first)(level, final, i.second);
    }

    // Call all lambda hooks.
    for(auto func : mLambdaHooks)
    {
        func(level, final);
    }
}

String Log::GetLogPath() const
{
    return mLogPath;
}

void Log::SetLogPath(const String& path, bool truncate)
{
    bool loaded = true;

    {
        // Lock the muxtex.
        std::lock_guard<std::mutex> lock(mLock);

        // Set the log path.
        mLogPath = path;

        // Close the old log file if it's open.
        if(nullptr != mLogFile)
        {
            delete mLogFile;
            mLogFile = nullptr;
        }

        // If the log path isn't empty, log to a file. The file will be
        // truncated and created new first if truncate is set.
        if(!mLogPath.IsEmpty())
        {
            int mode = std::ofstream::out;
            if(truncate)
            {
                mode |= std::ofstream::trunc;
            }
            else
            {
                mode |= std::ofstream::app;
            }

            mLogFile = new std::ofstream();
            mLogFile->open(mLogPath.C(), (std::ios_base::openmode)mode);
            mLogFile->flush();

            // If this failed, close it.
            if(!mLogFile->good())
            {
                delete mLogFile;
                mLogFile = nullptr;
                mLogPath.Clear();
                loaded = false;
            }
        }
    }

    if(!loaded)
    {
        LOG_CRITICAL("Failed to open the log file for writing.\n");
        LOG_CRITICAL("The application will now close.\n");
        LOG_INFO("Bye!\n");

        exit(EXIT_FAILURE);
    }
}

void Log::AddLogHook(Log::Hook_t func, void *data)
{
    // Lock the muxtex.
    std::lock_guard<std::mutex> lock(mLock);

    // Add the specified log hook.
    mHooks[func] = data;
}

void Log::AddLogHook(const std::function<void(Level_t level,
    const String& msg)>& func)
{
    mLambdaHooks.push_back(func);
}

void Log::AddStandardOutputHook()
{
    // Add the default hook to log all messages to the terminal.
    AddLogHook(&LogToStandardOutput);
}

void Log::ClearHooks()
{
    // Lock the muxtex.
    std::lock_guard<std::mutex> lock(mLock);

    // Remove all hooks.
    mHooks.clear();
    mLambdaHooks.clear();
}

bool Log::GetLogLevelEnabled(Level_t level) const
{
    // Sanity check.
    if(0 > level || LOG_LEVEL_COUNT <= level)
    {
        return false;
    }

    // Get if the level is enabled.
    return mLogEnables[level];
}

void Log::SetLogLevelEnabled(Level_t level, bool enabled)
{
    // Sanity check.
    if(0 > level || LOG_LEVEL_COUNT <= level)
    {
        return;
    }

    // Lock the mutex.
    std::lock_guard<std::mutex> lock(mLock);

    // Set if the level is enabled.
    mLogEnables[level] = enabled;
}

bool Log::GetLogFileTimestampsEnabled() const
{
    // Get if the log file timestamps are enabled.
    return mLogFileTimestampEnabled;
}

void Log::SetLogFileTimestampsEnabled(bool enabled)
{
    // Set if the log file timestamps are enabled.
    mLogFileTimestampEnabled = enabled;
}
