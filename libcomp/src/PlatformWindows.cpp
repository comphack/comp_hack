/**
 * @file libcomp/src/PlatformWindows.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Windows specific utility functions.
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

#include "PlatformWindows.h"

#ifdef _WIN32
#include <windows.h>

libcomp::String libcomp::Platform::GetLastErrorString()
{
    libcomp::String error; // String to return.
    LPTSTR errorText = NULL; // Temporary Windows string.
    DWORD errorCode = GetLastError(); // Error code of the last error.

    // Format the error code into a string.
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorText, 0, NULL);

    if(errorText)
    {
        // Convert the Windows string to a QString.
#ifdef UNICODE
        error = libcomp::String::FromUtf16(errorText);
#else
        error = libcomp::String(errorText);
#endif // UNICODE

        // Free the Windows string.
        LocalFree(errorText);
    }
    else
    {
        // The error message didn't format right so just return a string
        // with the error code.
        error = libcomp::String("0x%1").Arg((uint32_t)errorCode, 8, 16, '0');
    }

    return error;
}
#endif // _WIN32
