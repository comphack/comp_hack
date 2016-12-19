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

class DatabaseQueryCassandra : public DatabaseQueryImpl
{
public:
    DatabaseQueryCassandra(DatabaseCassandra *pDatabase);
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

    virtual bool GetMap(size_t index, std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool GetMap(const String& name, std::unordered_map<
        std::string, std::vector<char>> &values);
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    virtual bool BatchNext();

    virtual bool IsValid() const;

private:
    DatabaseCassandra *mDatabase;
    const CassPrepared *mPrepared;
    CassStatement *mStatement;

    CassFuture *mFuture;
    const CassResult *mResult;

    CassIterator *mRowIterator;
    CassBatch *mBatch;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEQUERYCASSANDRA_H
