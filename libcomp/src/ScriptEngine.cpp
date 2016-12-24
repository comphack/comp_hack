/**
 * @file libcomp/src/ScriptEngine.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage Squirrel scripting.
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

#include "ScriptEngine.h"

#include "Constants.h"
#include "Log.h"

#include <cstdio>
#include <cstdarg>

#include <sqstdaux.h>

using namespace libcomp;
using namespace Sqrat;

const SQInteger ONE_PARAM = 1;
const SQBool    NO_RETURN_VALUE = SQFalse;
const SQBool    RAISE_ERROR = SQTrue;

static void SquirrelPrintFunction(HSQUIRRELVM vm, const SQChar *szFormat, ...)
{
    (void)vm;

    va_list args;

    va_start(args, szFormat);
    int bytesNeeded = vsnprintf(NULL, 0, szFormat, args);
    va_end(args);

    char *szBuffer = new char[bytesNeeded + 1];
    szBuffer[0] = 0;

    va_start(args, szFormat);
    vsnprintf(szBuffer, (size_t)bytesNeeded + 1, szFormat, args);
    va_end(args);

    std::list<String> messages = String(szBuffer).Split("\n");

    for(String msg : messages)
    {
        LOG_INFO(String("SQUIRREL: %1\n").Arg(msg));
    }

    delete[] szBuffer;
}

static void SquirrelErrorFunction(HSQUIRRELVM vm, const SQChar *szFormat, ...)
{
    (void)vm;

    va_list args;

    va_start(args, szFormat);
    int bytesNeeded = vsnprintf(NULL, 0, szFormat, args);
    va_end(args);

    char *szBuffer = new char[bytesNeeded + 1];
    szBuffer[0] = 0;

    va_start(args, szFormat);
    vsnprintf(szBuffer, (size_t)bytesNeeded + 1, szFormat, args);
    va_end(args);

    std::list<String> messages = String(szBuffer).Split("\n");

    for(String msg : messages)
    {
        LOG_ERROR(String("SQUIRREL: %1\n").Arg(msg));
    }

    delete[] szBuffer;
}

ScriptEngine::ScriptEngine()
{
    mVM = sq_open(SQUIRREL_STACK_SIZE);

    sqstd_seterrorhandlers(mVM);
    sq_setcompilererrorhandler(mVM,
        [](HSQUIRRELVM vm, const SQChar *szDescription,
            const SQChar *szSource, SQInteger line, SQInteger column)
        {
            (void)vm;

            LOG_ERROR(String("Failed to compile Squirrel script: "
                "%1:%2:%3:  %4").Arg(szSource).Arg((int64_t)line).Arg(
                (int64_t)column).Arg(szDescription));
        });
    sq_setprintfunc(mVM, &SquirrelPrintFunction, &SquirrelErrorFunction);

    sq_pushroottable(mVM);
    sqstd_register_bloblib(mVM);
}

ScriptEngine::~ScriptEngine()
{
    sq_close(mVM);
}

bool ScriptEngine::Eval(const String& source, const String& sourceName)
{
    bool result = false;

    SQInteger top = sq_gettop(mVM);

    if(SQ_SUCCEEDED(sq_compilebuffer(mVM, source.C(),
        (SQInteger)source.Size(), sourceName.C(), 1)))
    {
        sq_pushroottable(mVM);

        if(SQ_SUCCEEDED(sq_call(mVM, ONE_PARAM,
            NO_RETURN_VALUE, RAISE_ERROR)))
        {
            result = true;
        }
    }

    sq_settop(mVM, top);

    return result;
}

HSQUIRRELVM ScriptEngine::GetVM()
{
    return mVM;
}
