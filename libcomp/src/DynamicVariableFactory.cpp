/**
 * @file libcomp/src/DynamicVariableFactory.cpp
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

#include "DynamicVariableFactory.h"

// libcomp Includes
#include "DynamicVariableArray.h"
#include "DynamicVariableInt.h"
#include "DynamicVariableList.h"
#include "DynamicVariableReference.h"
#include "DynamicVariableString.h"

using namespace libcomp;

DynamicVariableFactory::DynamicVariableFactory()
{
    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_S8] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<int8_t>(metaVariable));
        };
    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_U8] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<uint8_t>(metaVariable));
        };

    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_S16] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<int16_t>(metaVariable));
        };
    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_U16] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<uint16_t>(metaVariable));
        };

    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_S32] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<int32_t>(metaVariable));
        };
    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_U32] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<uint32_t>(metaVariable));
        };

    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_S64] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<int64_t>(metaVariable));
        };
    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_U64] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<uint64_t>(metaVariable));
        };

    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<float>(metaVariable));
        };
    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableInt<double>(metaVariable));
        };

    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_STRING] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableString(metaVariable));
        };

    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_ARRAY] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableArray(metaVariable));
        };
    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_LIST] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableList(metaVariable));
        };

    mAllocators[libobjgen::MetaVariable::MetaVariableType_t::TYPE_REF] =
        [](const std::shared_ptr<libobjgen::MetaVariable>& metaVariable)
        {
            return std::shared_ptr<DynamicVariable>(
                new DynamicVariableReference(metaVariable));
        };
}

std::shared_ptr<DynamicVariable> DynamicVariableFactory::Create(
    const std::shared_ptr<libobjgen::MetaVariable>& metaVariable) const
{
    auto it = mAllocators.find(metaVariable->GetMetaType());

    if(it != mAllocators.end())
    {
        return it->second(metaVariable);
    }

    return {};
}
