/**
 * @file libobjgen/tests/MetaVariable.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test the MetaVariable class.
 *
 * This file is part of the COMP_hack Object Generator Library (libobjgen).
 *
 * Copyright (C) 2014-2016 COMP_hack Team <compomega@tutanota.com>
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

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libobjgen Includes
#include <MetaVariable.h>

using namespace libobjgen;
#include <cmath>
#include <string>
#include <sstream>
#include <iostream>
TEST(MetaVariableNumeric, ToString)
{
    int8_t a = 127;
    int16_t b = -12345;
    float f;
/*"^[+-]?(nan|inf|([0-9]*[.])?[0-9]+([eE][-+]?[0-9]+)?)$"
"^([+-])?0x([0-9a-fA-F]+)$"
"^([+-])?0([0-7]+)$"
"^[+-]?[1-9][0-9]*$"*/
    std::stringstream ss("+inf");
    //ss >> std::setbase(16) >>  b;
    ss >> f;
    if(!ss)
    {
        std::cerr << "Bad conversion" << std::endl;
    }
    else
    {
        if(isnan(f))
        {
            std::cout << "ISNAN" << std::endl;
        }
        if(isinf(f))
        {
            std::cout << "IFINF" << std::endl;
        }
        std::cout << f << std::endl;
        std::cout << std::to_string(a) << ", " << std::to_string(b) << std::endl;
    }
    std::cout << NAN << ", " << INFINITY << std::endl;
}

int main(int argc, char *argv[])
{
    try
    {
        ::testing::InitGoogleTest(&argc, argv);

        return RUN_ALL_TESTS();
    }
    catch(...)
    {
        return EXIT_FAILURE;
    }
}
