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

class DatabaseBind;
class DatabaseQuery;

class PersistentObject : public Object
{
public:
    /// Map of MetaObject definitions by the source object's C++ type
    typedef std::unordered_map<std::type_index,
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

    /*
     * Get the MetaObject definition associated to the object.
     * @return Pointer to the MetaObject defintion
     */
    virtual std::shared_ptr<libobjgen::MetaObject> GetObjectMetadata() = 0;

    /*
     * Get database bindings for every data member.
     * @return List of pointers to data member database binds
     */
    virtual std::list<libcomp::DatabaseBind*> GetMemberBindValues() = 0;

    /*
     * Load the object from a successfully executed query.
     * @param query Database query that has executed to load from
     * @return true on success, false on failure
     */
    virtual bool LoadDatabaseValues(DatabaseQuery& query) = 0;

    /*
     * Register a derived class object to the cache and get a new UUID if not
     * specified.
     * @param self Pointer to the object itself
     * @return true on success, false on failure
     */
    static bool Register(const std::shared_ptr<PersistentObject>& self);

    /*
     * Unregisters an object from the cache and marks it as deleted.
     * Once an object is marked as deleted, it will not be cached again.
     */
    void Unregister();

    /*
     * Check if the record is marked as deleted from the database.
     * @return true if it is deleted, false if it is not
     */
    bool IsDeleted();

    /*
     * Register all derived types in libcomp to the TypeMap.
     * Persisted types needed in other databases should derive from this class
     * to register their own as well.
     * @return true on success, false on failure
     */
    static bool Initialize();

    /*
     * Retrieve an object by its UUID but do not load from the database.
     * @param uuid UUID to retrieve from the cache
     * @return Pointer to the cached object or nullptr if it doesn't exist
     */
    static std::shared_ptr<PersistentObject> GetObjectByUUID(
        const libobjgen::UUID& uuid);

    /*
     * Retrieve an object of the specified type by its UUID from the cache
     * or database.
     * @param uuid UUID of the object to load
     * @return Pointer to the object or nullptr if it doesn't exist
     */
    template<class T> static std::shared_ptr<T> LoadObjectByUUID(
    const libobjgen::UUID& uuid)
    {
        if(std::is_base_of<PersistentObject, T>::value)
        {
            return std::dynamic_pointer_cast<T>(LoadObjectByUUID(
                typeid(T), uuid));
        }

        return nullptr;
    }

    /*
     * Retrieve an object of the specified type ID by its UUID from the cache
     * or database.
     * @param type C++ type representing the object type to load
     * @param uuid UUID of the object to load
     * @return Pointer to the object or nullptr if it doesn't exist
     */
    static std::shared_ptr<PersistentObject> LoadObjectByUUID(
        std::type_index type, const libobjgen::UUID& uuid);

    /*
     * Get all PersistentObject derived class MetaObject definitions.
     * @return Map of MetaObject definitions by the source object's C++ type
     */
    static TypeMap& GetRegistry();

    /*
     * Get the PersistentObject of the specified type's MetaObject definition.
     * @return Pointer to the MetaObject definition
     */
    template<class T> static const std::shared_ptr<libobjgen::MetaObject>
        GetRegisteredMetadata()
    {
        if(std::is_base_of<PersistentObject, T>::value)
        {
            return GetRegisteredMetadata(typeid(T));
        }

        return nullptr;
    }

    /*
     * Get the PersistentObject of the specified type ID's MetaObject definition.
     * @param type C++ type representing the object type to load
     * @return Pointer to the MetaObject definition
     */
    static const std::shared_ptr<libobjgen::MetaObject> GetRegisteredMetadata(
        std::type_index type);

    /*
     * Get the MetaObject definition from loading a byte array.
     * @param bytes Byte array of the definition
     * @param length Number of bytes in the array
     * @return Pointer to the MetaObject definition or nullptr on failure
     */
    static std::shared_ptr<libobjgen::MetaObject> GetMetadataFromBytes(
        const char* bytes, size_t length);

    /*
     * Create a new instance of a PersistentObject of the specified type.
     * @return Pointer to a new PersistentObject of the specified type
     */
    template<class T> static std::shared_ptr<T> New()
    {
        if(std::is_base_of<PersistentObject, T>::value)
        {
            return std::dynamic_pointer_cast<T>(New(typeid(T)));
        }

        return nullptr;
    }

    /*
     * Create a new instance of a PersistentObject of the specified type ID.
     * @param type C++ type representing the object type to load
     * @return Pointer to a new PersistentObject of the specified type ID
     */
    static std::shared_ptr<PersistentObject> New(std::type_index type);

    /*
     * Save a new record to the database.
     * @return true on success, false on failure
     */
    bool Insert();

    /*
     * Save a new record to the database.
     * @param obj Pointer to the object to save
     * @return true on success, false on failure
     */
    static bool Insert(std::shared_ptr<PersistentObject>& obj);

    /*
     * Update an existing record in the database.
     * @return true on success, false on failure
     */
    bool Update();

    /*
     * Update an existing record in the database.
     * @param obj Pointer to the object to update
     * @return true on success, false on failure
     */
    static bool Update(std::shared_ptr<PersistentObject>& obj);

    /*
     * Deletes an existing record from the database.
     * @return true on success, false on failure
     */
    bool Delete();

    /*
     * Deletes an existing record from the database.
     * @param obj Pointer to the object to delete
     * @return true on success, false on failure
     */
    static bool Delete(std::shared_ptr<PersistentObject>& obj);

protected:
    /*
     * Register a derived class type with a function to describe it to the
     * database.
     * @param type C++ type representing the object type to load
     * @param obj MetaObject definition to register
     * @param f Function used to create a new object of the specified type
     */
    static void RegisterType(std::type_index type,
        const std::shared_ptr<libobjgen::MetaObject>& obj,
        const std::function<PersistentObject*()>& f);

    /*
     * Load an object from the database from a field database binding.
     * @param type C++ type representing the object type to load
     * @param pValue Pointer to a field bound to a database column
     * @return Pointer to the object or nullptr if it doesn't exist
     */
    static std::shared_ptr<PersistentObject> LoadObject(std::type_index type,
        DatabaseBind *pValue);

    /// Static value to be set to true if any PersistentObject type fails
    /// to register itself at runtime
    static bool sInitializationFailed;

    /// UUID associated to the object
    libobjgen::UUID mUUID;

private:
    /// Map of intantiated objects listed by their UUID
    static std::unordered_map<std::string,
        std::weak_ptr<PersistentObject>> sCached;

    /// Static map of MetaObject definitions by the source object's C++ type
    static TypeMap sTypeMap;

    /// Static map of functions to create an object by their C++ type
    static std::unordered_map<std::type_index,
        std::function<PersistentObject*()>> sFactory;

    /// Pointer to object itself
    std::weak_ptr<PersistentObject> mSelf;

    /// Indicator that the object has been deleted and should not be cached again
    bool mDeleted;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_PERSISTENTOBJECT_H
