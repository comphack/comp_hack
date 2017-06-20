/**
 * @file libcomp/src/DatabaseMariaDB.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Class to handle a MariaDB database.
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

#ifndef LIBCOMP_SRC_DATABASEMARIADB_H
#define LIBCOMP_SRC_DATABASEMARIADB_H

 // libcomp Includes
#include "Database.h"
#include "DatabaseConfigMariaDB.h"

// libobjgen Includes
#include <MetaVariable.h>

// Standard C++ Includes
#include <thread>

typedef struct st_mysql MYSQL;

namespace libcomp
{

/**
 * Represents a MariaDB database connection via the supplied config.
 */
class DatabaseMariaDB : public Database
{
public:
    /**
     * Create a new MariaDB Database connection.
     * @param config Pointer to a database configuration
     */
    DatabaseMariaDB(const std::shared_ptr<objects::DatabaseConfigMariaDB>& config);

    /**
     * Close and clean up the database connection.
     */
    virtual ~DatabaseMariaDB();

    /**
     * Open or create the database for use.
     * @return true on success, false on failure
     */
    virtual bool Open();

    /**
     * Close dall database connections.
     * @return true on success, false on failure
     */
    virtual bool Close();

    /**
     * Close the specified database connection.
     * @param connection Pointer to the connection to close
     * @return true on success, false on failure
     */
    bool Close(MYSQL*& connection);

    virtual bool IsOpen() const;

    virtual DatabaseQuery Prepare(const String& query);

    /**
     * Check if the database exists.
     * @return true if it exists, false if it does not
     */
    virtual bool Exists();
    virtual bool Setup(bool rebuild = false);
    virtual bool Use();

    virtual std::list<std::shared_ptr<PersistentObject>> LoadObjects(
        size_t typeHash, DatabaseBind *pValue);

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
     * @param recreateTables Optional parameter to archive and recreate all
     *  tables used by this database
     * @return true on success, false on failure
     */
    bool VerifyAndSetupSchema(bool recreateTables = false);

    /**
     * Retrieve the last error raised by a database operation.
     * @return The last error that occurred
     */
    virtual String GetLastError();

protected:
    virtual bool ProcessStandardChangeSet(const std::shared_ptr<
        DBStandardChangeSet>& changes);
    virtual bool ProcessOperationalChangeSet(const std::shared_ptr<
        DBOperationalChangeSet>& changes);

private:
    /**
     * Process and explicit update to a single record, checking each column's
     * state before and verifying it set to the expected value afterwards.
     * @param update Pointer to the explicit update
     * @return true if the the updated succeeded, false if it did not
     */
    bool ProcessExplicitUpdate(const std::shared_ptr<
        DBExplicitUpdate>& update);

    /**
     * Establish a connection to a MariaDB database.
     * @param connection Pointer to database connection to connect with
     * @param databaseName Name of the database to connect to on the configured host
     * @return true if the connection succeeded, false if it did not
     */
    bool ConnectToDatabase(MYSQL*& connection, const libcomp::String& databaseName);

    /**
     * Get a connection for the executing thread.
     * @param autoConnect true if the connection should be established before
     *  returning it, false if it should just return the conection reference
     * @return Reference to the executing thread's connection pointer
     */
    MYSQL*& GetConnection(bool autoConnect);

    /**
     * Get the MariaDB type represented by a MetaVariable type.
     * @param var Metadata variable containing a type to conver to a MariaDB type
     * @return Data type string representing a MariaDB type
     */
    String GetVariableType(const std::shared_ptr<libobjgen::MetaVariable> var);

    /// Mutex to lock access to the database connection map
    std::mutex mConnectionLock;

    /// Pointers to the MariaDB representation of a database connection per thread.
    /// References to the pointers in this collection are passed around for use
    /// throughout the class as they are not removed or modified until the database
    /// is closed.
    std::unordered_map<std::thread::id, MYSQL*> mConnections;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEMARIADB_H
