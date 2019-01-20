/**
 * @file libcomp/src/SqratInt64.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Squirrel binding for 64-bit integers.
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

#include "SqratInt64.h"

// libcomp Includes
#include "ScriptEngine.h"

Sqrat::s64::s64()
{
    mValue = 0;
}

Sqrat::s64::s64(int32_t value)
{
    mValue = value;
}

Sqrat::s64::s64(const s64& other)
{
    mValue = other.mValue;
}

SQInteger Sqrat::s64::_cmp(HSQUIRRELVM vm)
{
    Sqrat::Var<s64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<s64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (s64 or integer expected)");
        }

        if(arg1.value->mValue == (int64_t)arg2b.value)
        {
            Sqrat::PushVar(vm, 0);
        }
        else if(arg1.value->mValue > (int64_t)arg2b.value)
        {
            Sqrat::PushVar(vm, 1);
        }
        else
        {
            Sqrat::PushVar(vm, -1);
        }

        return 1;
    }

    if(arg1.value->mValue == arg2.value->mValue)
    {
        Sqrat::PushVar(vm, 0);
    }
    else if(arg1.value->mValue > arg2.value->mValue)
    {
        Sqrat::PushVar(vm, 1);
    }
    else
    {
        Sqrat::PushVar(vm, -1);
    }

    return 1;
}

SQInteger Sqrat::s64::_add(HSQUIRRELVM vm)
{
    Sqrat::Var<s64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<s64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (s64 or integer expected)");
        }

        s64 result;
        result.mValue = arg1.value->mValue + (int64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    s64 result;
    result.mValue = arg1.value->mValue + arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::s64::_sub(HSQUIRRELVM vm)
{
    Sqrat::Var<s64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<s64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (s64 or integer expected)");
        }

        s64 result;
        result.mValue = arg1.value->mValue - (int64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    s64 result;
    result.mValue = arg1.value->mValue - arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::s64::_mul(HSQUIRRELVM vm)
{
    Sqrat::Var<s64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<s64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (s64 or integer expected)");
        }

        s64 result;
        result.mValue = arg1.value->mValue * (int64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    s64 result;
    result.mValue = arg1.value->mValue * arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::s64::_div(HSQUIRRELVM vm)
{
    Sqrat::Var<s64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<s64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (s64 or integer expected)");
        }

        if(0 == arg2b.value)
        {
            return sq_throwerror(vm, "divide by zero");
        }

        s64 result;
        result.mValue = arg1.value->mValue / (int64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    if(0 == arg2.value->mValue)
    {
        return sq_throwerror(vm, "divide by zero");
    }

    s64 result;
    result.mValue = arg1.value->mValue / arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::s64::_mod(HSQUIRRELVM vm)
{
    Sqrat::Var<s64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<s64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (s64 or integer expected)");
        }

        if(0 == arg2b.value)
        {
            return sq_throwerror(vm, "divide by zero");
        }

        s64 result;
        result.mValue = arg1.value->mValue % (int64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    if(0 == arg2.value->mValue)
    {
        return sq_throwerror(vm, "divide by zero");
    }

    s64 result;
    result.mValue = arg1.value->mValue % arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

libcomp::String Sqrat::s64::_tostring() const
{
    return libcomp::String("%1").Arg(mValue);
}

void Sqrat::s64::set(int64_t value)
{
    mValue = value;
}

int64_t Sqrat::s64::value() const
{
    return mValue;
}

int32_t Sqrat::s64::valueTruncated() const
{
    return (int32_t)mValue;
}

SQInteger Sqrat::s64::equal(HSQUIRRELVM vm)
{
    Sqrat::Var<s64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<s64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (s64 or integer expected)");
        }

        bool result = arg1.value->mValue == (int64_t)arg2b.value;
        Sqrat::PushVar(vm, result);

        return 1;
    }

    bool result = arg1.value->mValue == arg2.value->mValue;
    Sqrat::PushVar(vm, result);

    return 1;
}

Sqrat::u64::u64()
{
    mValue = 0;
}

Sqrat::u64::u64(int32_t value)
{
    mValue = (uint64_t)value;
}

Sqrat::u64::u64(const u64& other)
{
    mValue = other.mValue;
}

SQInteger Sqrat::u64::_cmp(HSQUIRRELVM vm)
{
    Sqrat::Var<u64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<u64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (u64 or integer expected)");
        }

        if(arg1.value->mValue == (uint64_t)arg2b.value)
        {
            Sqrat::PushVar(vm, 0);
        }
        else if(arg1.value->mValue > (uint64_t)arg2b.value)
        {
            Sqrat::PushVar(vm, 1);
        }
        else
        {
            Sqrat::PushVar(vm, -1);
        }

        return 1;
    }

    if(arg1.value->mValue == arg2.value->mValue)
    {
        Sqrat::PushVar(vm, 0);
    }
    else if(arg1.value->mValue > arg2.value->mValue)
    {
        Sqrat::PushVar(vm, 1);
    }
    else
    {
        Sqrat::PushVar(vm, -1);
    }

    return 1;
}

SQInteger Sqrat::u64::_add(HSQUIRRELVM vm)
{
    Sqrat::Var<u64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<u64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (u64 or integer expected)");
        }

        u64 result;
        result.mValue = arg1.value->mValue + (uint64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    u64 result;
    result.mValue = arg1.value->mValue + arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::u64::_sub(HSQUIRRELVM vm)
{
    Sqrat::Var<u64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<u64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (u64 or integer expected)");
        }

        u64 result;
        result.mValue = arg1.value->mValue - (uint64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    u64 result;
    result.mValue = arg1.value->mValue - arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::u64::_mul(HSQUIRRELVM vm)
{
    Sqrat::Var<u64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<u64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (u64 or integer expected)");
        }

        u64 result;
        result.mValue = arg1.value->mValue * (uint64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    u64 result;
    result.mValue = arg1.value->mValue * arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::u64::_div(HSQUIRRELVM vm)
{
    Sqrat::Var<u64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<u64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (u64 or integer expected)");
        }

        if(0 == arg2b.value)
        {
            return sq_throwerror(vm, "divide by zero");
        }

        u64 result;
        result.mValue = arg1.value->mValue / (uint64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    if(0 == arg2.value->mValue)
    {
        return sq_throwerror(vm, "divide by zero");
    }

    u64 result;
    result.mValue = arg1.value->mValue / arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

SQInteger Sqrat::u64::_mod(HSQUIRRELVM vm)
{
    Sqrat::Var<u64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<u64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (u64 or integer expected)");
        }

        if(0 == arg2b.value)
        {
            return sq_throwerror(vm, "divide by zero");
        }

        u64 result;
        result.mValue = arg1.value->mValue % (uint64_t)arg2b.value;

        Sqrat::PushVar(vm, result);

        return 1;
    }

    if(0 == arg2.value->mValue)
    {
        return sq_throwerror(vm, "divide by zero");
    }

    u64 result;
    result.mValue = arg1.value->mValue % arg2.value->mValue;

    Sqrat::PushVar(vm, result);

    return 1;
}

libcomp::String Sqrat::u64::_tostring() const
{
    return libcomp::String("%1").Arg(mValue);
}

void Sqrat::u64::set(uint64_t value)
{
    mValue = value;
}

uint64_t Sqrat::u64::value() const
{
    return mValue;
}

int32_t Sqrat::u64::valueTruncated() const
{
    return (int32_t)mValue;
}

SQInteger Sqrat::u64::equal(HSQUIRRELVM vm)
{
    Sqrat::Var<u64*> arg1(vm, 1);

    if(Sqrat::Error::Occurred(vm))
    {
        return sq_throwerror(vm, Sqrat::Error::Message(vm).c_str());
    }

    Sqrat::Var<u64*> arg2(vm, 2);

    if(Sqrat::Error::Occurred(vm))
    {
        Sqrat::Error::Clear(vm);
        Sqrat::Var<int32_t> arg2b(vm, 2);

        if(Sqrat::Error::Occurred(vm))
        {
            return sq_throwerror(vm, "wrong type (u64 or integer expected)");
        }

        bool result = arg1.value->mValue == (uint64_t)arg2b.value;
        Sqrat::PushVar(vm, result);

        return 1;
    }

    bool result = arg1.value->mValue == arg2.value->mValue;
    Sqrat::PushVar(vm, result);

    return 1;
}

static Sqrat::s64 s64_cast(int32_t value)
{
    return Sqrat::s64(value);
}

static Sqrat::u64 u64_cast(int32_t value)
{
    return Sqrat::u64(value);
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<Sqrat::s64>()
    {
        if(!BindingExists("s64", true))
        {
            Sqrat::Class<Sqrat::s64> binding(mVM, "s64");
            binding.SquirrelFunc("_cmp", &Sqrat::s64::_cmp);
            binding.SquirrelFunc("_add", &Sqrat::s64::_add);
            binding.SquirrelFunc("_sub", &Sqrat::s64::_sub);
            binding.SquirrelFunc("_mul", &Sqrat::s64::_mul);
            binding.SquirrelFunc("_div", &Sqrat::s64::_div);
            binding.SquirrelFunc("_mod", &Sqrat::s64::_mod);
            binding.SquirrelFunc("equal", &Sqrat::s64::equal);
            binding.Func("_tostring", &Sqrat::s64::_tostring);
            binding.Func("value", &Sqrat::s64::valueTruncated);
            Bind<Sqrat::s64>("s64", binding);

            Sqrat::RootTable(mVM).Func("s64", s64_cast);
        }

        return *this;
    }

    template<>
    ScriptEngine& ScriptEngine::Using<Sqrat::u64>()
    {
        if(!BindingExists("u64", true))
        {
            Sqrat::Class<Sqrat::u64> binding(mVM, "u64");
            binding.SquirrelFunc("_cmp", &Sqrat::u64::_cmp);
            binding.SquirrelFunc("_add", &Sqrat::u64::_add);
            binding.SquirrelFunc("_sub", &Sqrat::u64::_sub);
            binding.SquirrelFunc("_mul", &Sqrat::u64::_mul);
            binding.SquirrelFunc("_div", &Sqrat::u64::_div);
            binding.SquirrelFunc("_mod", &Sqrat::u64::_mod);
            binding.SquirrelFunc("equal", &Sqrat::u64::equal);
            binding.Func("_tostring", &Sqrat::u64::_tostring);
            binding.Func("value", &Sqrat::u64::valueTruncated);
            Bind<Sqrat::u64>("u64", binding);

            Sqrat::RootTable(mVM).Func("u64", u64_cast);
        }

        return *this;
    }
}
