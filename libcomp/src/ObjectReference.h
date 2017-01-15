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
#include "Database.h"
#include "PersistentObject.h"

namespace libcomp
{

/**
 * Templated class that serves the dual purpose of containing an
 * @ref Object reference as well as UUID for for a @ref PersistentObject
 * that can be lazy loaded from the database when needed.
 */
template <class T>
class ObjectReference
{
public:
    /**
     * Create an empty reference of the templated type.
     */
    ObjectReference()
    {
        mLoadFailed = false;
    }

    /**
     * Create a reference of the templated type with the pointer set.
     * @param ref Pointer to the referenced object
     */
    ObjectReference(std::shared_ptr<T> ref)
    {
        mLoadFailed = false;
        SetReference(ref);
    }

    /**
     * Check if there is no UUID and no reference pointer.
     * @return true if both are null, false if one is not
     */
    bool IsNull() const
    {
        return mUUID.IsNull() && nullptr == mRef;
    }

    /**
     * Get the pointer to the referenced object.  This will
     * cause the reference to load from the database if only
     * the UUID is set.
     * @param db Database to load from if persistent
     * @return Pointer to the referenced object
     */
    const std::shared_ptr<T> Get(const std::shared_ptr<Database>& db = nullptr)
    {
        LoadReference(db);

        return mRef;
    }

    /**
     * Get the pointer to the referenced object but do
     * not load from the databaseif it is not loaded already.
     * @return Pointer to the referenced object
     */
    const std::shared_ptr<T> GetCurrentReference() const
    {
        return mRef;
    }

    /**
     * Get the pointer to the referenced object but do
     * not load from the databaseif it is not loaded already.
     * @return Pointer to the referenced object
     */
    T* operator->() const
    {
        return mRef.get();
    }

    /**
     * Checks if the pointer is valid or a nullptr.
     */
    operator bool() const
    {
        return nullptr != mRef.get();
    }

    /**
     * Load the referenced object from the database by its UUID.
     * This will fail if the object is not persistent, the UUID
     * is not set or the load was attempted already and failed
     * without having the UUID modified.
     * @param db Database to load from if persistent
     * @return true on success, false on failure
     */
    bool LoadReference(const std::shared_ptr<Database>& db = nullptr)
    {
        // Do not count an attempt to load without a DB as a
        // load failure
        if(IsPersistentReference() && nullptr != db
            && !mUUID.IsNull() && !mLoadFailed)
        {
            mRef = PersistentObject::LoadObjectByUUID<T>(db, mUUID);
            mLoadFailed = nullptr == mRef;
        }
        return mUUID.IsNull() || nullptr != mRef;
    }

    /**
     * Set the referenced object pointer and also the UUID if
     * the object is persistent.
     * @param ref Pointer to the referenced object
     */
    void SetReference(const std::shared_ptr<T>& ref)
    {
        if(IsPersistentReference())
        {
            SetUUID(std::dynamic_pointer_cast<PersistentObject>(ref)->GetUUID());
        }

        mRef = ref;
    }

    /**
     * Get the UUID of the persistent object reference.
     * @return UUID of the persistent object reference
     */
    const libobjgen::UUID& GetUUID() const
    {
        return mUUID;
    }

    /**
     * Set the UUID of the persistent object reference and clear
     * the referenced object pointer to load later.  This will
     * fail if the object is not persistent.
     * @param uuid UUID of the persistent object
     * @return true on success, false on failure
     */
    bool SetUUID(const libobjgen::UUID& uuid)
    {
        if(IsPersistentReference())
        {
            mUUID = uuid;
            mRef = nullptr;
            mLoadFailed = false;
            return true;
        }

        return false;
    }

    /**
     * Check if the object referenced is of type @ref PersistentObject.
     * @return true if it is persistent, false if it is not
     */
    bool IsPersistentReference()
    {
        return std::is_base_of<PersistentObject, T>::value;
    }

protected:
    /// Referenced object pointer
    std::shared_ptr<T> mRef;

    /// UUID of the persistent object reference
    libobjgen::UUID mUUID;

    /// Indicator that the object with the matching UUID failed to load
    bool mLoadFailed;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_OBJECTREFERENCE_H
