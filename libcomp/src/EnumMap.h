/**
 * @file libcomp/src/EnumMap.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base worker class to process messages for a thread.
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

#ifndef LIBCOMP_SRC_ENUMMAP_H
#define LIBCOMP_SRC_ENUMMAP_H

// Standard C++11 Includes
#include <unordered_map>

namespace libcomp
{

// Source: http://stackoverflow.com/questions/18837857/

/**
 * Helper class to convert an enum value into its numeric
 * representation so it can be used in sortable collections.
 */
class EnumClassHash
{
public:
    /**
     * Constructer to convert an enum to its numeric representation.
     * @param t Enum type to convert
     * @return Enum numeric representation
     */
    template<typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

/**
 * Validator that we are actually using an enum for the @ref EnumMap.
 */
template<typename Key>
using HashType = typename std::conditional<std::is_enum<Key>::value,
    EnumClassHash, std::hash<Key>>::type;

/**
 * Replacement for map with a enum key type and a templated value.
 */
template<typename Key, typename T>
using EnumMap = std::unordered_map<Key, T, HashType<Key>>;

} // namespace libcomp

#endif // LIBCOMP_SRC_ENUMMAP_H
