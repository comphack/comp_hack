/**
 * @file libobjgen/src/GeneratorSource.h
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief C++ source generator to write source code for an object.
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

#ifndef LIBOBJGEN_SRC_GENERATORSOURCE_H
#define LIBOBJGEN_SRC_GENERATORSOURCE_H

// libobjgen Includes
#include "Generator.h"

namespace libobjgen
{

class GeneratorSource : public Generator
{
public:
    virtual std::string Generate(const MetaObject& obj);

private:
    bool GeneratePersistentObjectFunctions(const MetaObject& obj, std::stringstream& ss);
    std::string GetBaseBooleanReturnValue(const MetaObject& obj, std::string function, std::string defaultValue = "true");
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_GENERATORSOURCE_H
