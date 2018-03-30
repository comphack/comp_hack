/**
 * @file libcomp/src/DatabaseQuery.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class to handle a database query.
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

#ifndef LIBCOMP_SRC_DATABASEQUERY_H
#define LIBCOMP_SRC_DATABASEQUERY_H

// libobjgen Includes
#include "UUID.h"

// libcomp Includes
#include "CString.h"

// Standard C++11 Includes
#include <unordered_map>

namespace libcomp
{

/**
 * Abstract base class to be implemented by specific database types to
 * facilitate column binding and data retrieval.
 * @sa DatabaseQuery
 */
class DatabaseQueryImpl
{
public:
    /**
     * Create the query implementation.
     */
    DatabaseQueryImpl();

    /**
     * Clean up the query implementation.
     */
    virtual ~DatabaseQueryImpl();

    /**
     * Prepare a database query for execution based upon query text.
     * @sa DatabaseQueryImpl::Execute
     * @param query Query text to prepare
     * @return true on success, false on failure
     */
    virtual bool Prepare(const String& query) = 0;

    /**
     * Execute a previously prepared query.
     * @sa DatabaseQueryImpl::Prepare
     * @return true on success, false on failure
     */
    virtual bool Execute() = 0;

    /**
     * Increment the query results to look at the next result set row.
     * @return true on success, false on failure
     */
    virtual bool Next() = 0;

    /**
     * Bind a string column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, const String& value) = 0;

    /**
     * Bind a string column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, const String& value) = 0;

    /**
     * Bind a blob column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, const std::vector<char>& value) = 0;

    /**
     * Bind a blob column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, const std::vector<char>& value) = 0;

    /**
     * Bind a UUID column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, const libobjgen::UUID& value) = 0;

    /**
     * Bind a UUID column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, const libobjgen::UUID& value) = 0;

    /**
     * Bind a 32-bit integer column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, int32_t value) = 0;

    /**
     * Bind a 32-bit integer column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, int32_t value) = 0;

    /**
     * Bind a 64-bit integer column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, int64_t value) = 0;

    /**
     * Bind a 64-bit integer column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, int64_t value) = 0;

    /**
     * Bind a float column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, float value) = 0;

    /**
     * Bind a float column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, float value) = 0;

    /**
     * Bind a double column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, double value) = 0;

    /**
     * Bind a double column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, double value) = 0;

    /**
     * Bind a boolean column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, bool value) = 0;

    /**
     * Bind a boolean column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, bool value) = 0;

    /**
     * Bind a map column value by its index.
     * @param index The column's index
     * @param values The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(size_t index, const std::unordered_map<
        std::string, std::vector<char>>& values);

    /**
     * Bind a map column value by its name.
     * @param name The column's name
     * @param values The value to bind
     * @return true on success, false on failure
     */
    virtual bool Bind(const String& name, const std::unordered_map<
        std::string, std::vector<char>>& values);

    /**
     * Get a string column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, String& value) = 0;

    /**
     * Get a string column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, String& value) = 0;

    /**
     * Get a blob column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, std::vector<char>& value) = 0;

    /**
     * Get a blob column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, std::vector<char>& value) = 0;

    /**
     * Get a UUID column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, libobjgen::UUID& value) = 0;

    /**
     * Get a UUID column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, libobjgen::UUID& value) = 0;

    /**
     * Get a 32-bit integer column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, int32_t& value) = 0;

    /**
     * Get a 32-bit integer column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, int32_t& value) = 0;

    /**
     * Get a 64-bit integer column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, int64_t& value) = 0;

    /**
     * Get a boolean column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, int64_t& value) = 0;

    /**
     * Get a float column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, float& value) = 0;

    /**
     * Get a float column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, float& value) = 0;

    /**
     * Get a double column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, double& value) = 0;

    /**
     * Get a double column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, double& value) = 0;

    /**
     * Get a boolean column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(size_t index, bool& value) = 0;

    /**
     * Get a boolean column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    virtual bool GetValue(const String& name, bool& value) = 0;

    /**
     * Get all of the query result's rows as bytes mapped
     * by column name.
     * @param rows Output map to insert a row with each
     *  column's bytes mapped by column name
     * @return true on success, false on failure
     */
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    /**
     * Get the count of affected rows from the last query
     * execution.
     * @return Number of affected rows
     */
    int64_t AffectedRowCount() const;

    /**
     * Check current query state validity.
     * @return true on valid, false on invalid
     */
    virtual bool IsValid() const = 0;

protected:
    /// Represents the number of affected rows since the
    /// last sucessful call to Execute
    int64_t mAffectedRowCount;
};

/**
 * Database query wrapper class that contains a database specific
 * query implementation so the pointer does not need to be worked
 * with directly.  Nearly all functions have a corresponding
 * @ref DatabaseQueryImpl function as well that does most of the
 * actual work.
 */
class DatabaseQuery
{
public:
    /**
     * Create a new database query.
     * @param pImpl Pointer to a database specific query implementation
     */
    DatabaseQuery(DatabaseQueryImpl *pImpl);

    /**
     * Create and prepare a new database query.
     * @param pImpl Pointer to a database specific query implementation
     * @param query Query text to prepare
     */
    DatabaseQuery(DatabaseQueryImpl *pImpl, const String& query);

    /**
     * Disable the default copy constructor.
     * @param other The other query to copy
     */
    DatabaseQuery(const DatabaseQuery& other) = delete;

    /**
     * Copy another database query.
     * @param other The other query to copy
     */
    DatabaseQuery(DatabaseQuery&& other);

    /**
     * Clean up the query and delete the implementation pointer.
     */
    ~DatabaseQuery();

    /**
     * Prepare a database query implementation for execution based
     * upon query text.
     * @sa DatabaseQuery::Execute
     * @param query Query text to prepare
     * @return true on success, false on failure
     */
    bool Prepare(const String& query);

    /**
     * Execute a previously prepared query implementation.
     * @sa DatabaseQuery::Prepare
     * @return true on success, false on failure
     */
    bool Execute();

    /**
     * Increment the query implementation's results to look at the next
     * result set row.
     * @return true on success, false on failure
     */
    bool Next();

    /**
     * Bind an implementation's string column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, const String& value);

    /**
     * Bind an implementation's string column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, const String& value);

    /**
     * Bind an implementation's blob column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, const std::vector<char>& value);

    /**
     * Bind an implementation's blob column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, const std::vector<char>& value);

    /**
     * Bind an implementation's UUID column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, const libobjgen::UUID& value);

    /**
     * Bind an implementation's UUID column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, const libobjgen::UUID& value);

    /**
     * Bind an implementation's 32-bit integer column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, int32_t value);

    /**
     * Bind an implementation's 32-bit integer column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, int32_t value);

    /**
     * Bind an implementation's 64-bit integer column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, int64_t value);

    /**
     * Bind an implementation's 64-bit integer column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, int64_t value);

    /**
     * Bind an implementation's float column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, float value);

    /**
     * Bind an implementation's float column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, float value);

    /**
     * Bind an implementation's double column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, double value);

    /**
     * Bind an implementation's double column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, double value);

    /**
     * Bind an implementation's boolean column value by its index.
     * @param index The column's index
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, bool value);

    /**
     * Bind an implementation's boolean column value by its name.
     * @param name The column's name
     * @param value The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, bool value);

    /**
     * Bind an implementation's map column value by its index.
     * @param index The column's index
     * @param values The value to bind
     * @return true on success, false on failure
     */
    bool Bind(size_t index, const std::unordered_map<std::string,
        std::vector<char>>& values);

    /**
     * Bind an implementation's map column value by its name.
     * @param name The column's name
     * @param values The value to bind
     * @return true on success, false on failure
     */
    bool Bind(const String& name, const std::unordered_map<
        std::string, std::vector<char>>& values);

    /**
     * Get an implementation's string column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, String& value);

    /**
     * Get an implementation's string column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, String& value);

    /**
     * Get an implementation's blob column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, std::vector<char>& value);

    /**
     * Get an implementation's blob column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, std::vector<char>& value);

    /**
     * Get an implementation's UUID column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, libobjgen::UUID& value);

    /**
     * Get an implementation's UUID column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, libobjgen::UUID& value);

    /**
     * Get an implementation's 32-bit integer column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, int32_t& value);

    /**
     * Get an implementation's 32-bit integer column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, int32_t& value);

    /**
     * Get an implementation's 64-bit integer column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, int64_t& value);

    /**
     * Get an implementation's 64-bit integer column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, int64_t& value);

    /**
     * Get an implementation's float column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, float& value);

    /**
     * Get an implementation's float column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, float& value);

    /**
     * Get an implementation's double column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, double& value);

    /**
     * Get an implementation's double column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, double& value);

    /**
     * Get an implementation's boolean column value by its index.
     * @param index The column's index
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(size_t index, bool& value);

    /**
     * Get an implementation's boolean column value by its name.
     * @param name The column's name
     * @param value Variable to bind the value to
     * @return true on success, false on failure
     */
    bool GetValue(const String& name, bool& value);

    /**
     * Get all of the query imementation's result's rows as
     * bytes mapped by column name.
     * @param rows Output map to insert a row with each
     *  column's bytes mapped by column name
     * @return true on success, false on failure
     */
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    /**
     * Check current query implementation's state validity.
     * @return true on valid, false on invalid
     */
    bool IsValid() const;

    /**
     * Get the count of affected rows from the last query
     * execution.
     * @return Number of affected rows
     */
    int64_t AffectedRowCount() const;

    /**
     * Copy operator implementation.
     * @param other The other query to copy
     * @return Reference to the new DatabaseQuery
     */
    DatabaseQuery& operator=(DatabaseQuery&& other);

protected:
    /// Database specific implementation
    DatabaseQueryImpl *mImpl;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEQUERY_H
