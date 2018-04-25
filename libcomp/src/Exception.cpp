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

#include "Exception.h"

#include "Constants.h"
#include "Log.h"

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#else // _WIN32
#include <regex_ext>
#include <execinfo.h>
#include <cxxabi.h>
#endif // _WIN32

#include <climits>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <sstream>

#include <signal.h>

using namespace libcomp;

/// If the module name should be stripped from the backtrace.
#define EXCEPTION_STRIP_MODULE (0)

#if __FreeBSD__
typedef size_t  backtrace_size_t;
#elif __linux__
typedef int     backtrace_size_t;
#endif

#ifdef _WIN32
#define MAX_SYMBOL_LEN (1024)
#endif // _WIN32

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
    // Array to store each backtrace address.
    void *backtraceAddresses[USHRT_MAX];

    USHORT frameCount = CaptureStackBackTrace(0, USHRT_MAX,
        backtraceAddresses, NULL);

    SymSetOptions(SYMOPT_LOAD_LINES);

    BOOL symInit = SymInitialize(GetCurrentProcess(), NULL, TRUE);

    if(TRUE != symInit)
    {
        LOG_CRITICAL("Failed to load symbols!\n");
    }

    for(USHORT i = 0; i < frameCount; ++i)
    {
        DWORD64 displacement = 0;

        uint8_t *pInfoBuffer = new uint8_t[sizeof(SYMBOL_INFO) +
            MAX_SYMBOL_LEN];
        SYMBOL_INFO *pInfo = reinterpret_cast<SYMBOL_INFO*>(pInfoBuffer);
        memset(pInfoBuffer, 0, sizeof(SYMBOL_INFO) + MAX_SYMBOL_LEN);
        pInfo->MaxNameLen = MAX_SYMBOL_LEN;
        pInfo->SizeOfStruct = sizeof(SYMBOL_INFO);

        // Retrieve the symbol for each frame in the array.
        if(TRUE == symInit && TRUE == SymFromAddr(GetCurrentProcess(),
            (DWORD64)backtraceAddresses[i], &displacement, pInfo))
        {
            char name[MAX_SYMBOL_LEN];
            char sym[MAX_SYMBOL_LEN];

            std::stringstream ss;

            if(0 >= UnDecorateSymbolName(pInfo->Name, sym,
                MAX_SYMBOL_LEN, UNDNAME_COMPLETE))
            {
                strncpy(sym, pInfo->Name, MAX_SYMBOL_LEN - 1);
            }

            if(0 < GetModuleFileNameA((HMODULE)pInfo->ModBase, name,
                MAX_SYMBOL_LEN))
            {
                char *pModuleName = strrchr(name, '\\');

                if(NULL == pModuleName)
                {
                    pModuleName = name;
                }
                else
                {
                    pModuleName++;
                }

                ss << pModuleName << "(" << sym << "+0x"
                    << std::hex << displacement << ")";
            }
            else
            {
                ss << R"_raw_(???()_raw_" << sym << "+0x"
                    << std::hex << displacement << ")";
            }

            ss << " [0x" << std::hex << (DWORD64)backtraceAddresses[i] << "]";

            DWORD lineDisplacement;
            IMAGEHLP_LINE64 line;

            if(TRUE == SymGetLineFromAddr64(GetCurrentProcess(),
                (ULONG64)backtraceAddresses[i], &lineDisplacement, &line))
            {
                ss << " " << line.FileName << ":"
                    << std::dec << line.LineNumber;
            }

            mBacktrace.push_back(ss.str());
        }
        else
        {
            std::stringstream ss;
            ss << "0x" << std::hex << (DWORD64)backtraceAddresses[i];

            mBacktrace.push_back(ss.str());
        }

        delete[] pInfoBuffer;
        pInfoBuffer = nullptr;
    }
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

#ifdef _WIN32
LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
    // Source: https://stackoverflow.com/questions/28099965/

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    // StackWalk64() may modify context record passed to it, so we will
    // use a copy.
    CONTEXT context_record = *pExceptionInfo->ContextRecord;

    // Initialize stack walking.
    STACKFRAME64 stack_frame;
    memset(&stack_frame, 0, sizeof(stack_frame));

#if defined(_WIN64)
    int machine_type = IMAGE_FILE_MACHINE_AMD64;
    stack_frame.AddrPC.Offset = context_record.Rip;
    stack_frame.AddrFrame.Offset = context_record.Rbp;
    stack_frame.AddrStack.Offset = context_record.Rsp;
#else
    int machine_type = IMAGE_FILE_MACHINE_I386;
    stack_frame.AddrPC.Offset = context_record.Eip;
    stack_frame.AddrFrame.Offset = context_record.Ebp;
    stack_frame.AddrStack.Offset = context_record.Esp;
#endif

    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    SYMBOL_INFO *symbol = (SYMBOL_INFO*)new uint8_t[
        sizeof(SYMBOL_INFO) + 256 + 1];
    memset(symbol, 0, sizeof(SYMBOL_INFO) + 256);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    std::stringstream ss;
    while(StackWalk64(machine_type,
        GetCurrentProcess(),
        GetCurrentThread(),
        &stack_frame,
        &context_record,
        NULL,
        &SymFunctionTableAccess64,
        &SymGetModuleBase64,
        NULL)) {

        DWORD64 displacement = 0;

        auto address = (DWORD64)stack_frame.AddrPC.Offset;
        if(SymFromAddr(process, address, &displacement, symbol))
        {
            IMAGEHLP_MODULE64 moduleInfo;
            memset(&moduleInfo, 0, sizeof(moduleInfo));
            moduleInfo.SizeOfStruct = sizeof(moduleInfo);

            if(SymGetModuleInfo64(process, symbol->ModBase, &moduleInfo))
                ss << moduleInfo.ModuleName;

            ss << "(" << symbol->Name << "+0x" << std::hex <<
                (int64_t)displacement << ")";
            
            DWORD lineDisplacement;
            IMAGEHLP_LINE64 line;

            if(TRUE == SymGetLineFromAddr64(GetCurrentProcess(),
                (ULONG64)address, &lineDisplacement, &line))
            {
                ss << " " << line.FileName << ":"
                    << std::dec << line.LineNumber;
            }

            ss << std::endl;
        }
    }

    delete symbol;

    LOG_CRITICAL("The server has crashed with an unhandled exception. A"
        " backtrace will follow.\n");
    LOG_CRITICAL(libcomp::String("Backtrace: %1\n").Arg(ss.str()));

    return EXCEPTION_CONTINUE_SEARCH;
}
#endif  // _WIN32

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

static void SignalHandler(int sig)
{
    (void)sig;

    Exception e("SIGSEGV", __FILE__, __LINE__);

    LOG_CRITICAL("The server has crashed. A backtrace will follow.\n");

    for(libcomp::String s : e.Backtrace())
    {
        LOG_CRITICAL(libcomp::String("Backtrace: %1\n").Arg(s));
    }

    exit(EXIT_FAILURE);
}

void Exception::RegisterSignalHandler()
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(TopLevelExceptionHandler);
#else
    signal(SIGSEGV, SignalHandler);
#endif

    std::set_terminate([]() {
        Exception e("Unhandled Exception", __FILE__, __LINE__);

        LOG_CRITICAL("The server has crashed. A backtrace will follow.\n");

        for(libcomp::String s : e.Backtrace())
        {
            LOG_CRITICAL(libcomp::String("Backtrace: %1\n").Arg(s));
        }

        exit(EXIT_FAILURE);
    });
}
