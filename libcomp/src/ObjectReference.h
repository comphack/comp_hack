/**
 * @file libcomp/src/ObjectReference.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Dual purpose reference class to object or persisted object.
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

#ifndef LIBCOMP_SRC_OBJECTREFERENCE_H
#define LIBCOMP_SRC_OBJECTREFERENCE_H

// libcomp Includes
#include "PersistentObject.h"

namespace libcomp
{

template <class T>
class ObjectReference
{
public:
    ObjectReference()
    {
        mLoadFailed = false;
    }

    ObjectReference(std::shared_ptr<T> ref)
    {
        mLoadFailed = false;
        SetReference(ref);
    }

    bool IsNull() const
    {
        return mUUID.IsNull() && nullptr == mRef;
    }

    const std::shared_ptr<T> Get()
    {
        LoadReference();

        return mRef;
    }

    const std::shared_ptr<T> GetCurrentReference() const
    {
        return mRef;
    }

    bool LoadReference()
    {
        if(IsPersistentReference() && !mUUID.IsNull() && !mLoadFailed)
        {
            mRef = PersistentObject::LoadObjectByUUID<T>(mUUID);
            mLoadFailed = nullptr == mRef;
        }
        return mUUID.IsNull() || nullptr != mRef;
    }

    void SetReference(const std::shared_ptr<T>& ref)
    {
        if(IsPersistentReference())
        {
            SetUUID(std::dynamic_pointer_cast<PersistentObject>(ref)->GetUUID());
        }

        mRef = ref;
    }

    const libobjgen::UUID& GetUUID() const
    {
        return mUUID;
    }

    bool SetUUID(const libobjgen::UUID& uuid)
    {
        if(IsPersistentReference())
        {
            mUUID = uuid;
            mRef = nullptr;
            mLoadFailed = false;
        }

        return false;
    }

    bool IsPersistentReference()
    {
        return std::is_base_of<PersistentObject, T>::value;
    }

protected:
    std::shared_ptr<T> mRef;

    libobjgen::UUID mUUID;

    bool mLoadFailed;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_OBJECTREFERENCE_H
