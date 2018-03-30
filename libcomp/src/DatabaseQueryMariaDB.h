/**
 * @file libcomp/src/DatabaseQueryMariaDB.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief A MariaDB database query.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_DATABASEQUERYMARIADB_H
#define LIBCOMP_SRC_DATABASEQUERYMARIADB_H

// libcomp Includes
#include "DatabaseQuery.h"

typedef struct st_mysql MYSQL;
typedef struct st_mysql_bind MYSQL_BIND;
typedef struct st_mysql_stmt MYSQL_STMT;

namespace libcomp
{

class DatabaseMariaDB;

/**
 * MariaDB database specific implementation of a query with binding and
 * data retrieval functionality. The connector for MariaDB is the same
 * one used by MySQL which makes heavy use of buffer pointers for both
 * input binding and output retrieval.
 */
class DatabaseQueryMariaDB : public DatabaseQueryImpl
{
public:
    /**
     * Create a new MariaDB database query.
     * @param pDatabase Pointer to the executing MariaDB database
     * @param maxRetryCount Maximum number of retry attempts allowed
     *  when access to the DB during query execution returns as busy
     * @param retryDelay Delay in milliseconds between execution retry
     *  attempts
     */
    DatabaseQueryMariaDB(MYSQL *pDatabase);

    /**
     * Clean up the query.
     */
    virtual ~DatabaseQueryMariaDB();

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
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    virtual bool IsValid() const;

private:
    /**
     * Get the index of a named binding.
     * @param name Name of the binding
     * @return Index of the binding
     */
    size_t GetNamedBindingIndex(const String& name);

    /**
     * Get the index of the current result set's column by name.
     * @param name Name of the column
     * @param index Variable to store the index in
     * @return true on success, false on failure
     */
    bool GetResultColumnIndex(const String& name, size_t& index) const;

    /**
     * Create parameter bindings if they do not already exist and return
     * the binding at the specified index.
     * @param index Index to retrieve a parameter binding in the collection
     *  from
     * @param type Binding type to set after retrieval
     * @return Pointer to the parameter binding at the specified index
     */
    MYSQL_BIND* PrepareBinding(size_t index, int type);

    /// Pointer to the MariaDB database the query executes on
    MYSQL *mDatabase;

    /// Pointer to the MariaDB representation of the query as a statement
    MYSQL_STMT *mStatement;

    /// Bindings configured for all parameters passed into a query
    std::vector<MYSQL_BIND> mBindings;

    /// Bindings configured to contain all results returned by a query
    std::vector<MYSQL_BIND> mResultBindings;

    /// Current status of the query as a MariaDB defined integer status
    /// code
    int mStatus;

    /// Param names pulled from the prepared statement to bind to
    std::vector<std::string> mParamNames;

    /// Column names from the current result set
    std::vector<std::string> mResultColumnNames;

    /// Column data types from the current result set represented as MariaDB
    /// data type integers
    std::vector<int> mResultColumnTypes;

    /// Buffer containing 32-bit int values for bound and selected values
    std::list<int32_t> mBufferInt;

    /// Buffer containing 64-bit int values for bound and selected values
    std::list<int64_t> mBufferBigInt;

    /// Buffer containing float values for bound and selected values
    std::list<float> mBufferFloat;

    /// Buffer containing double values for bound and selected values
    std::list<double> mBufferDouble;

    /// Buffer containing blob or string values for bound and selected values
    std::list<std::vector<char>> mBufferBlob;

    /// Buffer containing boolean values for bound and selected values
    std::list<bool> mBufferBool;

    /// Buffer containing null specifiers for bound and selected values
    std::list<char> mBufferNulls;

    /// Buffer containing length values for bound and selected values
    std::list<unsigned long> mBufferLengths;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEQUERYMARIADB_H
