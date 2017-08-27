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

// C++ Standard Includes
#include <cmath>
#include <random>
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
     * Get a random integer number of the templated type between the minimum
     * and maximum values supplied. Use GetRandomNumber64 for types uint64_t
     * and int64_t.
     * @param minVal Minimum value that can be generated
     * @param maxVal Maximum value that can be generated
     * @return Random number between the minimum and maximum values
     */
    template <class T>
    static T GetRandomNumber(T minVal, T maxVal)
    {
        SeedRNG();

        std::uniform_int_distribution<T> dis(minVal, maxVal);
        return dis(sGen);
    }

    /**
     * Get a random 64 bit integer number of the templated type between the
     * minimum and maximum values supplied. Use GetRandomNumber instead for
     * non-64 bit integers as this is significantly less performant.
     * @param minVal Minimum value that can be generated
     * @param maxVal Maximum value that can be generated
     * @return Random number between the minimum and maximum values
     */
    template <class T>
    static T GetRandomNumber64(T minVal, T maxVal)
    {
        SeedRNG();

        std::uniform_int_distribution<T> dis(minVal, maxVal);
        return dis(sGen64);
    }

    /**
     * Get a random decimal number of the templated type between the minimum and
     * maximum values supplied.
     * @param minVal Minimum value that can be generated
     * @param maxVal Maximum value that can be generated
     * @return Random decimal number between the minimum and maximum values
     */
    template <class T>
    static T GetRandomDecimal(T minVal, T maxVal, uint8_t precision);

private:
    /**
     * Generate the random number seed value for the 32-bit and 64-bit
     * generators if it has not been already.
     */
    static void SeedRNG();

    /// Thread specific 32-bit random number generator
    static thread_local std::mt19937 sGen;

    /// Thread specific 65-bit random number generator
    static thread_local std::mt19937_64 sGen64;
};

} // namspace libcomp

#define RNG(T, X, Y) libcomp::Randomizer::GetRandomNumber<T>(X, Y)
#define RNG64(T, X, Y) libcomp::Randomizer::GetRandomNumber64<T>(X, Y)
#define RNG_DEC(T, X, Y, P) libcomp::Randomizer::GetRandomDecimal<T>(X, Y, P)

#endif // LIBCOMP_SRC_RANDOMIZER_H