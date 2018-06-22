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

#include "ScriptEngine.h"

// libcomp Includes
#include "BaseServer.h"
#include "Constants.h"
#include "Database.h"
#include "Decrypt.h"
#include "DefinitionManager.h"
#include "Log.h"
#include "ServerDataManager.h"

// objects Includes
#include <Account.h>
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <BazaarItem.h>
#include <Character.h>
#include <Demon.h>
#include <RegisteredChannel.h>
#include <RegisteredWorld.h>

#include <cstdio>
#include <cstdarg>

#include <sqstdaux.h>

using namespace libcomp;
using namespace Sqrat;

const SQInteger ONE_PARAM = 1;
const SQBool    NO_RETURN_VALUE = SQFalse;
const SQBool    RAISE_ERROR = SQTrue;

std::unordered_map<std::string, std::function<bool(ScriptEngine&,
    const std::string& module)>> ScriptEngine::mModules;

static std::shared_ptr<objects::Account> ToAccount(
    const std::shared_ptr<libcomp::PersistentObject>& obj)
{
    return std::dynamic_pointer_cast<objects::Account>(obj);
}

static std::shared_ptr<objects::AccountWorldData> ToAccountWorldData(
    const std::shared_ptr<libcomp::PersistentObject>& obj)
{
    return std::dynamic_pointer_cast<objects::AccountWorldData>(obj);
}

static std::shared_ptr<objects::Character> ToCharacter(const std::shared_ptr<
    libcomp::PersistentObject>& obj)
{
    return std::dynamic_pointer_cast<objects::Character>(obj);
}

static std::shared_ptr<objects::Demon> ToDemon(const std::shared_ptr<
    libcomp::PersistentObject>& obj)
{
    return std::dynamic_pointer_cast<objects::Demon>(obj);
}

static bool ScriptInclude(HSQUIRRELVM vm, const char *szPath)
{
    return ScriptEngine::Self(vm)->Include(szPath);
}

static bool ScriptImport(HSQUIRRELVM vm, const char *szModule)
{
    return ScriptEngine::Self(vm)->Import(szModule);
}

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

static void SquirrelPrintFunctionRaw(HSQUIRRELVM vm,
    const SQChar *szFormat, ...)
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

    LOG_INFO(szBuffer);

    delete[] szBuffer;
}

static void SquirrelErrorFunctionRaw(HSQUIRRELVM vm,
    const SQChar *szFormat, ...)
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
        LOG_ERROR(msg + "\n");
    }

    delete[] szBuffer;
}

ScriptEngine::ScriptEngine(bool useRawPrint) : mUseRawPrint(useRawPrint)
{
    if(mModules.empty())
    {
        InitializeBuiltins();
    }

    mVM = sq_open(SQUIRREL_STACK_SIZE);

    sq_setforeignptr(mVM, this);
    sqstd_seterrorhandlers(mVM);
    sq_setcompilererrorhandler(mVM,
        [](HSQUIRRELVM vm, const SQChar *szDescription,
            const SQChar *szSource, SQInteger line, SQInteger column)
        {
            (void)vm;

            LOG_ERROR(String("Failed to compile Squirrel script: "
                "%1:%2:%3:  %4\n").Arg(szSource).Arg((int64_t)line).Arg(
                (int64_t)column).Arg(szDescription));
        });
    if(useRawPrint)
    {
        sq_setprintfunc(mVM, &SquirrelPrintFunctionRaw,
            &SquirrelErrorFunctionRaw);
    }
    else
    {
        sq_setprintfunc(mVM, &SquirrelPrintFunction,
            &SquirrelErrorFunction);
    }

    sq_pushroottable(mVM);
    sqstd_register_bloblib(mVM);

    Sqrat::RootTable(mVM).VMFunc("include", ScriptInclude);
    Sqrat::RootTable(mVM).VMFunc("import", ScriptImport);

    // Bind some root level object conversions
    Sqrat::RootTable(mVM).Func("ToAccount", ToAccount);
    Sqrat::RootTable(mVM).Func("ToAccountWorldData", ToAccountWorldData);
    Sqrat::RootTable(mVM).Func("ToCharacter", ToCharacter);
    Sqrat::RootTable(mVM).Func("ToDemon", ToDemon);
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

std::shared_ptr<ScriptEngine> ScriptEngine::Self()
{
    return shared_from_this();
}

std::shared_ptr<const ScriptEngine> ScriptEngine::Self() const
{
    return shared_from_this();
}

std::shared_ptr<ScriptEngine> ScriptEngine::Self(HSQUIRRELVM vm)
{
    ScriptEngine *pScriptEngine = (ScriptEngine*)sq_getforeignptr(vm);

    if(pScriptEngine)
    {
        return pScriptEngine->Self();
    }

    return {};
}

bool ScriptEngine::BindingExists(const std::string& name, bool lockBinding)
{
    bool result = mBindings.find(name) != mBindings.end();
    if(!result && lockBinding)
    {
        mBindings.insert(name);
    }

    return result;
}

bool ScriptEngine::Include(const std::string& path)
{
    std::vector<char> file = libcomp::Decrypt::LoadFile(path);
    LOG_INFO(libcomp::String("Include: %1\n").Arg(path));
    if(file.empty())
    {
        auto msg = libcomp::String("Failed to include script file: "
            "%1\n").Arg(path);

        if(mUseRawPrint)
        {
            printf("%s", msg.C());
        }
        else
        {
            LOG_ERROR(msg);
        }

        return false;
    }

    file.push_back(0);

    if(!Eval(&file[0], path))
    {
        auto msg = libcomp::String("Failed to run script file: "
            "%1\n").Arg(path);

        if(mUseRawPrint)
        {
            printf("%s", msg.C());
        }
        else
        {
            LOG_ERROR(msg);
        }

        return false;
    }

    return true;
}

bool ScriptEngine::Import(const std::string& module)
{
    bool result = mImports.find(module) != mImports.end();

    if(result)
    {
        LOG_WARNING(libcomp::String("Module has already been imported: "
            "%s\n").Arg(module));

        return false;
    }

    auto it = mModules.find(module);

    if(mModules.end() == it)
    {
        LOG_ERROR(libcomp::String("Failed to import script module: "
            "%1\n").Arg(module));

        return false;
    }

    result = (it->second)(*this, module);

    if(result)
    {
        mImports.insert(module);
    }

    return result;
}

void ScriptEngine::RegisterModule(const std::string& module,
    const std::function<bool(ScriptEngine&, const std::string& module)>& func)
{
    mModules[module] = func;
}

void ScriptEngine::InitializeBuiltins()
{
    RegisterModule("database", [](ScriptEngine& engine,
        const std::string& module) -> bool
    {
        (void)module;

        engine.Using<Database>();

        // Now register the common objects you might want to access
        // from the database.
        engine.Using<objects::Account>();
        engine.Using<objects::AccountWorldData>();
        engine.Using<objects::BazaarData>();
        engine.Using<objects::BazaarItem>();
        engine.Using<objects::Character>();
        engine.Using<objects::Demon>();
        engine.Using<objects::RegisteredChannel>();
        engine.Using<objects::RegisteredWorld>();

        return true;
    });

    RegisterModule("server", [](ScriptEngine& engine,
        const std::string& module) -> bool
    {
        (void)module;

        engine.Using<BaseServer>();

        // Now register the common objects you might want to access
        // from the server.
        engine.Using<DefinitionManager>();
        engine.Using<ServerDataManager>();

        return true;
    });
}
