/**
 * @file libobjgen/tests/UUID.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test the UUID class against the Cassandra implementation.
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
#include <UUID.h>

using namespace libobjgen;

TEST(UUID, Null)
{
    EXPECT_TRUE(UUID().IsNull());
    EXPECT_TRUE(UUID("00000000-0000-0000-0000-000000000000").IsNull());

    EXPECT_FALSE(UUID("00000001-0000-0000-0000-000000000000").IsNull());
    EXPECT_FALSE(UUID("00000000-0000-0000-0000-000000000001").IsNull());
}

TEST(UUID, Generate)
{
    UUID a = UUID::Random();
    UUID b = UUID::Random();

    EXPECT_FALSE(a.IsNull());
    EXPECT_FALSE(b.IsNull());

    EXPECT_NE(a, b);
}

TEST(UUID, BinaryConversion)
{
    std::string uuidString = "e70ebdd0-7a79-4bff-9e1f-1d8c0a3a6fb6";

    unsigned char uuidData[] = {
        0xe7, 0x0e, 0xbd, 0xd0, 0x7a, 0x79, 0x4b, 0xff,
        0x9e, 0x1f, 0x1d, 0x8c, 0x0a, 0x3a, 0x6f, 0xb6,
    };

    UUID uuid(std::vector<char>((char*)uuidData,
        (char*)uuidData + sizeof(uuidData)));

    EXPECT_EQ(uuid.ToString(), uuidString);

    std::vector<char> uuidDataCopy = uuid.ToData();

    ASSERT_EQ(uuidDataCopy.size(), sizeof(uuidData));
    EXPECT_EQ(UUID(uuidDataCopy).ToString(), uuidString);
    EXPECT_EQ(memcmp(&uuidDataCopy[0], &uuidData[0], sizeof(uuidData)), 0);
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
