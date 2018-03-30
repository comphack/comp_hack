/**
 * @file libcomp/src/Child.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to wrap and manage a child process.
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

#include "Child.h"

// Linux Includes
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// Standard C Includes
#include <cstring>

// Standard C++ Includes
#include <sstream>

using namespace libcomp;

static pid_t CreateProcess(const char *szProgram,
    char * const szArguments[], bool redirectOutput = true)
{
    pid_t pid = fork();

    if(0 == pid)
    {
        if(redirectOutput)
        {
            dup2(open("/dev/null", O_WRONLY), STDOUT_FILENO);
            dup2(open("/dev/null", O_WRONLY), STDERR_FILENO);
        }

        (void)execv(szProgram, szArguments);

        pid = 0;
    }

    return pid;
}

static pid_t CreateProcess(const std::string& program,
    const std::list<std::string>& arguments, bool redirectOutput = true)
{
    char **szArguments = new char*[arguments.size() + 2];

    szArguments[0] = strdup(program.c_str());

    int i = 1;

    for(auto arg : arguments)
    {
        szArguments[i++] = strdup(arg.c_str());
    }

    szArguments[i] = 0;

    pid_t pid = CreateProcess(szArguments[0], szArguments, redirectOutput);

    delete[] szArguments;

    return pid;
}

Child::Child(const std::string& program,
    const std::list<std::string>& arguments,
    int bootTimeout, bool restart, bool displayOutput) :
    mProgram(program), mArguments(arguments), mPID(0),
    mBootTimeout(bootTimeout), mRestart(restart),
    mDisplayOutput(displayOutput)
{
}

Child::~Child()
{
    if(0 != mPID)
    {
        int status;
        Kill();
        waitpid(mPID, &status, 0);
    }
}

void Child::Kill()
{
    if(0 != mPID)
    {
        kill(mPID, SIGTERM);
    }
}

void Child::Interrupt()
{
    if(0 != mPID)
    {
        kill(mPID, SIGINT);
    }
}

bool Child::Start(bool notify)
{
    if(0 != mPID)
    {
        printf("Restarting %d: %s\n", mPID, GetCommandLine().c_str());

        int status;
        Kill();
        waitpid(mPID, &status, 0);
    }

    std::list<std::string> arguments = mArguments;

    if(notify)
    {
        std::stringstream ss;
        ss << "--notify=" << (int)getpid();

        arguments.push_front(ss.str());
    }

    mPID = CreateProcess(mProgram, arguments, !mDisplayOutput);

    return 0 != mPID;
}

std::string Child::GetCommandLine() const
{
    std::stringstream ss;
    ss << mProgram;

    for(auto arg : mArguments)
    {
        ss << " " << arg;
    }

    return ss.str();
}

pid_t Child::GetPID() const
{
    return mPID;
}

bool Child::ShouldRestart() const
{
    return mRestart;
}

int Child::GetBootTimeout() const
{
    return mBootTimeout;
}
