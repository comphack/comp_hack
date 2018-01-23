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

// libcomp Includes
#include "Decrypt.h"
#include "ScriptEngine.h"

using namespace libcomp;

thread_local std::mt19937 Randomizer::sGen;
thread_local std::mt19937_64 Randomizer::sGen64;

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<Randomizer>()
    {
        if(!BindingExists("Randomizer", true))
        {
            Sqrat::Class<Randomizer> binding(mVM, "Randomizer");
            binding
                .StaticFunc<int32_t(*)(int32_t, int32_t)>(
                    "RNG", &Randomizer::GetRandomNumber<int32_t>)
                .StaticFunc<int64_t(*)(int64_t, int64_t)>(
                    "RNG64", &Randomizer::GetRandomNumber64<int64_t>);

            Bind<Randomizer>("Randomizer", binding);
        }

        return *this;
    }
}

void Randomizer::SeedRNG()
{
    static thread_local uint64_t seed = 0;

    // Only seed if we haven't already
    if(seed == 0)
    {
        // Generate the default seed
        auto b = libcomp::Decrypt::GenerateRandom(8).Data();
        seed = (uint64_t)&b[0];

        // Seed the high precision random number generators
        sGen.seed((uint32_t)seed);
        sGen64.seed(seed);
    }
}

namespace libcomp
{
    template<>
    float Randomizer::GetRandomDecimal<float>(float minVal, float maxVal, uint8_t precision)
    {
        uint16_t p = precision > 0 ? (uint16_t)pow(10, (uint16_t)precision-1) : 0;

        int32_t r = GetRandomNumber<int32_t>((int32_t)(minVal * (float)p),
            (int32_t)(maxVal * (float)p));

        return (float)((double)r / (double)p);
    }

    template<>
    double Randomizer::GetRandomDecimal<double>(double minVal, double maxVal, uint8_t precision)
    {
        uint16_t p = precision > 0 ? (uint16_t)pow(10, (uint16_t)precision - 1) : 0;

        int64_t r = GetRandomNumber64<int64_t>((int64_t)(minVal * (double)p),
            (int64_t)(maxVal * (double)p));

        return (double)((double)r / (double)p);
    }
}