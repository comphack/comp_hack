/**
 * @file libcomp/src/WindowsService.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to expose the server as a Windows service.
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

#include "WindowsService.h"

#if defined(_WIN32) && defined(WIN32_SERV)

// libcomp Includes
#include "Shutdown.h"

using namespace libcomp;

namespace libcomp
{

char *SERVICE_NAME = "COMP_hack Server";

WindowsService *gService = nullptr;

} // namespace libcomp

int ServiceMain(int argc, const char *argv[]);
VOID WINAPI ServiceCtrlHandler(DWORD);

static int ServiceMain(int argc, const char *argv[])
{
    return gService->Run(argc, argv);
}

static VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    gService->HandleCtrlCode(CtrlCode);
}

WindowsService::WindowsService(const std::function<int(int,
    char*[])>& func) : mStatus({0}), mStatusHandle(NULL), mMain(func)
{
}

int WindowsService::Run(int argc, const char *argv[])
{
    int exitCode = -1;

    DWORD Status = E_FAIL;

    // Register our service control handler with the SCM.
    mStatusHandle = RegisterServiceCtrlHandler(
        SERVICE_NAME, ServiceCtrlHandler);

    if(NULL == mStatusHandle)
    {
        return -1;
    }

    // Tell the service controller we are starting.
    ZeroMemory(&mStatus, sizeof(mStatus));
    mStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    mStatus.dwControlsAccepted = 0;
    mStatus.dwCurrentState = SERVICE_START_PENDING;
    mStatus.dwWin32ExitCode = 0;
    mStatus.dwServiceSpecificExitCode = 0;
    mStatus.dwCheckPoint = 0;

    if(!SetServiceStatus(mStatusHandle , &mStatus))
    {
        OutputDebugStringA("SetServiceStatus returned error");
    }

    char cwd[1024];
    char *cwd_end;

    if(0 < GetModuleFileNameA(NULL, cwd, sizeof(cwd)) &&
        NULL != (cwd_end = strrchr(cwd, '\\')))
    {
        *cwd_end = '\0';

        (void)SetCurrentDirectoryA(cwd);
    }

    exitCode = mMain(argc, argv);

    // Tell the service controller we are stopped.
    mStatus.dwControlsAccepted = 0;
    mStatus.dwCurrentState = SERVICE_STOPPED;
    mStatus.dwWin32ExitCode = 0;
    mStatus.dwCheckPoint = 3;

    if(!SetServiceStatus(mStatusHandle, &mStatus))
    {
        OutputDebugStringA("SetServiceStatus returned error");
    }

    return exitCode;
}

void WindowsService::Started()
{
    // Tell the service controller we are started.
    mStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    mStatus.dwCurrentState = SERVICE_RUNNING;
    mStatus.dwWin32ExitCode = 0;
    mStatus.dwCheckPoint = 0;

    if(!SetServiceStatus(mStatusHandle, &mStatus))
    {
        OutputDebugStringA("SetServiceStatus returned error");
    }
}

void WindowsService::HandleCtrlCode(DWORD CtrlCode)
{
    switch(CtrlCode)
    {
        case SERVICE_CONTROL_STOP:
        {
            if(mStatus.dwCurrentState != SERVICE_RUNNING)
            {
                break;
            }

            mStatus.dwControlsAccepted = 0;
            mStatus.dwCurrentState = SERVICE_STOP_PENDING;
            mStatus.dwWin32ExitCode = 0;
            mStatus.dwCheckPoint = 4;

            if(!SetServiceStatus(mStatusHandle, &mStatus) == FALSE)
            {
                OutputDebugStringA("SetServiceStatus returned error");
            }

            // This will signal the server to start shutting down.
            ShutdownSignalHandler(0);
            break;
        }
        default:
            break;
    }
}

#endif // defined(_WIN32) && defined(WIN32_SERV)
