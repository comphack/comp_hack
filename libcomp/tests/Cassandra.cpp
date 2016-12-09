/**
 * @file libcomp/tests/Cassandra.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Cassandra database tests.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

// libcomp Includes
#include <DatabaseCassandra.h>

// Standard C++ Includes
#include <iostream>

using namespace libcomp;

TEST(Cassandra, Connection)
{
    DatabaseCassandra db("comp_hack_test");

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open("127.0.0.1"));
    EXPECT_TRUE(db.IsOpen());
    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());

    // std::cout << db.GetLastError().ToUtf8() << std::endl;
}

TEST(Cassandra, BadPrepare)
{
    DatabaseCassandra db("comp_hack_test");

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open("127.0.0.1"));
    EXPECT_TRUE(db.IsOpen());

    EXPECT_FALSE(db.Execute("SELECT"));

    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
}

TEST(Cassandra, ObjectBindIndex)
{
    uint32_t testValue = 0x12345678;
    std::vector<char> testValueData;
    testValueData.insert(testValueData.end(), (char*)&testValue,
        (char*)&(&testValue)[1]);

    String testValue2 = "今晩は！";
    std::vector<char> testValueData2;
    testValueData2.insert(testValueData2.end(), testValue2.C(),
        testValue2.C() + testValue2.Size());

    std::unordered_map<std::string, std::vector<char>> values;
    values["test_int"] = testValueData;
    values["test_string"] = testValueData2;

    int16_t testValue3 = -1337;
    std::vector<char> testValueData3;
    testValueData3.insert(testValueData3.end(), (char*)&testValue3,
        (char*)&(&testValue3)[1]);

    std::unordered_map<std::string, std::vector<char>> values2;
    values2["test_short"] = testValueData3;

    DatabaseCassandra db("comp_hack_test");

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open("127.0.0.1"));
    EXPECT_TRUE(db.IsOpen());

    EXPECT_TRUE(db.Execute("DROP KEYSPACE IF EXISTS comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE KEYSPACE comp_hack_test WITH REPLICATION = {"
        " 'class' : 'NetworkTopologyStrategy', 'datacenter1' : 1 };"));
    EXPECT_TRUE(db.Execute("USE comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE TABLE objects ( uid uuid PRIMARY KEY, "
        "member_vars map<ascii, blob> );"));

    DatabaseQuery q = db.Prepare("INSERT INTO objects ( uid, member_vars ) "
        "VALUES ( uuid(), ? );");
    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Bind(0, values));
    EXPECT_TRUE(q.Execute());
    EXPECT_TRUE(q.Bind(0, values2));
    EXPECT_TRUE(q.Execute());

    q = db.Prepare("SELECT uid, member_vars FROM objects;");
    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Execute());

    std::unordered_map<std::string, std::vector<char>> valuesRead;
    std::unordered_map<std::string, std::vector<char>> valuesRead2;

    for(int i = 0; i < 2; ++i)
    {
        std::unordered_map<std::string, std::vector<char>> theseValues;

        EXPECT_TRUE(q.Next());
        EXPECT_TRUE(q.GetMap(1, theseValues));

        if(values.size() == theseValues.size())
        {
            valuesRead = std::move(theseValues);
        }
        else
        {
            valuesRead2 = std::move(theseValues);
        }
    }

    EXPECT_FALSE(q.Next());

    EXPECT_EQ(valuesRead.size(), values.size());
    EXPECT_EQ(valuesRead.count("test_int"), values.count("test_int"));
    EXPECT_EQ(valuesRead.count("test_string"), values.count("test_string"));

    std::vector<char> readTestValueData = valuesRead.at("test_int");
    std::vector<char> readTestValueData2 = valuesRead.at("test_string");

    EXPECT_EQ(readTestValueData.size(), testValueData.size());
    EXPECT_EQ(readTestValueData2.size(), testValueData2.size());

    uint32_t readTestValue = 0;
    memcpy(&readTestValue, &readTestValueData[0], readTestValueData.size());

    String readTestValue2(&readTestValueData2[0], readTestValueData2.size());

    EXPECT_EQ(readTestValue, testValue);
    EXPECT_EQ(readTestValue2, testValue2);

    EXPECT_EQ(valuesRead2.size(), values2.size());
    EXPECT_EQ(valuesRead2.count("test_short"), values2.count("test_short"));

    std::vector<char> readTestValueData3 = valuesRead2.at("test_short");

    EXPECT_EQ(readTestValueData3.size(), testValueData3.size());

    int16_t readTestValue3 = 0;
    memcpy(&readTestValue3, &readTestValueData3[0], readTestValueData3.size());

    EXPECT_EQ(readTestValue3, testValue3);

    EXPECT_TRUE(db.Execute("DROP TABLE objects;"));

    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
}

TEST(Cassandra, ObjectBindName)
{
    uint32_t testValue = 0x12345678;
    std::vector<char> testValueData;
    testValueData.insert(testValueData.end(), (char*)&testValue,
        (char*)&(&testValue)[1]);

    String testValue2 = "今晩は！";
    std::vector<char> testValueData2;
    testValueData2.insert(testValueData2.end(), testValue2.C(),
        testValue2.C() + testValue2.Size());

    std::unordered_map<std::string, std::vector<char>> values;
    values["test_int"] = testValueData;
    values["test_string"] = testValueData2;

    int16_t testValue3 = -1337;
    std::vector<char> testValueData3;
    testValueData3.insert(testValueData3.end(), (char*)&testValue3,
        (char*)&(&testValue3)[1]);

    std::unordered_map<std::string, std::vector<char>> values2;
    values2["test_short"] = testValueData3;

    DatabaseCassandra db("comp_hack_test");

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open("127.0.0.1"));
    EXPECT_TRUE(db.IsOpen());

    EXPECT_TRUE(db.Execute("DROP KEYSPACE IF EXISTS comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE KEYSPACE comp_hack_test WITH REPLICATION = {"
        " 'class' : 'NetworkTopologyStrategy', 'datacenter1' : 1 };"));
    EXPECT_TRUE(db.Execute("USE comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE TABLE objects ( uid uuid PRIMARY KEY, "
        "member_vars map<ascii, blob> );"));

    DatabaseQuery q = db.Prepare("INSERT INTO objects ( uid, member_vars ) "
        "VALUES ( uuid(), ? );");
    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Bind("member_vars", values));
    EXPECT_TRUE(q.Execute());
    EXPECT_TRUE(q.Bind("member_vars", values2));
    EXPECT_TRUE(q.Execute());

    q = db.Prepare("SELECT uid, member_vars FROM objects;");
    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Execute());

    std::unordered_map<std::string, std::vector<char>> valuesRead;
    std::unordered_map<std::string, std::vector<char>> valuesRead2;

    for(int i = 0; i < 2; ++i)
    {
        std::unordered_map<std::string, std::vector<char>> theseValues;

        EXPECT_TRUE(q.Next());
        EXPECT_TRUE(q.GetMap("member_vars", theseValues));

        if(values.size() == theseValues.size())
        {
            valuesRead = std::move(theseValues);
        }
        else
        {
            valuesRead2 = std::move(theseValues);
        }
    }

    EXPECT_FALSE(q.Next());

    EXPECT_EQ(valuesRead.size(), values.size());
    EXPECT_EQ(valuesRead.count("test_int"), values.count("test_int"));
    EXPECT_EQ(valuesRead.count("test_string"), values.count("test_string"));

    std::vector<char> readTestValueData = valuesRead.at("test_int");
    std::vector<char> readTestValueData2 = valuesRead.at("test_string");

    EXPECT_EQ(readTestValueData.size(), testValueData.size());
    EXPECT_EQ(readTestValueData2.size(), testValueData2.size());

    uint32_t readTestValue = 0;
    memcpy(&readTestValue, &readTestValueData[0], readTestValueData.size());

    String readTestValue2(&readTestValueData2[0], readTestValueData2.size());

    EXPECT_EQ(readTestValue, testValue);
    EXPECT_EQ(readTestValue2, testValue2);

    EXPECT_EQ(valuesRead2.size(), values2.size());
    EXPECT_EQ(valuesRead2.count("test_short"), values2.count("test_short"));

    std::vector<char> readTestValueData3 = valuesRead2.at("test_short");

    EXPECT_EQ(readTestValueData3.size(), testValueData3.size());

    int16_t readTestValue3 = 0;
    memcpy(&readTestValue3, &readTestValueData3[0], readTestValueData3.size());

    EXPECT_EQ(readTestValue3, testValue3);

    EXPECT_TRUE(db.Execute("DROP TABLE objects;"));

    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
}

TEST(Cassandra, ObjectBatch)
{
    uint32_t testValue = 0x12345678;
    std::vector<char> testValueData;
    testValueData.insert(testValueData.end(), (char*)&testValue,
        (char*)&(&testValue)[1]);

    String testValue2 = "今晩は！";
    std::vector<char> testValueData2;
    testValueData2.insert(testValueData2.end(), testValue2.C(),
        testValue2.C() + testValue2.Size());

    std::unordered_map<std::string, std::vector<char>> values;
    values["test_int"] = testValueData;
    values["test_string"] = testValueData2;

    int16_t testValue3 = -1337;
    std::vector<char> testValueData3;
    testValueData3.insert(testValueData3.end(), (char*)&testValue3,
        (char*)&(&testValue3)[1]);

    std::unordered_map<std::string, std::vector<char>> values2;
    values2["test_short"] = testValueData3;

    DatabaseCassandra db("comp_hack_test");

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open("127.0.0.1"));
    EXPECT_TRUE(db.IsOpen());

    EXPECT_TRUE(db.Execute("DROP KEYSPACE IF EXISTS comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE KEYSPACE comp_hack_test WITH REPLICATION = {"
        " 'class' : 'NetworkTopologyStrategy', 'datacenter1' : 1 };"));
    EXPECT_TRUE(db.Execute("USE comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE TABLE objects ( uid uuid PRIMARY KEY, "
        "member_vars map<ascii, blob> );"));

    EXPECT_TRUE(db.Execute("INSERT INTO objects ( uid, member_vars ) "
        "VALUES ( 7261eed8-5f0e-441b-a115-6ccd418fba15, "
        "{ 'test_var' : 0xDEADBABE } );"));

    DatabaseQuery q = db.Prepare("DELETE FROM objects WHERE uid = "
        "7261eed8-5f0e-441b-a115-6ccd418fba15;");

    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.BatchNext());
    EXPECT_TRUE(q.Prepare("INSERT INTO objects ( uid, member_vars ) "
        "VALUES ( uuid(), ? );"));

    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Bind("member_vars", values));
    EXPECT_TRUE(q.BatchNext());
    EXPECT_TRUE(q.Bind("member_vars", values2));
    EXPECT_TRUE(q.Execute());

    q = db.Prepare("SELECT uid, member_vars FROM objects;");
    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Execute());

    std::unordered_map<std::string, std::vector<char>> valuesRead;
    std::unordered_map<std::string, std::vector<char>> valuesRead2;

    for(int i = 0; i < 2; ++i)
    {
        std::unordered_map<std::string, std::vector<char>> theseValues;

        EXPECT_TRUE(q.Next());
        EXPECT_TRUE(q.GetMap("member_vars", theseValues));

        if(values.size() == theseValues.size())
        {
            valuesRead = std::move(theseValues);
        }
        else
        {
            valuesRead2 = std::move(theseValues);
        }
    }

    EXPECT_FALSE(q.Next());

    EXPECT_EQ(valuesRead.size(), values.size());
    EXPECT_EQ(valuesRead.count("test_int"), values.count("test_int"));
    EXPECT_EQ(valuesRead.count("test_string"), values.count("test_string"));

    std::vector<char> readTestValueData = valuesRead.at("test_int");
    std::vector<char> readTestValueData2 = valuesRead.at("test_string");

    EXPECT_EQ(readTestValueData.size(), testValueData.size());
    EXPECT_EQ(readTestValueData2.size(), testValueData2.size());

    uint32_t readTestValue = 0;
    memcpy(&readTestValue, &readTestValueData[0], readTestValueData.size());

    String readTestValue2(&readTestValueData2[0], readTestValueData2.size());

    EXPECT_EQ(readTestValue, testValue);
    EXPECT_EQ(readTestValue2, testValue2);

    EXPECT_EQ(valuesRead2.size(), values2.size());
    EXPECT_EQ(valuesRead2.count("test_short"), values2.count("test_short"));

    std::vector<char> readTestValueData3 = valuesRead2.at("test_short");

    EXPECT_EQ(readTestValueData3.size(), testValueData3.size());

    int16_t readTestValue3 = 0;
    memcpy(&readTestValue3, &readTestValueData3[0], readTestValueData3.size());

    EXPECT_EQ(readTestValue3, testValue3);

    EXPECT_TRUE(db.Execute("DROP TABLE objects;"));

    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
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
