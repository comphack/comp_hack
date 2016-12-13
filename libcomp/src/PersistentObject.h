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
#include <Object.h>

// libobjgen Includes
#include <MetaObject.h>

// Standard C++ 11 Includes
#include <typeindex>

namespace libcomp
{

class PersistentObject : public Object
{
public:
    typedef std::unordered_map<std::type_index, std::shared_ptr<libobjgen::MetaObject>> TypeMap;

    PersistentObject();
    PersistentObject(const PersistentObject& other);
    virtual ~PersistentObject();

    /*
    *   Register a derived class object to the cache and get a new UUID if not specified.
    */
    bool Register(std::shared_ptr<PersistentObject>& self,
        const libobjgen::UUID& uuid = libobjgen::UUID());

    /*
    *   Register all derived types in libcomp to the TypeMap.
    *   Persisted types needed in other databases should derive from this class to
    *   register their own as well.
    */
    static void Initialize();

    /*
    *   Retrieve an object by its UUID but do not load from the database
    */
    static std::shared_ptr<PersistentObject> GetObjectByUUID(const libobjgen::UUID& uuid);

    /*
    *   Retrieve an object by its UUID and load from the database and cache if needed
    */
    static std::shared_ptr<PersistentObject> LoadObjectByUUID(const libobjgen::UUID& uuid);

    /*
    *   Returns all generated PersistentObject derived classes
    */
    static TypeMap& GetRegistry();

    /*
    *   Returns all generated PersistentObject derived classes
    */
    static const std::shared_ptr<libobjgen::MetaObject> GetRegisteredMetadata(std::type_index type);

    /*
    *   Returns the result of parsing metadata XML
    */
    static std::shared_ptr<libobjgen::MetaObject> GetMetadataFromXml(const std::string& xml);

    /*
    *   Returns a new instance of a PersistentObject derived type
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
    *   Returns a new instance of a PersistentObject derived type by type index
    */
    static std::shared_ptr<PersistentObject> New(std::type_index type);

protected:
    /*
    *   Register a derived class type with a function to describe it to the database
    */
    static void RegisterType(std::type_index type,
        const std::shared_ptr<libobjgen::MetaObject>& obj,
        const std::function<PersistentObject*()>& f);

private:
    static std::unordered_map<std::string, std::weak_ptr<PersistentObject>> sCached;

    static TypeMap sTypeMap;

    static std::unordered_map<std::type_index, std::function<PersistentObject*()>> sFactory;

    std::weak_ptr<PersistentObject> mSelf;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_PERSISTENTOBJECT_H
