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

/**
 * SQLite3 database specific implementation of a query with binding and
 * data retrieval functionality.
 */
class DatabaseQuerySQLite3 : public DatabaseQueryImpl
{
public:
    /**
     * Create a new SQLite3 database query.
     * @param pDatabase Pointer to the executing SQLite3 database
     */
    DatabaseQuerySQLite3(sqlite3 *pDatabase);

    /**
     * Clean up the query.
     */
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

    virtual bool GetValue(size_t index, String& value);
    virtual bool GetValue(const String& name, String& value);
    virtual bool GetValue(size_t index, std::vector<char>& value);
    virtual bool GetValue(const String& name, std::vector<char>& value);
    virtual bool GetValue(size_t index, libobjgen::UUID& value);
    virtual bool GetValue(const String& name, libobjgen::UUID& value);
    virtual bool GetValue(size_t index, int32_t& value);
    virtual bool GetValue(const String& name, int32_t& value);
    virtual bool GetValue(size_t index, int64_t& value);
    virtual bool GetValue(const String& name, int64_t& value);
    virtual bool GetValue(size_t index, float& value);
    virtual bool GetValue(const String& name, float& value);
    virtual bool GetValue(size_t index, double& value);
    virtual bool GetValue(const String& name, double& value);
    virtual bool GetValue(size_t index, bool& value);
    virtual bool GetValue(const String& name, bool& value);
    virtual bool GetMap(size_t index, std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool GetMap(const String& name, std::unordered_map<
        std::string, std::vector<char>> &values);
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    virtual bool BatchNext();

    virtual bool IsValid() const;

    /**
     * Get the current status of the query as a SQLite3 defined integer
     * status code.
     * @return SQLite3 defined integer status code
     */
    int GetStatus() const;

private:
    /**
     * Get the index of a named binding.
     * @param name Name of the binding
     * @return Index of the binding
     */
    size_t GetNamedBindingIndex(const String& name) const;

    /**
     * Helper function to format a named binding in the :NAME format.
     * @param name Name of the binding
     * @return Formatted binding
     */
    std::string GetNamedBinding(const String& name) const;

    /**
     * Get the index of the current result set's column by name.
     * @param name Name of the column
     * @param index Variable to store the index in
     * @return true on success, false on failure
     */
    bool GetResultColumnIndex(const String& name, size_t& index) const;

    /// Pointer to the SQLite3 database the query executes on
    sqlite3 *mDatabase;

    /// Pointer to the SQLite3 representation of the query as a statement
    sqlite3_stmt *mStatement;

    /// Current status of the query as a SQLite3 defined integer status
    /// code
    int mStatus;

    /// Indicator that Next() should be skipped the first time following
    /// execution to offset the need to call step (aka: Next()) to execute
    /// the query itself
    bool mDidJustExecute;

    /// Column names from the current result set
    std::vector<std::string> mResultColumnNames;

    /// Column data types from the current result set represented as SQLite3
    /// data type integers
    std::vector<int> mResultColumnTypes;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEQUERYSQLITE3_H
