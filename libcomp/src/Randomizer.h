/**
 * @file libcomp/src/Randomizer.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Static utility class used for improved random number generation.
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

#ifndef LIBCOMP_SRC_RANDOMIZER_H
#define LIBCOMP_SRC_RANDOMIZER_H

#include <stdint.h>

namespace libcomp
{

/**
 * Static utility class used for improved random number generation. Random
 * number generation is handled on a per thread basis and is thus thread safe.
 */
class Randomizer
{
public:
    /**
     * Get a random number of the templated type between the minimum and maximum
     * values supplied.
     * @param minVal Minimum value that can be generated
     * @param maxVal Maximum value that can be generated
     * @return Random number between the minimum and maximum values
     */
    template <class T>
    static T GetRandomNumber(T minVal, T maxVal);

    /**
     * Get a random decimal number of the templated type between the minimum and
     * maximum values supplied.
     * @param minVal Minimum value that can be generated
     * @param maxVal Maximum value that can be generated
     * @return Random decimal number between the minimum and maximum values
     */
    template <class T>
    static T GetRandomDecimal(T minVal, T maxVal, uint8_t precision);
};

} // namspace libcomp

#define RNG(T, X, Y) libcomp::Randomizer::GetRandomNumber<T>(X, Y)
#define RNG_DEC(T, X, Y, P) libcomp::Randomizer::GetRandomDecimal<T>(X, Y, P)

#endif // LIBCOMP_SRC_RANDOMIZER_H