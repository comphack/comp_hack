/**
 * @file libcomp/tests/MariaDB.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief MariaDB database tests.
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
#include <Account.h>
#include <DatabaseMariaDB.h>

using namespace libcomp;

class MariaDBAccount : public objects::Account
{
public:
    MariaDBAccount()
    {
    }

    static void RegisterPersistentType()
    {
        RegisterType(typeid(MariaDBAccount),
            MariaDBAccount::GetMetadata(), []()
        { 
            return (PersistentObject*)new MariaDBAccount();
        });
    }

    libcomp::String text;
};

std::shared_ptr<objects::DatabaseConfigMariaDB> GetConfig()
{
    auto config = std::shared_ptr<objects::DatabaseConfigMariaDB>(
        new objects::DatabaseConfigMariaDB);
    config->SetDatabaseName("comp_hack_test");
    config->SetUsername("testuser");
    config->SetPassword("un1tt3st");
    return config;
}

TEST(MariaDB, Connection)
{
    auto config = GetConfig();

    DatabaseMariaDB db(config);

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open());
    EXPECT_TRUE(db.IsOpen());
    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
}

TEST(MariaDB, BadPrepare)
{
    auto config = GetConfig();

    DatabaseMariaDB db(config);

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open());
    EXPECT_TRUE(db.IsOpen());

    EXPECT_FALSE(db.Execute("SELECT"));

    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
}

TEST(MariaDB, ObjectBindIndex)
{
    libobjgen::UUID uuid1 = libobjgen::UUID::Random();

    int32_t sort1 = 1;

    uint32_t testValue = 0x12345678;
    std::vector<char> testValueData;
    testValueData.insert(testValueData.end(), (char*)&testValue,
        (char*)&(&testValue)[1]);

    String testString = "今日は！";

    libobjgen::UUID uuid2 = libobjgen::UUID::Random();

    int32_t sort2 = 2;

    uint32_t testValue2 = 0x87654321;
    std::vector<char> testValueData2;
    testValueData.insert(testValueData.end(), (char*)&testValue2,
        (char*)&(&testValue2)[1]);

    String testString2 = "今晩は！";

    auto config = GetConfig();

    DatabaseMariaDB db(config);

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open());
    EXPECT_TRUE(db.IsOpen());

    EXPECT_TRUE(db.Execute("DROP DATABASE IF EXISTS comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE DATABASE comp_hack_test;"));
    EXPECT_TRUE(db.Use());
    EXPECT_TRUE(db.Execute("CREATE TABLE objects ( uid VARCHAR(36) PRIMARY KEY, "
        "sortby int, data blob, txt text );"));

    DatabaseQuery q = db.Prepare("INSERT INTO objects ( uid, sortby, data, txt ) "
        "VALUES ( ?, ?, ?, ? );");
    EXPECT_TRUE(q.IsValid());

    EXPECT_TRUE(q.Bind(0, uuid1));
    EXPECT_TRUE(q.Bind(1, sort1));
    EXPECT_TRUE(q.Bind(2, testValueData));
    EXPECT_TRUE(q.Bind(3, testString));
    EXPECT_TRUE(q.Execute());

    EXPECT_TRUE(q.Bind(0, uuid2));
    EXPECT_TRUE(q.Bind(1, sort2));
    EXPECT_TRUE(q.Bind(2, testValueData2));
    EXPECT_TRUE(q.Bind(3, testString2));
    EXPECT_TRUE(q.Execute());

    q = db.Prepare("SELECT uid, sortby, data, txt FROM objects ORDER BY sortby ASC;");
    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Execute());

    libobjgen::UUID outUuid;
    int32_t outSort = 0;
    std::vector<char> outData;
    String outString;

    // Compare first row
    EXPECT_NE(outUuid, uuid1);
    EXPECT_NE(outSort, sort1);
    EXPECT_NE(outData, testValueData);
    EXPECT_NE(outString, testString);

    EXPECT_TRUE(q.Next());

    EXPECT_TRUE(q.GetValue(0, outUuid));
    EXPECT_TRUE(q.GetValue(1, outSort));
    EXPECT_TRUE(q.GetValue(2, outData));
    EXPECT_TRUE(q.GetValue(3, outString));

    EXPECT_EQ(outUuid, uuid1);
    EXPECT_EQ(outSort, sort1);
    EXPECT_EQ(outData, testValueData);
    EXPECT_EQ(outString, testString);

    // Compare second row
    EXPECT_NE(outUuid, uuid2);
    EXPECT_NE(outSort, sort2);
    EXPECT_NE(outData, testValueData2);
    EXPECT_NE(outString, testString2);

    EXPECT_TRUE(q.Next());

    EXPECT_TRUE(q.GetValue(0, outUuid));
    EXPECT_TRUE(q.GetValue(1, outSort));
    EXPECT_TRUE(q.GetValue(2, outData));
    EXPECT_TRUE(q.GetValue(3, outString));

    EXPECT_EQ(outUuid, uuid2);
    EXPECT_EQ(outSort, sort2);
    EXPECT_EQ(outData, testValueData2);
    EXPECT_EQ(outString, testString2);

    EXPECT_FALSE(q.Next());

    EXPECT_TRUE(db.Execute("DROP TABLE objects;"));

    EXPECT_TRUE(db.Execute("DROP DATABASE IF EXISTS comp_hack_test;"));

    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
}

TEST(MariaDB, ObjectBindName)
{
    libobjgen::UUID uuid1 = libobjgen::UUID::Random();

    int32_t sort1 = 1;

    uint32_t testValue = 0x12345678;
    std::vector<char> testValueData;
    testValueData.insert(testValueData.end(), (char*)&testValue,
        (char*)&(&testValue)[1]);

    String testString = "今日は！";

    libobjgen::UUID uuid2 = libobjgen::UUID::Random();

    int32_t sort2 = 2;

    uint32_t testValue2 = 0x87654321;
    std::vector<char> testValueData2;
    testValueData.insert(testValueData.end(), (char*)&testValue2,
        (char*)&(&testValue2)[1]);

    String testString2 = "今晩は！";

    auto config = GetConfig();

    DatabaseMariaDB db(config);

    EXPECT_FALSE(db.IsOpen());
    EXPECT_TRUE(db.Open());
    EXPECT_TRUE(db.IsOpen());

    EXPECT_TRUE(db.Execute("DROP DATABASE IF EXISTS comp_hack_test;"));
    EXPECT_TRUE(db.Execute("CREATE DATABASE comp_hack_test;"));
    EXPECT_TRUE(db.Use());
    EXPECT_TRUE(db.Execute("CREATE TABLE objects ( uid VARCHAR(36) PRIMARY KEY, "
        "sortby int, data blob, txt text );"));

    DatabaseQuery q = db.Prepare("INSERT INTO objects ( uid, sortby, data, txt ) "
        "VALUES ( :uid, :sortby, :data, :txt );");
    EXPECT_TRUE(q.IsValid());

    EXPECT_TRUE(q.Bind("uid", uuid1));
    EXPECT_TRUE(q.Bind("sortby", sort1));
    EXPECT_TRUE(q.Bind("data", testValueData));
    EXPECT_TRUE(q.Bind("txt", testString));
    EXPECT_TRUE(q.Execute());

    EXPECT_TRUE(q.Bind("uid", uuid2));
    EXPECT_TRUE(q.Bind("sortby", sort2));
    EXPECT_TRUE(q.Bind("data", testValueData2));
    EXPECT_TRUE(q.Bind("txt", testString2));
    EXPECT_TRUE(q.Execute());

    q = db.Prepare("SELECT uid, sortby, data, txt FROM objects ORDER BY sortby ASC;");
    EXPECT_TRUE(q.IsValid());
    EXPECT_TRUE(q.Execute());

    libobjgen::UUID outUuid;
    int32_t outSort = 0;
    std::vector<char> outData;
    String outString;

    // Compare first row
    EXPECT_NE(outUuid, uuid1);
    EXPECT_NE(outSort, sort1);
    EXPECT_NE(outData, testValueData);
    EXPECT_NE(outString, testString);

    EXPECT_TRUE(q.Next());

    EXPECT_TRUE(q.GetValue("uid", outUuid));
    EXPECT_TRUE(q.GetValue("sortby", outSort));
    EXPECT_TRUE(q.GetValue("data", outData));
    EXPECT_TRUE(q.GetValue("txt", outString));

    EXPECT_EQ(outUuid, uuid1);
    EXPECT_EQ(outSort, sort1);
    EXPECT_EQ(outData, testValueData);
    EXPECT_EQ(outString, testString);

    // Compare second row
    EXPECT_NE(outUuid, uuid2);
    EXPECT_NE(outSort, sort2);
    EXPECT_NE(outData, testValueData2);
    EXPECT_NE(outString, testString2);

    EXPECT_TRUE(q.Next());

    EXPECT_TRUE(q.GetValue("uid", outUuid));
    EXPECT_TRUE(q.GetValue("sortby", outSort));
    EXPECT_TRUE(q.GetValue("data", outData));
    EXPECT_TRUE(q.GetValue("txt", outString));

    EXPECT_EQ(outUuid, uuid2);
    EXPECT_EQ(outSort, sort2);
    EXPECT_EQ(outData, testValueData2);
    EXPECT_EQ(outString, testString2);

    EXPECT_FALSE(q.Next());

    EXPECT_TRUE(db.Execute("DROP TABLE objects;"));

    EXPECT_TRUE(db.Execute("DROP DATABASE IF EXISTS comp_hack_test;"));

    EXPECT_TRUE(db.Close());
    EXPECT_FALSE(db.IsOpen());
}

TEST(MariaDB, ChangeSet)
{
    auto config = GetConfig();
    MariaDBAccount::RegisterPersistentType();

    DatabaseMariaDB db(config);

    EXPECT_TRUE(db.Open());
    EXPECT_TRUE(db.Setup());

    auto account = std::make_shared<MariaDBAccount>();
    account->Register(account);
    account->SetCP(0);

    auto changeset = libcomp::DatabaseChangeSet::Create();
    changeset->Insert(account);

    EXPECT_TRUE(db.ProcessChangeSet(changeset));

    EXPECT_EQ(account->GetCP(), 0);

    auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();
    opChangeset->Update(account);

    auto expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
    EXPECT_TRUE(expl->Set<int64_t>("CP", 1000));
    opChangeset->AddOperation(expl);

    expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
    EXPECT_TRUE(expl->AddFrom<int64_t>("CP", 5, 1000));
    opChangeset->AddOperation(expl);

    expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
    EXPECT_TRUE(expl->SubtractFrom<int64_t>("CP", 10, 1005));
    opChangeset->AddOperation(expl);

    // Sanity check
    EXPECT_EQ(account->GetCP(), 0);

    EXPECT_TRUE(db.ProcessChangeSet(opChangeset));

    EXPECT_EQ(account->GetCP(), 995);

    EXPECT_TRUE(db.Execute("DROP DATABASE IF EXISTS comp_hack_test;"));

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
