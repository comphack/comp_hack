/**
 * @file libcomp/src/Randomizer.cpp
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

#include "Randomizer.h"
#include "Log.h"

// C++ Standard Includes
#include <cmath>
#include <random>

using namespace libcomp;

namespace libcomp
{
    template<>
    int32_t Randomizer::GetRandomNumber<int32_t>(int32_t minVal, int32_t maxVal)
    {
        // Keep one random number generator per thread
        static thread_local std::mt19937 gen;
        static thread_local uint32_t seed = 0;
        if(seed == 0)
        {
            // Use the system time for default seed
            seed = (uint32_t)(time(0) * rand());

            // Seed the high precision random number generator
            gen.seed(seed);
        }

        std::uniform_int_distribution<> dis(minVal, maxVal);
        return dis(gen);
    }

    template<>
    uint16_t Randomizer::GetRandomNumber<uint16_t>(uint16_t minVal, uint16_t maxVal)
    {
        int32_t r = GetRandomNumber<int32_t>((int32_t)minVal, (int32_t)maxVal);

        return (uint16_t)r;
    }

    template<>
    int16_t Randomizer::GetRandomNumber<int16_t>(int16_t minVal, int16_t maxVal)
    {
        int32_t r = GetRandomNumber<int32_t>((int32_t)minVal, (int32_t)maxVal);

        return (int16_t)r;
    }

    template<>
    uint8_t Randomizer::GetRandomNumber<uint8_t>(uint8_t minVal, uint8_t maxVal)
    {
        int32_t r = GetRandomNumber<int32_t>((int32_t)minVal, (int32_t)maxVal);

        return (uint8_t)r;
    }

    template<>
    float Randomizer::GetRandomDecimal<float>(float minVal, float maxVal, uint8_t precision)
    {
        int32_t p = precision > 0 ? (int32_t)pow(10, (int32_t)precision-1) : 0;

        int32_t r = GetRandomNumber<int32_t>((int32_t)(minVal * (float)p),
            (int32_t)(maxVal * (float)p));

        return (float)((double)r / (double)p);
    }
}