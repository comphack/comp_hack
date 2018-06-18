/**
 * @file libcomp/src/PersistentObject.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Derived class from Object and base class for peristed objgen objects.
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

#ifndef LIBCOMP_SRC_PERSISTENTOBJECT_H
#define LIBCOMP_SRC_PERSISTENTOBJECT_H

// libcomp Includes
#include <CString.h>
#include <Object.h>

// libobjgen Includes
#include <MetaObject.h>
#include <UUID.h>

// Standard C++ 11 Includes
#include <typeindex>

namespace libcomp
{

class Database;
class DatabaseBind;
class DatabaseQuery;
class ScriptEngine;

/**
 * Base class of a all dynamically generated objects that persist in the
 * database. Persistent objects are cached upon load or by explicitly
 * registering them which will allow them to be retrieved via a generated
 * UUID.
 */
class PersistentObject : public Object
{
    friend class ScriptEngine;

public:
    /// Map of MetaObject definitions by the source object's C++ type hash
    typedef std::unordered_map<size_t,
        std::shared_ptr<libobjgen::MetaObject>> TypeMap;

    /**
     * Create a persistent object with no UUID.
     */
    PersistentObject();

    /**
     * Override the copy constructor to still default the persistent members.
     * @param other The other object to copy
     */
    PersistentObject(const PersistentObject& other);

    /**
     * Clean up the object and remove it from the UUID cache.
     */
    ~PersistentObject();

    /**
     * Get the object's UUID.
     * @return The object's UUID
     */
    libobjgen::UUID GetUUID() const;

    /**
     * Get the MetaObject definition associated to the object.
     * @return Pointer to the MetaObject defintion
     */
    virtual std::shared_ptr<libobjgen::MetaObject> GetObjectMetadata() = 0;

    /**
     * Get database bindings for all or changed data members.
     * Calling this will clear the set of fields marked as changed.
     * @param retrieveAll Optional parameter to retrieve all data members
     *  instead of the default change only behavior
     * @param clearChanges Optional parameter to clear not clear all
     *  fields marked as changed
     * @return List of pointers to data member database binds
     */
    virtual std::list<libcomp::DatabaseBind*> GetMemberBindValues(
        bool retrieveAll = false, bool clearChanges = true) = 0;

    /**
     * Load the object from a successfully executed query.
     * @param query Database query that has executed to load from
     * @return true on success, false on failure
     */
    virtual bool LoadDatabaseValues(DatabaseQuery& query) = 0;

    /**
     * Register a derived class object to the cache and get a new UUID if not
     * specified.
     * @param self Pointer to the object itself
     * @param uuid Optional predefined UUID to use if the record has a null
     *  value
     * @return true on success, false on failure
     */
    static bool Register(const std::shared_ptr<PersistentObject>& self,
        const libobjgen::UUID& pUuid = NULLUUID);

    /**
     * Unregisters an object from the cache and marks it as deleted.
     * Once an object is marked as deleted, it will not be cached again.
     */
    void Unregister();

    /**
     * Check if the record is marked as deleted from the database.
     * @return true if it is deleted, false if it is not
     */
    bool IsDeleted();

    /**
     * Register all derived types in libcomp to the TypeMap.
     * Persisted types needed in other databases should derive from this class
     * to register their own as well.
     * @return true on success, false on failure
     */
    static bool Initialize();

    /**
     * Retrieve an object by its UUID but do not load from the database.
     * @param uuid UUID to retrieve from the cache
     * @return Pointer to the cached object or nullptr if it doesn't exist
     */
    static std::shared_ptr<PersistentObject> GetObjectByUUID(
        const libobjgen::UUID& uuid);

    /**
     * Retrieve all objects of the specified type by its UUID from the
     * database.  Use sparingly.
     * @param db Database to load from
     * @return List of pointers to all objects of the specified type
     */
    template<class T> static std::list<std::shared_ptr<T>> LoadAll(
        const std::shared_ptr<Database>& db)
    {
        std::list<std::shared_ptr<T>> retval;
        if(std::is_base_of<PersistentObject, T>::value)
        {
            for(auto obj : LoadObjects(typeid(T).hash_code(), db, nullptr))
            {
                retval.push_back(std::dynamic_pointer_cast<T>(obj));
            }
        }

        return retval;
    }

    /**
     * Retrieve an object of the specified type by its UUID from the cache
     * or database.
     * @param db Database to load from
     * @param uuid UUID of the object to load
     * @param reload Forces a reload from the DB if true
     * @return Pointer to the object or nullptr if it doesn't exist
     */
    template<class T> static std::shared_ptr<T> LoadObjectByUUID(
        const std::shared_ptr<Database>& db, const libobjgen::UUID& uuid,
        bool reload = false)
    {
        if(std::is_base_of<PersistentObject, T>::value)
        {
            return std::dynamic_pointer_cast<T>(LoadObjectByUUID(
                typeid(T).hash_code(), db, uuid, reload));
        }

        return nullptr;
    }

    /**
     * Retrieve an object of the specified type ID by its UUID from the cache
     * or database.
     * @param typeHash C++ type hash representing the object type to load
     * @param db Database to load from
     * @param uuid UUID of the object to load
     * @param reload Forces a reload from the DB if true
     * @return Pointer to the object or nullptr if it doesn't exist
     */
    static std::shared_ptr<PersistentObject> LoadObjectByUUID(
        size_t typeHash, const std::shared_ptr<Database>& db,
        const libobjgen::UUID& uuid, bool reload = false);

    /**
     * Get all PersistentObject derived class MetaObject definitions.
     * @return Map of MetaObject definitions by the source object's C++ type
     */
    static TypeMap& GetRegistry();

    /**
     * Get the C++ type hash by the associated object type's name.
     * @param name Name of a persistent object type
     * @param result true if the type is registered, false if it is not
     * @return C++ type hash matching the supplied name
     */
    static size_t GetTypeHashByName(const std::string& name, bool& result);

    /**
     * Get the C++ type hash by the associated object type's name.
     * @param name Name of a persistent object type
     * @return C++ type hash matching the supplied name
     */
    static size_t GetTypeHashByName(const std::string& name);

    /**
     * Get the PersistentObject of the specified type's MetaObject definition.
     * @return Pointer to the MetaObject definition
     */
    template<class T> static const std::shared_ptr<libobjgen::MetaObject>
        GetRegisteredMetadata()
    {
        if(std::is_base_of<PersistentObject, T>::value)
        {
            return GetRegisteredMetadata(typeid(T).hash_code());
        }

        return nullptr;
    }

    /**
     * Get the PersistentObject of the specified type ID's MetaObject definition.
     * @param typeHash C++ type hash representing the object type to load
     * @return Pointer to the MetaObject definition
     */
    static const std::shared_ptr<libobjgen::MetaObject> GetRegisteredMetadata(
        size_t typeHash);

    /**
     * Get the MetaObject definition from loading a byte array.
     * @param bytes Byte array of the definition
     * @param length Number of bytes in the array
     * @return Pointer to the MetaObject definition or nullptr on failure
     */
    static std::shared_ptr<libobjgen::MetaObject> GetMetadataFromBytes(
        const char* bytes, size_t length);

    /**
     * Create a new instance of a PersistentObject of the specified type.
     * @param doRegister Register the pointer automatically on success.
     * @return Pointer to a new PersistentObject of the specified type
     */
    template<class T> static std::shared_ptr<T> New(bool doRegister = false)
    {
        if(std::is_base_of<PersistentObject, T>::value)
        {
            auto result = std::dynamic_pointer_cast<T>(New(typeid(T).hash_code()));
            if(doRegister)
            {
                result->Register(result);
            }
            return result;
        }

        return nullptr;
    }

    /**
     * Create a new instance of a PersistentObject of the specified type ID.
     * @param typeHash C++ type hash representing the object type to load
     * @return Pointer to a new PersistentObject of the specified type ID
     */
    static std::shared_ptr<PersistentObject> New(size_t typeHash);

    /**
     * Convert a list of PersistentObject derived object pointers into a
     * list of PersistentObject pointers.
     * @param objList List of PersistentObject derived object pointers
     * @return List of PersistentObject pointers
     */
    template<class T> static std::list<std::shared_ptr<PersistentObject>>
        ToList(std::list<std::shared_ptr<T>> objList)
    {
        std::list<std::shared_ptr<PersistentObject>> converted;
        if(std::is_base_of<PersistentObject, T>::value)
        {
            for(auto obj : objList)
            {
                converted.push_back(std::dynamic_pointer_cast<
                    PersistentObject>(obj));
            }
        }

        return converted;
    }

    /**
     * Save a new record to the database.
     * @param db Database to load from
     * @return true on success, false on failure
     */
    bool Insert(const std::shared_ptr<Database>& db);

    /**
     * Update an existing record in the database.
     * @param db Database to load from
     * @return true on success, false on failure
     */
    bool Update(const std::shared_ptr<Database>& db);

    /**
     * Deletes an existing record from the database.
     * @param db Database to load from
     * @return true on success, false on failure
     */
    bool Delete(const std::shared_ptr<Database>& db);

protected:
    /**
     * Register a derived class type with a function to describe it to the
     * database.
     * @param type C++ type representing the object type to load
     * @param obj MetaObject definition to register
     * @param f Function used to create a new object of the specified type
     */
    static void RegisterType(std::type_index type,
        const std::shared_ptr<libobjgen::MetaObject>& obj,
        const std::function<PersistentObject*()>& f);

    /**
     * Load an object from the database from a field database binding.
     * @param typeHash C++ type hash representing the object type to load
     * @param db Database to load from
     * @param pValue Pointer to a field bound to a database column
     * @return Pointer to the object or nullptr if it doesn't exist
     */
    static std::shared_ptr<PersistentObject> LoadObject(size_t typeHash,
        const std::shared_ptr<Database>& db, DatabaseBind *pValue);

    /**
     * Load an object from the database from a field database binding.
     * @param typeHash C++ type hash representing the object type to load
     * @param db Database to load from
     * @return Pointer to the object or nullptr if it doesn't exist
     */
    static std::shared_ptr<PersistentObject> LoadObject(size_t typeHash,
        const std::shared_ptr<Database>& db);

    /**
     * Load multiple objects from the database from a field database binding.
     * @param typeHash C++ type hash representing the object type to load
     * @param db Database to load from
     * @param pValue Pointer to a field bound to a database column
     * @return List of pointers objects
     */
    static std::list<std::shared_ptr<PersistentObject>> LoadObjects(
        size_t typeHash, const std::shared_ptr<Database>& db,
        DatabaseBind *pValue);

    /**
     * Load multiple objects from the database from a field database binding.
     * @param typeHash C++ type hash representing the object type to load
     * @param db Database to load from
     * @return List of pointers objects
     */
    static std::list<std::shared_ptr<PersistentObject>> LoadObjects(
        size_t typeHash, const std::shared_ptr<Database>& db);

    /// Static value to be set to true if any PersistentObject type fails
    /// to register itself at runtime
    static bool sInitializationFailed;

    /// UUID associated to the object
    libobjgen::UUID mUUID;

    /// Set of fields that have been updated since the last save operation
    std::set<std::string> mDirtyFields;

private:
    /// Map of intantiated objects listed by their UUID
    static std::unordered_map<std::string,
        std::weak_ptr<PersistentObject>> sCached;

    /// Mutex to lock accessing the cache
    static std::mutex mCacheLock;

    /// Static map of MetaObject definitions by the source object's C++ type hash
    static TypeMap sTypeMap;

    /// Static map of functions to create an object by their C++ type hash
    static std::unordered_map<size_t,
        std::function<PersistentObject*()>> sFactory;

    /// Static map of C++ type hashes by name
    static std::unordered_map<std::string, size_t> sTypeNames;

    /// Pointer to object itself
    std::weak_ptr<PersistentObject> mSelf;

    /// Indicator that the object has been deleted and should not be cached again
    bool mDeleted;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_PERSISTENTOBJECT_H
