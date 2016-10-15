/**
 * @file libcomp/tests/VectkrSfrea.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tests of the VectorStream class.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2016 COMP_hack Team <compomega@tutanota.com>
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

#include <VectorStream.h>

using namespace libcomp;


TEST(VectorStream, ReadWrite)
{
    uint32_t valueA = 0xCAFEBABE;

    std::vector<char> data;
    VectorStream<char> buffer(data);

    std::ostream out(&buffer);
    out.write(reinterpret_cast<char*>(&valueA), sizeof(valueA));

    EXPECT_TRUE(out.good());
    ASSERT_EQ(data.size(), sizeof(valueA));
    EXPECT_EQ(memcmp(&valueA, &data[0], sizeof(valueA)), 0);

    std::vector<char> data2;
    data2.resize(sizeof(valueA));
    memcpy(&data2[0], &valueA, sizeof(valueA));
    VectorStream<char> buffer2(data2);

    std::istream in(&buffer2);
    uint32_t valueB;
    in.read(reinterpret_cast<char*>(&valueB), sizeof(valueB));

    EXPECT_TRUE(in.good());
    EXPECT_EQ(valueA, valueB);
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
