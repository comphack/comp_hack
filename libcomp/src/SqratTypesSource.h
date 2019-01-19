/**
 * @file libcomp/src/SqratTypesSource.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Extra Squirrel binding types.
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

#ifndef LIBCOMP_SRC_SQRATTYPESSOURCE_H
#define LIBCOMP_SRC_SQRATTYPESSOURCE_H

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push libcomp::String to and from the stack
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<libcomp::String> {

    libcomp::String value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as a libcomp::String
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM vm, SQInteger idx) {
        const SQChar* ret;
        sq_tostring(vm, idx);
        sq_getstring(vm, -1, &ret);
        value = libcomp::String(static_cast<const char*>(ret),
            static_cast<size_t>(sq_getsize(vm, -1)));
        sq_pop(vm,1);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put a string on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM vm, const libcomp::String& value) {
        sq_pushstring(vm, static_cast<const SQChar*>(value.C()),
            static_cast<SQInteger>(value.Size()));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push const libcomp::String references to and from the stack as copies (strings are always copied)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<const libcomp::String&> {

    libcomp::String value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as a string
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM vm, SQInteger idx) {
        const SQChar* ret;
        sq_tostring(vm, idx);
        sq_getstring(vm, -1, &ret);
        value = libcomp::String(static_cast<const char*>(ret),
            static_cast<size_t>(sq_getsize(vm, -1)));
        sq_pop(vm,1);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put a string on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM vm, const libcomp::String& value) {
        sq_pushstring(vm, static_cast<const SQChar*>(value.C()),
            static_cast<SQInteger>(value.Size()));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push std::list<T> to and from the stack
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
struct Var< std::list<std::shared_ptr<T>> > {

    std::list<std::shared_ptr<T>> value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as a list<T>
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM vm, SQInteger idx) {
        SQObjectType value_type = sq_gettype(vm, idx);

        if(OT_ARRAY != value_type)
        {
            SQTHROW(vm, FormatTypeError(vm, idx, _SC("array")));
        }

        sq_push(vm, idx);
        sq_pushnull(vm);

        while(SQ_SUCCEEDED(sq_next(vm, -2)))
        {
            ObjectReference<T> *ref = NULL;
            T* ptr = ClassType<T>::GetInstance(vm, -1, false, &ref);
            SQCATCH_NOEXCEPT(vm) {
                return;
            }

            if(ref) {
                value.push_back(ref->Promote());
            } else {
                value.push_back(SharedPtr<T>());
            }

            sq_pop(vm, 2);
        }

        sq_pop(vm, 2);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put a list<T> on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM vm, const std::list<std::shared_ptr<T>>& value) {
        SQInteger i = 0;

        sq_newarray(vm, value.size());

        for(auto v : value)
        {
            sq_pushinteger(vm, i++);

            if (ClassType<T>::hasClassData(vm)) {
                ClassType<T>::PushSharedInstance(vm, v);
            } else {
                PushVarR(vm, *v);
            }

            sq_set(vm, -3);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push const std::list<T> references to and from the stack
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
struct Var<const std::list<std::shared_ptr<T>>&> {

    std::list<std::shared_ptr<T>> value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as a list<T>
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM vm, SQInteger idx) {
        SQObjectType value_type = sq_gettype(vm, idx);

        if(OT_ARRAY != value_type)
        {
            SQTHROW(vm, FormatTypeError(vm, idx, _SC("array")));
        }

        sq_push(vm, idx);
        sq_pushnull(vm);

        while(SQ_SUCCEEDED(sq_next(vm, -2)))
        {
            ObjectReference<T> *ref = NULL;
            T* ptr = ClassType<T>::GetInstance(vm, -1, false, &ref);
            SQCATCH_NOEXCEPT(vm) {
                return;
            }

            if(ref) {
                value.push_back(ref->Promote());
            } else {
                value.push_back(SharedPtr<T>());
            }

            sq_pop(vm, 2);
        }

        sq_pop(vm, 2);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put a list<T> on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM vm, const std::list<std::shared_ptr<T>>& value) {
        SQInteger i = 0;

        sq_newarray(vm, value.size());

        for(auto v : value)
        {
            sq_pushinteger(vm, i++);

            if (ClassType<T>::hasClassData(vm)) {
                ClassType<T>::PushSharedInstance(vm, v);
            } else {
                PushVarR(vm, *v);
            }

            sq_set(vm, -3);
        }
    }
};

#ifdef SQRAT_WRAP_INTEGER64
template<>
struct Var<int64_t>
{
    int64_t value;

    Var(HSQUIRRELVM vm, SQInteger idx)
    {
        Var<s64> instance(vm, idx);
        SQCATCH_NOEXCEPT(vm) {
            return;
        }

        sq_pop(vm,1);

        value = instance.value.value();
    }

    static void push(HSQUIRRELVM vm, const int64_t& value)
    {
        s64 v;
        v.set(value);

        Var<s64>::push(vm, v);
    }
};

template<>
struct Var<const int64_t&>
{
    int64_t value;

    Var(HSQUIRRELVM vm, SQInteger idx)
    {
        Var<s64> instance(vm, idx);
        SQCATCH_NOEXCEPT(vm) {
            return;
        }

        sq_pop(vm,1);

        value = instance.value.value();
    }

    static void push(HSQUIRRELVM vm, const int64_t& value)
    {
        s64 v;
        v.set(value);

        Var<s64>::push(vm, v);
    }
};

template<>
struct Var<uint64_t>
{
    uint64_t value;

    Var(HSQUIRRELVM vm, SQInteger idx)
    {
        Var<u64> instance(vm, idx);
        SQCATCH_NOEXCEPT(vm) {
            return;
        }

        sq_pop(vm,1);

        value = instance.value.value();
    }

    static void push(HSQUIRRELVM vm, const uint64_t& value)
    {
        u64 v;
        v.set(value);

        Var<u64>::push(vm, v);
    }
};

template<>
struct Var<const uint64_t&>
{
    uint64_t value;

    Var(HSQUIRRELVM vm, SQInteger idx)
    {
        Var<u64> instance(vm, idx);
        SQCATCH_NOEXCEPT(vm) {
            return;
        }

        sq_pop(vm,1);

        value = instance.value.value();
    }

    static void push(HSQUIRRELVM vm, const uint64_t& value)
    {
        u64 v;
        v.set(value);

        Var<u64>::push(vm, v);
    }
};
#endif // SQRAT_WRAP_INTEGER64

#define INTEGER_LIST(type) \
template<> \
struct Var< std::list<type> > { \
\
    std::list<type> value; \
\
    Var(HSQUIRRELVM vm, SQInteger idx) { \
        SQObjectType value_type = sq_gettype(vm, idx); \
\
        if(OT_ARRAY != value_type) \
        { \
            SQTHROW(vm, FormatTypeError(vm, idx, _SC("array"))); \
        } \
\
        sq_push(vm, idx); \
        sq_pushnull(vm); \
\
        while(SQ_SUCCEEDED(sq_next(vm, -2))) \
        { \
            Var<type> instance(vm, -1); \
            SQCATCH_NOEXCEPT(vm) { \
                return; \
            } \
\
            value.push_back(instance.value); \
            sq_pop(vm, 2); \
        } \
\
        sq_pop(vm, 2); \
    } \
\
    static void push(HSQUIRRELVM vm, const std::list<type>& value) { \
        SQInteger i = 0; \
\
        sq_newarray(vm, value.size()); \
\
        for(auto v : value) \
        { \
            sq_pushinteger(vm, i++); \
            Var<type>::push(vm, v); \
            sq_set(vm, -3); \
        } \
    } \
}; \
\
template<> \
struct Var<const std::list<type>&> { \
\
    std::list<type> value; \
\
    Var(HSQUIRRELVM vm, SQInteger idx) { \
        SQObjectType value_type = sq_gettype(vm, idx); \
\
        if(OT_ARRAY != value_type) \
        { \
            SQTHROW(vm, FormatTypeError(vm, idx, _SC("array"))); \
        } \
\
        sq_push(vm, idx); \
        sq_pushnull(vm); \
\
        while(SQ_SUCCEEDED(sq_next(vm, -2))) \
        { \
            Var<type> instance(vm, -1); \
            SQCATCH_NOEXCEPT(vm) { \
                return; \
            } \
\
            value.push_back(instance.value); \
            sq_pop(vm, 2); \
        } \
\
        sq_pop(vm, 2); \
    } \
\
    static void push(HSQUIRRELVM vm, const std::list<type>& value) { \
        SQInteger i = 0; \
\
        sq_newarray(vm, value.size()); \
\
        for(auto v : value) \
        { \
            sq_pushinteger(vm, i++); \
            Var<type>::push(vm, v); \
            sq_set(vm, -3); \
        } \
    } \
};

INTEGER_LIST(int8_t)
INTEGER_LIST(uint8_t)
INTEGER_LIST(int16_t)
INTEGER_LIST(uint16_t)
INTEGER_LIST(int32_t)
INTEGER_LIST(uint32_t)
INTEGER_LIST(int64_t)
INTEGER_LIST(uint64_t)

#endif // LIBCOMP_SRC_SQRATTYPESSOURCE_H
