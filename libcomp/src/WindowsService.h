/**
 * @file libcomp/src/WindowsService.h
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

#ifndef LIBCOMP_SRC_WINDOWSSERVICE_H
#define LIBCOMP_SRC_WINDOWSSERVICE_H

#if defined(_WIN32) && defined(WIN32_SERV)

// Windows Includes
#include <windows.h>

// Standard C++11 Includes
#include <functional>

namespace libcomp
{

extern char *SERVICE_NAME;

class WindowsService
{
public:
    WindowsService(const std::function<int(int, const char**)>& func);

    int Run(int argc, const char *argv[]);
    void HandleCtrlCode(DWORD CtrlCode);
    void Started();

private:
    SERVICE_STATUS mStatus = {0};
    SERVICE_STATUS_HANDLE mStatusHandle;

    std::function<int(int, const char**)> mMain;
};

extern WindowsService *gService;

} // namespace libcomp

#endif // defined(_WIN32) && defined(WIN32_SERV)

#endif // LIBCOMP_SRC_WINDOWSSERVICE_H
