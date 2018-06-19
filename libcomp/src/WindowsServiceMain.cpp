/**
 * @file libcomp/src/WindowsServiceLib.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Global for the service to be used by libcomp.
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

using namespace libcomp;

namespace libcomp
{

extern char *SERVICE_NAME;

} // namespace libcomp

int ServiceMain(int argc, const char *argv[]);
int ApplicationMain(int argc, const char *argv[]);;

int main(int argc, const char *argv[])
{
    gService = new WindowsService(&ApplicationMain);

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {
            SERVICE_NAME,
            (LPSERVICE_MAIN_FUNCTION)ServiceMain
        },
        {
            NULL,
            NULL
        }
    };

    if(!StartServiceCtrlDispatcher(ServiceTable))
    {
        return GetLastError();
    }

    return 0;
}

#endif // defined(_WIN32) && defined(WIN32_SERV)
