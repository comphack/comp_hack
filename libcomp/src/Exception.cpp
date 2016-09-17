/**
 * @file libcomp/src/Exception.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation of the base Exception class.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#include "Exception.h"

#include "Constants.h"
#include "Log.h"

#ifdef _WIN32
#include <windows.h>
//#include <dbghelp.h>
#else // _WIN32
#include <regex_ext>
#include <execinfo.h>
#include <cxxabi.h>
#endif // _WIN32

#include <sstream>
#include <cstdlib>

using namespace libcomp;

/// If the module name should be stripped from the backtrace.
#define EXCEPTION_STRIP_MODULE (0)

#if __FreeBSD__
typedef size_t  backtrace_size_t;
#elif __linux__
typedef int     backtrace_size_t;
#endif

/**
 * Length of the absolute path to the source directory to strip from backtrace
 * paths. Calculate the length of the path to the project so we may remove that
 * portion of the path from the exception.
 */
static size_t baseLen = strlen(__FILE__) - strlen("libcomp/src/Exception.cpp");

Exception::Exception(const String& msg, const String& f, int l) :
    mLine(l), mFile(f), mMessage(msg)
{
#ifdef _WIN32
    /// @todo Implement backtraces on Windows.
#else // _WIN32
    // Array to store each backtrace address.
    void *backtraceAddresses[MAX_BACKTRACE_DEPTH];

    // Populate the array of backtrace addresses and get how many were added.
    backtrace_size_t backtraceSize = ::backtrace(backtraceAddresses, MAX_BACKTRACE_DEPTH);

    // If we have a valid array of backtraces, parse them.
    if(backtraceSize > 0)
    {
        // Retrieve the symbols for each backtrace in the array.
        char **backtraceSymbols = backtrace_symbols(
            backtraceAddresses, backtraceSize);

        // If the symbols were created, parse then.
        if(backtraceSymbols)
        {
            // For each symbol in the array, convert it to a String and add it
            // to the backtrace string list. Set i = 1 to skip over this
            // constructor function.
            for(backtrace_size_t i = 1; i < backtraceSize; i++)
            {
                std::string symbol = backtraceSymbols[i];
                std::string demangled;

                // Demangle any C++ symbols in the backtrace.
                auto callback = [&](const std::smatch& match)
                {
                    std::string s;

                    int status = -1;

#if 1 == EXCEPTION_STRIP_MODULE
                    char *szDemangled = abi::__cxa_demangle(
                        match.str(2).c_str(), 0, 0, &status);
#else // 1 != EXCEPTION_STRIP_MODULE
                    char *szDemangled = abi::__cxa_demangle(
                        match.str(1).c_str(), 0, 0, &status);
#endif // 1 == EXCEPTION_STRIP_MODULE

                    if(0 == status)
                    {
                        std::stringstream ss;
#if 1 == EXCEPTION_STRIP_MODULE
                        ss << szDemangled << "+" << match.str(3);
#else // 1 != EXCEPTION_STRIP_MODULE
                        ss << "(" << szDemangled << "+" << match.str(2) << ")";
#endif // 1 == EXCEPTION_STRIP_MODULE
                        s = ss.str();
                    }
                    else
                    {
                        s = match.str(0);
                    }

                    free(szDemangled);

                    return s;
                };

#if 1 == EXCEPTION_STRIP_MODULE
                std::regex re("^(.*)\\((.+)\\+(0x[0-9a-fA-F]+)\\)");
#else // 1 != EXCEPTION_STRIP_MODULE
                std::regex re("\\((.+)\\+(0x[0-9a-fA-F]+)\\)");
#endif // 1 == EXCEPTION_STRIP_MODULE

                demangled = std::regex_replace(symbol.cbegin(), symbol.cend(),
                    re, callback);

                mBacktrace.push_back(demangled);
            }

            // Since backtrace_symbols allocated the array, we must free it.
            // Note that the man page specifies that the strings themselves
            // should not be freed.
            free(backtraceSymbols);
        }
    }
#endif // _WIN32
}

int Exception::Line() const
{
    return mLine;
}

String Exception::File() const
{
    // If the path to the file begins with the project directory,
    // strip the project directory from the path.
    if( mFile.Length() > baseLen && mFile.Left(baseLen) ==
        String(__FILE__).Left(baseLen) )
    {
        return mFile.Mid(baseLen);
    }

    return mFile;
}

String Exception::Message() const
{
    return mMessage;
}

std::list<String> Exception::Backtrace() const
{
    return mBacktrace;
}

void Exception::Log() const
{
    // Basic exception log message shows the file and line number where the
    // exception occured and the message describing the exception.
    LOG_ERROR(String(
        "Exception at %1:%2\n"
        "========================================"
        "========================================\n"
        "%3\n"
        "========================================"
        "========================================\n"
        "%4\n"
        "========================================"
        "========================================\n"
    ).Arg(File()).Arg(Line()).Arg(Message()).Arg(
        String::Join(Backtrace(), "\n")));
}
