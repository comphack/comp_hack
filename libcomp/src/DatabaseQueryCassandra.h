/**
 * @file libcomp/src/DatabaseQueryCassandra.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief A Cassandra database query.
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

#ifndef LIBCOMP_SRC_DATABASEQUERYCASSANDRA_H
#define LIBCOMP_SRC_DATABASEQUERYCASSANDRA_H

// libcomp Includes
#include "DatabaseQuery.h"

// Cassandra Includes
#include <cassandra.h>

namespace libcomp
{

class DatabaseCassandra;

/**
 * Cassandra database specific implementation of a query with binding and
 * data retrieval functionality.
 */
class DatabaseQueryCassandra : public DatabaseQueryImpl
{
public:
    /**
     * Create a new Cassandra database query.
     * @param pDatabase Pointer to the executing Cassandra database
     */
    DatabaseQueryCassandra(DatabaseCassandra *pDatabase);

    /**
     * Clean up the query.
     */
    virtual ~DatabaseQueryCassandra();

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

private:
    /**
     * Get a Cassandra value pointer to the current row's column data.
     * via the column's index.
     * @param index Column index to retrieve from
     * @return Pointer to a Cassandra value
     */
    const CassValue* GetValue(size_t index);

    /**
     * Get a Cassandra value pointer to the current row's column data.
     * via the column's name.
     * @param name Column name to retrieve from
     * @return Pointer to a Cassandra value
     */
    const CassValue* GetValue(const String& name);

    /**
     * Get a string value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetTextValue(const CassValue *pValue, String& value);

    /**
     * Get a blob value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetBlobValue(const CassValue *pValue, std::vector<char>& value);

    /**
     * Get a UUID value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetUuidValue(const CassValue *pValue, libobjgen::UUID& value);

    /**
     * Get a 32-bit integer value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetIntValue(const CassValue *pValue, int32_t& value);

    /**
     * Get a 64-bit integer value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetBigIntValue(const CassValue *pValue, int64_t& value);

    /**
     * Get a float value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetFloatValue(const CassValue *pValue, float& value);

    /**
     * Get a double value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetDoubleValue(const CassValue *pValue, double& value);

    /**
     * Get a boolean value represented by a pointer to a Cassandra value.
     * @param pValue Pointer to the data's Cassandra value
     * @param value Variable to store the resulting data in
     * @return true on success, false on failure
     */
    bool GetBoolValue(const CassValue *pValue, bool& value);

    /// Pointer to the Cassandra database the query executes on
    DatabaseCassandra *mDatabase;

    /// Pointer to the Cassandra representation of a statement that has been
    /// prepared cluster-side
    const CassPrepared *mPrepared;

    /// Pointer to the Cassandra representation of the query as a statement
    CassStatement *mStatement;

    /// Pointer to the Cassandra representation of the future result of an
    /// operation
    CassFuture *mFuture;

    /// Pointer to the Cassandra representation of the results from the query's
    /// execution
    const CassResult *mResult;

    /// Pointer to the current row being used by the "Get" functions
    CassIterator *mRowIterator;

    /// Pointer to the Cassandra representation of a batch of query statements
    CassBatch *mBatch;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEQUERYCASSANDRA_H
