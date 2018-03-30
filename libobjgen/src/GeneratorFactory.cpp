/**
 * @file libobjgen/src/GeneratorFactory.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Factory to create an object code generator.
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

#include "GeneratorFactory.h"

#include "GeneratorHeader.h"
#include "GeneratorSource.h"

using namespace libobjgen;

GeneratorFactory::GeneratorFactory()
{
    mGenerators["cpp"] = []() {
        return std::shared_ptr<libobjgen::Generator>(new GeneratorSource);
    };
    mGenerators["h"] = []() {
        return std::shared_ptr<libobjgen::Generator>(new GeneratorHeader);
    };
}

std::shared_ptr<libobjgen::Generator> GeneratorFactory::Generator(
    const std::string& extension) const
{
    auto generatorPair = mGenerators.find(extension);

    if(generatorPair != mGenerators.end())
    {
        return (*generatorPair->second)();
    }
    else
    {
        return std::shared_ptr<libobjgen::Generator>();
    }
}
