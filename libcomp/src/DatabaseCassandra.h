/**
 * @file libcomp/src/DatabaseCassandra.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to handle a Cassandra database.
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

#ifndef LIBCOMP_SRC_DATABASECASSANDRA_H
#define LIBCOMP_SRC_DATABASECASSANDRA_H

// libcomp Includes
#include "Database.h"
#include "DatabaseConfigCassandra.h"

// libobjgen Includes
#include <MetaVariable.h>

// Cassandra Includes
#include <cassandra.h>

namespace libcomp
{

/**
 * Represents a Cassandra database connection associated to a specific
 * keyspace via the supplied config.
 */
class DatabaseCassandra : public Database
{
public:
    friend class DatabaseQueryCassandra;

    /**
     * Create a new Cassandra Database connection.
     * @param config Pointer to a database configuration
     */
    DatabaseCassandra(const std::shared_ptr<objects::DatabaseConfigCassandra>& config);

    /**
     * Close and clean up the database connection.
     */
    virtual ~DatabaseCassandra();

    virtual bool Open();
    virtual bool Close();
    virtual bool IsOpen() const;

    virtual DatabaseQuery Prepare(const String& query);
    virtual bool Exists();
    virtual bool Setup();
    virtual bool Use();

    virtual std::list<std::shared_ptr<PersistentObject>> LoadObjects(
        std::type_index type, DatabaseBind *pValue);

    virtual bool InsertSingleObject(std::shared_ptr<PersistentObject>& obj);
    virtual bool UpdateSingleObject(std::shared_ptr<PersistentObject>& obj);
    virtual bool DeleteObjects(std::list<std::shared_ptr<PersistentObject>>& objs);

    /**
     * Verify/create any missing tables based off of @ref PersistentObject
     * types used by the database as well as any utility tables needed.  Tables
     * with invalid schemas will be archived in case data migration needs to
     * take place and a replacement will be built instead and missing indexes
     * will be created should they not exist based off of of fields marked
     * as lookup keys in their objgen definitions.
     * @return true on success, false on failure
     */
    bool VerifyAndSetupSchema();

    /**
     * Check if this database is configured to use the default keyspace
     * for the Cassandra database.  Non-default keyspace connections are
     * only responsible for verifying the schema of their own tables where-as
     * default keyspace connections may also have additional tables to maintain.
     * @return true if the default keyspace is configured, false if it is not
     */
    bool UsingDefaultKeyspace();

protected:
    /**
     * Wait for the completion of an async future execution.
     * @param pFuture Pointer to a query's future operation
     * @return true on success, false on error
     */
    bool WaitForFuture(CassFuture *pFuture);

    /**
     * Get the Cassandra type represented by a MetaVariable type.
     * @param var Metadata variable containing a type to conver to a Cassandra type
     * @return Data type string representing a Cassandra type
     */
    String GetVariableType(const std::shared_ptr<libobjgen::MetaVariable> var);

    /**
     * Get a pointer to the Cassandra representation of the connection's session.
     * @return Pointer to the Cassandra representation of the connection's session
     */
    CassSession* GetSession() const;

private:
    /// Pointer to the Cassandra representation of the connection's cluster
    CassCluster *mCluster;

    /// Pointer to the Cassandra representation of the connection's session
    CassSession *mSession;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASECASSANDRA_H
