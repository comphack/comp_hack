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

#ifndef LIBCOMP_SRC_OBJECTREFERENCE_H
#define LIBCOMP_SRC_OBJECTREFERENCE_H

// libcomp Includes
#include "Database.h"
#include "PersistentObject.h"

namespace libcomp
{

/**
 * Represents a UUID and its associated PersistentObject that can either
 * be loaded from the DB or pending loading from the DB.  These are
 * cached in @ref ObjectReferenceData when the UUID is not null so each
 * reference to that UUID will have the loaded the PersistentObject
 * accessible the moment it gets cached.
 */
class ObjectReferenceData
{
public:
    /**
     * Create a null reference with no unsaved object.
     */
    ObjectReferenceData()
    {
        mLoadFailed = false;
    }

    /**
     * Create a reference to a persistent object.  This object does
     * not need to be saved in the DB yet.
     * @param ref Pointer to the object this references
     * @param uuid Optional param to set the UUID only for when the
     * PersistentObject is not loaded yet
     */
    ObjectReferenceData(const std::shared_ptr<PersistentObject>& ref,
        const libobjgen::UUID& uuid = libobjgen::UUID())
        : mRef(ref),
        mUUID(ref != nullptr ? ref->GetUUID() : uuid)
    {
        mLoadFailed = false;
    }

    /// Referenced object pointer
    std::shared_ptr<PersistentObject> mRef;

    /// UUID of the persistent object reference
    const libobjgen::UUID mUUID;

    /// Indicator that the object with the matching UUID failed to load
    bool mLoadFailed;
};

/**
 * Templated class that handles references to a @ref PersistentObject
 * derived class that can be lazy loaded from the database when needed.
 * References to a UUID are cached between all instances of this object
 * allowing one database load to populate all references simultaneously.
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
        mData = sNull;
    }

    /**
     * Create a reference of the templated type with the UUID set.
     * @param uuid UUID of the referenced object
     */
    ObjectReference(const libobjgen::UUID& uuid)
    {
        mData = sNull;
        SetUUID(uuid);
    }

    /**
     * Create a reference of the templated type with the pointer set.
     * @param ref Pointer to the referenced object
     */
    ObjectReference(std::shared_ptr<T> ref)
    {
        mData = sNull;
        SetReference(ref);
    }

    /**
     * Clean up the reference.
     */
    ~ObjectReference()
    {
        ClearReference();
    }

    /**
     * Check if there is no UUID and no reference set.
     * @return true if both are null, false if one is not
     */
    bool IsNull() const
    {
        return mData == sNull;
    }

    /**
     * Get the pointer to the referenced object.  If the object
     * is persistent and has not be loaded from the database yet
     * this will not load it however if it is already cached at
     * a server level, it will be cached here as well.
     * @return Pointer to the referenced object
     */
    const std::shared_ptr<T> Get()
    {
        LoadReference();

        return GetReference();
    }

    /**
     * Get the pointer to the referenced object.  This will
     * cause the reference to load from the database if the
     * UUID is set and has not been loaded from the DB already.
     * @param db Database to load from
     * @param reload Forces a reload from the DB if true
     * @return Pointer to the referenced object
     */
    const std::shared_ptr<T> Get(const std::shared_ptr<Database>& db,
        bool reload = false)
    {
        LoadReference(db, reload);

        return GetReference();
    }

    /**
     * Get the pointer to the referenced object but do
     * not load from the database if it is not loaded already.
     * @return Pointer to the referenced object
     */
    const std::shared_ptr<T> GetCurrentReference() const
    {
        return GetReference();
    }

    /**
     * Get the pointer to the referenced object but do
     * not load from the database if it is not loaded already.
     * @return Pointer to the referenced object
     */
    std::shared_ptr<T> operator->()
    {
        return Get();
    }

    /**
    * Update the data associated to this reference.
    * @param ref Pointer to the referenced object
    */
    void SetReference(const std::shared_ptr<T>& ref)
    {
        libobjgen::UUID uuid;
        auto pRef = std::dynamic_pointer_cast<PersistentObject>(ref);
        if (nullptr != pRef)
        {
            uuid = pRef->GetUUID();
        }

        SetReference(uuid, pRef);
    }

    /**
     * Checks if the pointer is valid or a nullptr.
     * @return true if the reference is set, false if it is not
     */
    operator bool() const
    {
        return nullptr != mData->mRef;
    }

    /**
     * Get the UUID of the reference.
     * @return UUID of the reference
     */
    const libobjgen::UUID& GetUUID() const
    {
        return mData->mUUID;
    }

    /**
     * Set the UUID of the reference.  If the reference associated
     * to that UUID is already cached, it will not need to be loaded.
     * If it is not, it will need to be lazy loaded later.
     * @param uuid UUID of the persistent object
     * @return true on success, false on failure
     */
    bool SetUUID(const libobjgen::UUID& uuid)
    {
        if(mData->mUUID != uuid)
        {
            SetReference(uuid);
        }
        return true;
    }

    /**
     * Clear the loaded pointer associated to a UUID when an object
     * needs to be cleaned up.  This allows circular references in
     * objgen schemas to not cause issues with garbage collection.
     * @param uuid UUID of the persistent object
     * @return true if it was loaded, false if it was not
     */
    static bool Unload(const libobjgen::UUID& uuid)
    {
        std::string uuidStr = uuid.ToString();

        std::lock_guard<std::mutex> lock(mReferenceLock);
        auto iter = sData.find(uuidStr);
        if(iter != sData.end())
        {
            bool loaded = iter->second->mRef != nullptr;
            iter->second->mRef = nullptr;
            return loaded;
        }
        return false;
    }

    /**
     * Copy another reference's data after clearing the stored reference.
     * @param other Other reference to copy
     */
    ObjectReference& operator=(const ObjectReference& other)
    {
        auto uuid = other.GetUUID();
        if(uuid != NULLUUID)
        {
            ClearReference();
            SetUUID(uuid);
        }
        else
        {
            SetReference(other.GetReference());
        }
        return *this;
    }

protected:
    /**
     * Get the PersistentObject reference casted to the templated
     * type.
     * @return Casted pointer to the reference
     */
    const std::shared_ptr<T> GetReference() const
    {
        std::lock_guard<std::mutex> lock(mReferenceLock);
        return std::dynamic_pointer_cast<T>(mData->mRef);
    }

    /**
     * Load the referenced object from the database by its UUID.
     * This will do nothing if the UUID is not set, the load fails or
     * the templated type is a generic PersistentObject. Calling this
     * function without a DB set will pull from the PersistentObject
     * cache instead.
     * @param db Database to load from
     * @param reload Forces a reload from the DB if true
     * @return true on success, false on failure
     */
    bool LoadReference(const std::shared_ptr<Database>& db = nullptr,
        bool reload = false)
    {
        // Do not count an attempt to load without a DB as a
        // load failure
        if(!mData->mLoadFailed && (reload || nullptr == mData->mRef) &&
            !mData->mUUID.IsNull())
        {
            auto uuid = mData->mUUID;

            bool dbLoad = nullptr != db &&
                typeid(PersistentObject) != typeid(T);
            std::shared_ptr<PersistentObject> pRef;
            if(dbLoad)
            {
                pRef = std::dynamic_pointer_cast<PersistentObject>(
                    PersistentObject::LoadObjectByUUID<T>(db, uuid,
                        reload));
            }
            else
            {
                pRef = PersistentObject::GetObjectByUUID(uuid);
            }

            SetReference(uuid, pRef, dbLoad);
        }

        return IsNull() || nullptr != GetReference();
    }
    
    /**
     * Update the data associated to this reference.  The values
     * set can be just a UUID, just the reference or both.
     * @param uuid UUID of the referenced object
     * @param ref Pointer to the referenced object
     * @param setLoadFailure Marks the reference to not attempt
     *  a second load if specified and the pointer passed in is null
     */
    void SetReference(const libobjgen::UUID& uuid,
        const std::shared_ptr<PersistentObject>& ref = nullptr,
        bool setLoadFailure = false)
    {
        ClearReference();

        if(!uuid.IsNull())
        {
            std::string uuidStr = uuid.ToString();

            std::lock_guard<std::mutex> lock(mReferenceLock);
            auto iter = sData.find(uuidStr);
            if(iter == sData.end())
            {
                sData[uuidStr] = std::shared_ptr<ObjectReferenceData>(
                    new ObjectReferenceData(ref, uuid));
            }
            else if(sData[uuidStr]->mRef == nullptr)
            {
                sData[uuidStr]->mRef = ref;
            }

            if(ref == nullptr && setLoadFailure)
            {
                sData[uuidStr]->mLoadFailed = true;
            }

            mData = sData[uuidStr];
        }
        else
        {
            if(ref != nullptr)
            {
                mData = std::shared_ptr<ObjectReferenceData>(
                    new ObjectReferenceData(ref));
            }
            else
            {
                mData = sNull;
            }
        }
    }

    /**
     * Clear the data associated to this reference, setting it
     * back to null.  If the old data associated is no longer
     * being used by another reference, clear it from the cache.
     */
    void ClearReference()
    {
        if(mData != nullptr && !mData->mUUID.IsNull())
        {
            std::string uuidStr = mData->mUUID.ToString();

            std::lock_guard<std::mutex> lock(mReferenceLock);
            mData = sNull;

            if(sData[uuidStr].use_count() == 1)
            {
                sData.erase(uuidStr);
            }
        }
        else
        {
            mData = sNull;
        }
    }

    /// Local copy of the data cached in sData
    std::shared_ptr<ObjectReferenceData> mData;

    /// Static cache of non-null UUIDs to persistent objects
    static std::unordered_map<std::string,
        std::shared_ptr<ObjectReferenceData>> sData;

    /// Default value of mData instantiated once to avoid newing up
    /// excess shared_ptrs
    static std::shared_ptr<ObjectReferenceData> sNull;

    /// Mutex to lock accessing the cache
    static std::mutex mReferenceLock;
};

template<class T>
std::unordered_map<std::string, std::shared_ptr<ObjectReferenceData>>
    ObjectReference<T>::sData;

template<class T>
std::shared_ptr<ObjectReferenceData> ObjectReference<T>::sNull = 
    std::shared_ptr<ObjectReferenceData>(new ObjectReferenceData);

template<class T>
std::mutex ObjectReference<T>::mReferenceLock;

} // namespace libcomp

#endif // LIBCOMP_SRC_OBJECTREFERENCE_H
