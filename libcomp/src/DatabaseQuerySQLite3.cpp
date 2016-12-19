/**
 * @file libcomp/src/DatabaseQuerySQLite3.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief A SQLite3 database query.
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

#include "DatabaseQuerySQLite3.h"
#include "DatabaseSQLite3.h"

 // SQLite3 Includes
#include <sqlite3.h>

using namespace libcomp;

DatabaseQuerySQLite3::DatabaseQuerySQLite3(sqlite3 *pDatabase)
    : mDatabase(pDatabase), mStatement(nullptr), mStatus(SQLITE_OK)
{
}

DatabaseQuerySQLite3::~DatabaseQuerySQLite3()
{
    if(nullptr != mStatement)
    {
        sqlite3_finalize(mStatement);
    }
}

bool DatabaseQuerySQLite3::Prepare(const String& query)
{
    mStatus = sqlite3_prepare_v2(mDatabase, query.C(),
        (int)query.Length(), &mStatement, nullptr);

    return IsValid();
}

bool DatabaseQuerySQLite3::Execute()
{
    if(!IsValid())
    {
        return false;
    }

    mStatus = sqlite3_step(mStatement);

    return IsValid();
}

bool DatabaseQuerySQLite3::Next()
{
    mStatus = sqlite3_step(mStatement);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(size_t index, const String& value)
{
    mStatus = sqlite3_bind_text(mStatement, (int)index,
        value.C(), (int)value.Length(), 0);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name, const String& value)
{
    auto binding = GetNamedBinding(name);
    size_t index = (size_t)sqlite3_bind_parameter_index(mStatement, binding.c_str());

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, const std::vector<char>& value)
{
    if(nullptr != mStatement)
    {
        mStatus = sqlite3_bind_blob(mStatement, index, &value[0],
            (int)value.size(), 0);
    }
    else
    {
        return false;
    }

    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name,
    const std::vector<char>& value)
{
    auto binding = GetNamedBinding(name);
    size_t index = (size_t)sqlite3_bind_parameter_index(mStatement, binding.c_str());

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, const libobjgen::UUID& value)
{
    (void)index;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name,
    const libobjgen::UUID& value)
{
    (void)name;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(size_t index, int32_t value)
{
    (void)index;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name, int32_t value)
{
    (void)name;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(size_t index, int64_t value)
{
    (void)index;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name, int64_t value)
{
    (void)name;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(size_t index, float value)
{
    (void)index;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name, float value)
{
    (void)name;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(size_t index, double value)
{
    (void)index;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name, double value)
{
    (void)name;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(size_t index, bool value)
{
    (void)index;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name, bool value)
{
    (void)name;
    (void)value;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(size_t index, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    (void)index;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    (void)name;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::GetMap(size_t index,
    std::unordered_map<std::string, std::vector<char>>& values)
{
    (void)index;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::GetMap(const String& name,
    std::unordered_map<std::string, std::vector<char>>& values)
{
    (void)name;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::GetRows(std::list<std::unordered_map<
    std::string, std::vector<char>> >& rows)
{
    int colCount = sqlite3_column_count(mStatement);

    if(colCount == 0)
    {
        mStatus = SQLITE_ERROR;
        return false;
    }

    std::vector<std::string> colNames;
    std::vector<int> colTypes;
    for(int i = 0; i < colCount; i++)
    {
        colNames.push_back(std::string(sqlite3_column_name(mStatement, i)));
        colTypes.push_back(sqlite3_column_type(mStatement, i));
    }

    while(SQLITE_ROW == mStatus)
    {
        std::unordered_map<std::string, std::vector<char>> m;
        for(int i = 0; i < colCount; i++)
        {
            std::string colName = colNames[(size_t)i];
            int colType = colTypes[(size_t)i];

            std::vector<char> value;

            size_t bytes = (size_t)sqlite3_column_bytes(mStatement, i);
            switch(colType)
            {
                case SQLITE_INTEGER:
                    {
                        int val = sqlite3_column_int(mStatement, i);
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(&val),
                            reinterpret_cast<const char*>(&val) +
                            bytes);
                    }
                    break;
                case SQLITE_FLOAT:
                    {
                        double val = sqlite3_column_double(mStatement, i);
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(&val),
                            reinterpret_cast<const char*>(&val) +
                            bytes);
                    }
                    break;
                case SQLITE_BLOB:
                    {
                        auto val = sqlite3_column_blob(mStatement, i);
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(&val),
                            reinterpret_cast<const char*>(&val) +
                            bytes);
                    }
                    break;
                case SQLITE_TEXT:
                    {
                        auto val = sqlite3_column_text(mStatement, i);
                        value.insert(value.begin(), val, val + bytes);
                    }
                    break;
                case SQLITE_NULL:
                    //Nothing to add
                    break;
                default:
                    mStatus = SQLITE_ERROR;
                    break;
            }

            m[colName] = value;
        }
        rows.push_back(m);

        //Make sure the status has not been updated
        if(SQLITE_ROW == mStatus)
        {
            Next();
        }
    }

    return IsValid();
}

bool DatabaseQuerySQLite3::BatchNext()
{
    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::IsValid() const
{
    return nullptr != mDatabase && nullptr != mStatement &&
        (SQLITE_OK == mStatus || SQLITE_ROW == mStatus ||
        SQLITE_DONE == mStatus);
}

int DatabaseQuerySQLite3::GetStatus() const
{
    return mStatus;
}

std::string DatabaseQuerySQLite3::GetNamedBinding(const String& name) const
{
    return libcomp::String(":%1").Arg(name).ToUtf8();
}
