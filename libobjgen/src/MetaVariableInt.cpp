/**
 * @file libobjgen/src/MetaVariableInt.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for an integer based object member variable.
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

#include "MetaVariableInt.h"

namespace libobjgen
{

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<int8_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_S8;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<uint8_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_U8;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<int16_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_S16;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<uint16_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_U16;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<int32_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_S32;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<uint32_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_U32;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<int64_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_S64;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<uint64_t>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_U64;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<float>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_FLOAT;
}

template<>
MetaVariable::MetaVariableType_t MetaVariableInt<double>::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_DOUBLE;
}

} // namespace libobjgen
