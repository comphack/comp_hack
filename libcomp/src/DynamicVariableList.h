/**
 * @file libcomp/src/DynamicVariable.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Dynamically generated variable (for a DynamicObject).
 *
 * This file is part of the COMP_hack Object Generator Library (libobjgen).
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

#ifndef LIBCOMP_SRC_DYNAMICVARIABLELIST_H
#define LIBCOMP_SRC_DYNAMICVARIABLELIST_H

// libobjgen Includes
#include <MetaVariableList.h>

namespace libcomp
{

/**
 * Represents a list variable that can be built dynamically at runtime.
 */
class DynamicVariableList : public DynamicVariable
{
public:
    /**
     * Create a new dynamic list variable.
     * @param metaVariable Pointer to a MetaVariableList definition
     */
    DynamicVariableList(const std::shared_ptr<libobjgen::MetaVariable>&
        metaVariable);
        
    /**
     * Clean up the variable.
     */
    virtual ~DynamicVariableList();

    virtual bool Load(ObjectInStream& stream);
    virtual bool Save(ObjectOutStream& stream) const;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DYNAMICVARIABLELIST_H
