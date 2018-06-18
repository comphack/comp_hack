/**
 * @file libcomp/src/Database.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class to handle the database.
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

#ifndef LIBCOMP_SRC_DATABASE_H
#define LIBCOMP_SRC_DATABASE_H

// libcomp Includes
#include "CString.h"
#include "DatabaseConfig.h"
#include "DatabaseQuery.h"
#include "DatabaseChangeSet.h"
#include "PersistentObject.h"

namespace libcomp
{

class BaseServer;
class DatabaseBind;
class DataStore;

/**
 * Abstract base class that represents a database to use for loading
 * and modifying @ref PersistentObject instances as well as utility
 * tables by translating actions to database specific query syntax.
 * The typeHash is used as a parameter to access object metadata
 * to perform various tasks such as checking column data types and
 * knowing what table name to use in a select statement.
 */
class Database : public std::enable_shared_from_this<Database>
{
public:
    /**
     * Create a new Database connection.
     * @param config Pointer to a database configuration
     */
    Database(const std::shared_ptr<objects::DatabaseConfig>& config);

    /**
     * Close and clean up the database connection.
     */
    virtual ~Database();

    /**
     * Open the connection to the database.
     * @return true on success, false on failure
     */
    virtual bool Open() = 0;

    /**
     * Close the connection to the database.
     * @return true on success, false on failure
     */
    virtual bool Close() = 0;

    /**
     * Check if the connection to the database is open.
     * @return true if it is open, false if it is not
     */
    virtual bool IsOpen() const = 0;

    /**
     * Prepare a database query for execution based upon query text.
     * @param query Query text to prepare
     * @return Prepared query
     */
    virtual DatabaseQuery Prepare(const String& query) = 0;

    /**
     * Prepare and execute a database query for execution based upon
     * query text.  Queries that return results should not use this
     * function since a query is not returned to retrieve the results
     * from.
     * @param query Query text to prepare and execute
     * @return true on success, false on error
     */
    virtual bool Execute(const String& query);

    /**
     * Check if the database is valid.
     * @return true if it exists, false if it does not
     */
    virtual bool Exists() = 0;

    /**
     * Setup the database schema and perform all validation steps.
     * @param rebuild Optional parameter to rebuild all tables used
     *  by this database during the verification step
     * @param server Server to pass to migration scripts (or null).
     * @param pDataStore Pointer to the data store for migrations (or null).
     * @param migrationDirectory Directory to look for migrations in.
     * @return true if it could be set up, false if it could not
     */
    virtual bool Setup(bool rebuild = false,
        const std::shared_ptr<BaseServer>& server = {},
        DataStore *pDataStore = nullptr,
        const std::string& migrationDirectory = std::string()) = 0;

    /**
     * Use the database schema, keyspace, namespace, etc.
     * @return true on success, false on failure
     */
    virtual bool Use() = 0;

    /**
     * Check if the supplied table name exists and has at least one row.
     * @param table Name of the table to check
     * @return true if a row exists, false if it does not
     */
    virtual bool TableHasRows(const String& table);

    /**
     * Load multiple @ref PersistentObject instances from a single bound
     * database column and value to select upon.
     * @param typeHash C++ type hash representing the object type to load
     * @param pValue Database specific agnostic binding
     * @return List of pointers to loaded objects from the query results
     */
    virtual std::list<std::shared_ptr<PersistentObject>> LoadObjects(
       size_t typeHash, DatabaseBind *pValue) = 0;

    /**
     * Load one @ref PersistentObject instance from a single bound
     * database column and value to select upon.  This simply filters
     * down the results of @ref LoadObjects to the first record so
     * it should be used only when the value being bound to is unique.
     * @param typeHash C++ type hash representing the object type to load
     * @param pValue Database agnostic column binding
     * @return Pointer to the first loaded object from the query results
     */
    virtual std::shared_ptr<PersistentObject> LoadSingleObject(
        size_t typeHash, DatabaseBind *pValue);

    /**
     * Insert one @ref PersistentObject instance into the database.
     * @param obj Pointer to the object to insert
     * @return true on success, false on failure
     */
    virtual bool InsertSingleObject(std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Update all fields on one @ref PersistentObject instance in the database.
     * @param obj Pointer to the object to update
     * @return true on success, false on failure
     */
    virtual bool UpdateSingleObject(std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Delete one @ref PersistentObject instance from the database.
     * @param obj Pointer to the object to delete
     * @return true on success, false on failure
     */
    virtual bool DeleteSingleObject(std::shared_ptr<PersistentObject>& obj);

    /**
     * Delete multiple @ref PersistentObject instances from the database at
     * once.
     * @param objs List of ointers to the objects to delete
     * @return true on success, false on failure
     */
    virtual bool DeleteObjects(std::list<std::shared_ptr<PersistentObject>>& objs) = 0;

    /**
     * Check if a table exists.
     * @param table Name of the table to check for.
     * @returns true if the table exists, false otherwise.
     */
    virtual bool TableExists(const libcomp::String& table) = 0;

    /**
     * Apply a database migration script.
     * @param server Server to pass to the script.
     * @param pDataStore Data store to read the script from.
     * @param migration Name of the migration.
     * @param path Path to the script in the data store.
     * @returns true if the migration was applied; false otherwise.
     */
    virtual bool ApplyMigration(const std::shared_ptr<BaseServer>& server,
        DataStore *pDataStore, const libcomp::String& migration,
        const libcomp::String& path);

    /**
     * Queue an object to insert during the next call to
     * ProcessTransactionQueue.
     * @param obj Pointer to the object to insert
     * @param uuid UUID used to group changes together
     */
    void QueueInsert(std::shared_ptr<PersistentObject> obj,
        const libobjgen::UUID& uuid = NULLUUID);

    /**
     * Queue an object to update during the next call to
     * ProcessTransactionQueue.
     * @param obj Pointer to the object to update
     * @param uuid UUID used to group changes together
     */
    void QueueUpdate(std::shared_ptr<PersistentObject> obj,
        const libobjgen::UUID& uuid = NULLUUID);

    /**
     * Queue an object to delete during the next call to
     * ProcessTransactionQueue.
     * @param obj Pointer to the object to delete
     * @param uuid UUID used to group changes together
     */
    void QueueDelete(std::shared_ptr<PersistentObject> obj,
        const libobjgen::UUID& uuid = NULLUUID);

    /**
     * Queue a set of changes to execute during the next call to
     * ProcessTransactionQueue.
     * @param changes DatabaseChangeSet to queue for processing
     * @return true on success, false on failure
     */
    bool QueueChangeSet(const std::shared_ptr<DatabaseChangeSet>& changes);

    /**
     * Pop and process all transactions stored in the transaction queue.
     * @return List of all transaction group UUIDs that failed to process
     */
    std::list<libobjgen::UUID> ProcessTransactionQueue();

    /**
     * Process one or many database changes as a single transaction.
     * @param changes Grouping of changes to apply to the database
     * @return true on success, false on failure
     */
    bool ProcessChangeSet(const std::shared_ptr<DatabaseChangeSet>& changes);

    /**
     * Check if the config is for the default database type.  Non-default connections are
     * only responsible for verifying the schema of their own tables where-as
     * default database connections may also have additional tables to maintain.
     * @return true if the default database type is configured, false if it is not
     */
    bool UsingDefaultDatabaseType();

    /**
     * Retrieve the last error raised by a database operation.
     * @return The last error that occurred
     */
    virtual String GetLastError();

    /**
     * Get the database config.
     * @return Pointer to the database config
     */
    std::shared_ptr<objects::DatabaseConfig> GetConfig() const;

protected:
    /**
     * Get a pointer to a new @ref PersistentObject of the specified
     * type populated with the current row being read from a database
     * query.
     * @param typeHash C++ type hash of object to load
     * @param query Current query to use results from
     * @return Pointer to the new object
     */
    std::shared_ptr<PersistentObject> LoadSingleObjectFromRow(
        size_t typeHash, DatabaseQuery& query);

    /**
     * Process one or many standard database changes as a single transaction.
     * @param changes Grouping of changes to apply to the database
     * @return true on success, false on failure
     */
    virtual bool ProcessStandardChangeSet(const std::shared_ptr<
        DBStandardChangeSet>& changes) = 0;

    /**
     * Process an operational change set as a single transaction.
     * @param changes Grouping of changes to apply to the database
     * @return true on success, false on failure
     */
    virtual bool ProcessOperationalChangeSet(const std::shared_ptr<
        DBOperationalChangeSet>& changes) = 0;

    /**
     * Get the list of objects mapped to the current database type
     * configured for the database.
     * @return List of pointers to object definitions with a source
     *  set to the current database type
     */
    std::vector<std::shared_ptr<libobjgen::MetaObject>> GetMappedObjects();

    /// Last error raised by a database related action
    String mError;

    /// Pointer to the config file used to configure the database connection
    std::shared_ptr<objects::DatabaseConfig> mConfig;

private:
    /// Map of transaction pointers by UUID
    std::unordered_map<std::string,
        std::shared_ptr<DBStandardChangeSet>> mTransactionQueue;

    /// Mutex to lock accessing the transaction queue
    std::mutex mTransactionLock;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASE_H
