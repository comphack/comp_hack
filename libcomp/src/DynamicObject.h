/**
 * @file libcomp/src/DynamicObject.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Dynamically generated object.
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

#ifndef LIBCOMP_SRC_DYNAMICOBJECT_H
#define LIBCOMP_SRC_DYNAMICOBJECT_H

// libcomp Includes
#include <DynamicVariable.h>
#include <Object.h>

// libobjgen Includes
#include <MetaObject.h>

// Standard C++11 Includes
#include <memory>

namespace libcomp
{

/**
 * Represents an object that can be built dynamically at runtime.
 * While objgen generated code that derives from @ref Object
 * must be predefined and also compiled in the same fashion as other source
 * files, DynamicObject is intended to be used when an object definition
 * can be built on the fly such as a definition stored in the database.
 */
class DynamicObject : Object
{
public:
    /**
     * Create a new dynamic object from a MetaObject definition.
     * @param metaObject Pointer to a MetaObject definition
     */
    DynamicObject(const std::shared_ptr<libobjgen::MetaObject>& metaObject);

    virtual bool IsValid(bool recursive = true) const;

    //virtual bool Load(ObjectInStream& stream);
    //virtual bool Save(ObjectOutStream& stream) const;

    /**
     * Get the dynamic size count defined in the MetaObject definition.
     * @return The MetaObject definition's dynamic size count
     */
    virtual uint16_t GetDynamicSizeCount() const;

private:
    /// Pointer to the MetaObject definition
    std::shared_ptr<libobjgen::MetaObject> mMetaData;

    /// List of pointers to the variables defined in the MetaObject
    std::list<std::shared_ptr<DynamicVariable>> mVariables;

    /// Map of names to pointers to the variables defined in the MetaObject
    std::unordered_map<std::string, std::shared_ptr<
        DynamicVariable>> mVariableLookup;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DYNAMICOBJECT_H
