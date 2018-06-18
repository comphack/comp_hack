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

#endif // LIBCOMP_SRC_SQRATTYPESSOURCE_H
