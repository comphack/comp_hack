/**
 * @file libcomp/src/DatabaseSQLite3.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to handle an SQLite3 database.
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

#include "DatabaseSQLite3.h"

// libcomp Includes
#include "DatabaseQuerySQLite3.h"
#include "Log.h"

// SQLite3 Includes
#include <sqlite3.h>

using namespace libcomp;

DatabaseSQLite3::DatabaseSQLite3(const std::shared_ptr<
    objects::DatabaseConfigSQLite3>& config) : mDatabase(nullptr), mConfig(config)
{
}

DatabaseSQLite3::~DatabaseSQLite3()
{
    Close();
}

bool DatabaseSQLite3::Open()
{
    auto filename = mConfig->GetDatabaseName();

    bool result = true;

    if(SQLITE_OK != sqlite3_open(filename.C(), &mDatabase))
    {
        result = false;

        LOG_ERROR(String("Failed to open database connection: %1\n").Arg(
            sqlite3_errmsg(mDatabase)));

        (void)Close();
    }

    return result;
}

bool DatabaseSQLite3::Close()
{
    bool result = true;

    if(nullptr != mDatabase)
    {
        if(SQLITE_OK != sqlite3_close(mDatabase))
        {
            result = false;

            LOG_ERROR("Failed to close database connection.\n");
        }

        mDatabase = nullptr;
    }

    return result;
}

bool DatabaseSQLite3::IsOpen() const
{
    return nullptr != mDatabase;
}

DatabaseQuery DatabaseSQLite3::Prepare(const String& query)
{
    return DatabaseQuery(new DatabaseQuerySQLite3(mDatabase), query);
}

bool DatabaseSQLite3::Exists()
{
    auto filename = mConfig->GetDatabaseName();

    // Check if the database file exists
    if(FILE *file = fopen(filename.C(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}

bool DatabaseSQLite3::Setup()
{
    if(!IsOpen())
    {
        LOG_ERROR("Trying to setup a database that is not open!\n");

        return false;
    }

    auto filename = mConfig->GetDatabaseName();
    if(!Exists())
    {
        if(UsingDefaultDatabaseFile() &&
            !Execute("CREATE TABLE objects ( uid uuid PRIMARY KEY, "
            "member_vars map<ascii, blob> );"))
        {
            LOG_ERROR("Failed to create the objects table.\n");

            return false;
        }
    }

    LOG_DEBUG(libcomp::String("Database connection established to '%1' file.\n")
        .Arg(filename));

    if(!VerifyAndSetupSchema())
    {
        LOG_ERROR("Schema verification and setup failed.\n");

        return false;
    }

    return true;
}

bool DatabaseSQLite3::Use()
{
    // Since each database is its own file there is nothing to do here.
    return true;
}


std::list<std::shared_ptr<PersistentObject>> DatabaseSQLite3::LoadObjects(
    std::type_index type, const std::string& fieldName, const libcomp::String& value)
{
    (void)type;
    (void)fieldName;
    (void)value;

    std::list<std::shared_ptr<PersistentObject>> retval;
    /// @todo
    return retval;
}

std::shared_ptr<PersistentObject> DatabaseSQLite3::LoadSingleObject(
    std::type_index type, const std::string& fieldName, const libcomp::String& value)
{
    (void)type;
    (void)fieldName;
    (void)value;

    /// @todo
    return nullptr;
}

bool DatabaseSQLite3::InsertSingleObject(std::shared_ptr<PersistentObject>& obj)
{
    (void)obj;

    /// @todo
    return false;
}

bool DatabaseSQLite3::UpdateSingleObject(std::shared_ptr<PersistentObject>& obj)
{
    (void)obj;

    /// @todo
    return false;
}

bool DatabaseSQLite3::DeleteSingleObject(std::shared_ptr<PersistentObject>& obj)
{
    (void)obj;

    /// @todo
    return false;
}

bool DatabaseSQLite3::VerifyAndSetupSchema()
{
    auto databaseName = mConfig->GetDatabaseName();
    std::vector<std::shared_ptr<libobjgen::MetaObject>> metaObjectTables;
    for(auto registrar : PersistentObject::GetRegistry())
    {
        std::string source = registrar.second->GetSourceLocation();
        if(source == databaseName || (source.length() == 0 && UsingDefaultDatabaseFile()))
        {
            metaObjectTables.push_back(registrar.second);
        }
    }

    /// @todo: finish

    return true;
}

bool DatabaseSQLite3::UsingDefaultDatabaseFile()
{
    return mConfig->GetDatabaseName() == mConfig->GetDefaultDatabaseName();
}
