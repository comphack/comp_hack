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

#ifndef LIBCOMP_SRC_DYNAMICVARIABLEINT_H
#define LIBCOMP_SRC_DYNAMICVARIABLEINT_H

// libobjgen Includes
#include <MetaVariableInt.h>

namespace libcomp
{

/**
 * Represents a numeric variable that can be built dynamically at runtime.
 */
template<typename T>
class DynamicVariableInt : public DynamicVariable
{
public:
    /**
     * Create a new dynamic variable of the templated numeric type.
     * @param metaVariable Pointer to a MetaVariableInt<T> definition
     */
    DynamicVariableInt(const std::shared_ptr<libobjgen::MetaVariable>&
        metaVariable) : DynamicVariable(metaVariable)
    {
        mValue = std::dynamic_pointer_cast<libobjgen::MetaVariableInt<T>>(
            metaVariable)->GetDefaultValue();
    }

    /**
     * Clean up the variable.
     */
    virtual ~DynamicVariableInt()
    {
    }

    virtual bool Load(ObjectInStream& stream);
    virtual bool Save(ObjectOutStream& stream) const;

private:
    T mValue;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DYNAMICVARIABLEINT_H
