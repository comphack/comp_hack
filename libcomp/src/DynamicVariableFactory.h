/**
 * @file libcomp/src/DynamicVariableFactory.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Factory to create DynamicVariable objects of the correct type.
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

#ifndef LIBCOMP_SRC_DYNAMICVARIABLEFACTORY_H
#define LIBCOMP_SRC_DYNAMICVARIABLEFACTORY_H

// libcomp Includes
#include "DynamicVariable.h"
#include "EnumMap.h"

// libobjgen Includes
#include <MetaVariable.h>

namespace libcomp
{

class DynamicVariableFactory
{
public:
    DynamicVariableFactory();

    std::shared_ptr<DynamicVariable> Create(const std::shared_ptr<
        libobjgen::MetaVariable>& metaVariable) const;

private:
    EnumMap<libobjgen::MetaVariable::MetaVariableType_t,
        std::function<std::shared_ptr<DynamicVariable>(const std::shared_ptr<
        libobjgen::MetaVariable>& metaVariable)>> mAllocators;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DYNAMICVARIABLEFACTORY_H
