/**
 * @file libcomp/src/DatabaseQuerySQLite3.h
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

#ifndef LIBCOMP_SRC_DATABASEQUERYSQLITE3_H
#define LIBCOMP_SRC_DATABASEQUERYSQLITE3_H

// libcomp Includes
#include "DatabaseQuery.h"

// sqlite3 Includes
#include <sqlite3.h>

namespace libcomp
{

class DatabaseSQLite3;

class DatabaseQuerySQLite3 : public DatabaseQueryImpl
{
public:
    DatabaseQuerySQLite3(sqlite3 *pDatabase);
    virtual ~DatabaseQuerySQLite3();

    virtual bool Prepare(const String& query);
    virtual bool Execute();
    virtual bool Next();

    virtual bool Bind(size_t index, const String& value);
    virtual bool Bind(const String& name, const String& value);
    virtual bool Bind(size_t index, const std::vector<char>& value);
    virtual bool Bind(const String& name, const std::vector<char>& value);
    virtual bool Bind(size_t index, const libobjgen::UUID& value);
    virtual bool Bind(const String& name, const libobjgen::UUID& value);
    virtual bool Bind(size_t index, int32_t value);
    virtual bool Bind(const String& name, int32_t value);
    virtual bool Bind(size_t index, int64_t value);
    virtual bool Bind(const String& name, int64_t value);
    virtual bool Bind(size_t index, float value);
    virtual bool Bind(const String& name, float value);
    virtual bool Bind(size_t index, double value);
    virtual bool Bind(const String& name, double value);
    virtual bool Bind(size_t index, bool value);
    virtual bool Bind(const String& name, bool value);
    virtual bool Bind(size_t index, const std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool Bind(const String& name, const std::unordered_map<
        std::string, std::vector<char>>& values);

    virtual bool GetMap(size_t index, std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool GetMap(const String& name, std::unordered_map<
        std::string, std::vector<char>> &values);
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    virtual bool BatchNext();

    virtual bool IsValid() const;

    int GetStatus() const;

private:
    std::string GetNamedBinding(const String& name) const;

    sqlite3 *mDatabase;

    sqlite3_stmt *mStatement;

    int mStatus;

    // SQLite3 must call step (aka: Next) to execute so skip the first call
    // to it after execution
    bool mDidJustExecute;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEQUERYSQLITE3_H
