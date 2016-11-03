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

class DynamicObject : Object
{
public:
    DynamicObject(const std::shared_ptr<libobjgen::MetaObject>& metaObject);

    virtual bool IsValid(bool recursive = true) const;

    virtual bool Load(ObjectInStream& stream);
    virtual bool Save(ObjectOutStream& stream) const;

    virtual uint16_t GetDynamicSizeCount() const;

private:
    std::shared_ptr<libobjgen::MetaObject> mMetaData;
    std::list<std::shared_ptr<DynamicVariable>> mVariables;
    std::unordered_map<std::string, std::shared_ptr<
        DynamicVariable>> mVariableLookup;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DYNAMICOBJECT_H
