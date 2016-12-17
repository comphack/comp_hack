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
    : mDatabase(pDatabase), mStatement(nullptr)
{
}

DatabaseQuerySQLite3::~DatabaseQuerySQLite3()
{
    /// @todo: does anything need to be freed up?
}

bool DatabaseQuerySQLite3::Prepare(const String& query)
{
    const char *pzTail;

    auto status = sqlite3_prepare(mDatabase, query.C(),
        (int)query.Length(), &mStatement, &pzTail);

    if(SQLITE_OK == status)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool DatabaseQuerySQLite3::Execute()
{
    if(SQLITE_OK != sqlite3_step(mStatement))
    {
        return false;
    }

    return SQLITE_OK == sqlite3_finalize(mStatement);
}

bool DatabaseQuerySQLite3::Next()
{
    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(size_t index, const String& value)
{
    /// @todo: verify and test
    return SQLITE_OK == sqlite3_bind_text(mStatement, (int)index,
        value.C(), (int)value.Length(), 0);
}

bool DatabaseQuerySQLite3::Bind(const String& name, const String& value)
{
    /// @todo: verify and test
    size_t index = (size_t)sqlite3_bind_parameter_index(mStatement, name.C());

    if(index == 0)
    {
        return false;
    }

    return Bind(index, value);
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
    (void)rows;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::BatchNext()
{
    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::IsValid() const
{
    /// @todo
    return false;
}
