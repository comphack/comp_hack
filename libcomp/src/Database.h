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

#ifndef LIBCOMP_SRC_DATABASE_H
#define LIBCOMP_SRC_DATABASE_H

// libcomp Includes
#include "CString.h"
#include "DatabaseConfig.h"
#include "DatabaseQuery.h"
#include "PersistentObject.h"

namespace libcomp
{

class DatabaseBind;

/**
 * Abstract base class that represents a database to use for loading
 * and modifying @ref PersistentObject instances as well as utility
 * tables by translating actions to database specific query syntax.
 * std::type_index is used as a parameter to access object metadata
 * to perform various tasks such as checking column data types and
 * knowing what table name to use in a select statement.
 */
class Database
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
    ~Database();

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
     * @return true if it could be set up, false if it could not
     */
    virtual bool Setup() = 0;

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
     * @param type C++ type representing the object type to load
     * @param pValue Database specific column binding
     * @return List of pointers to loaded objects from the query results
     */
    virtual std::list<std::shared_ptr<PersistentObject>> LoadObjects(
        std::type_index type, DatabaseBind *pValue) = 0;

    /**
     * Load one @ref PersistentObject instance from a single bound
     * database column and value to select upon.  This simply filters
     * down the results of @ref LoadObjects to the first record so
     * it should be used only when the value being bound to is unique.
     * @param type C++ type representing the object type to load
     * @param pValue Database specific column binding
     * @return Pointer to the first loaded object from the query results
     */
    virtual std::shared_ptr<PersistentObject> LoadSingleObject(
        std::type_index type, DatabaseBind *pValue);

    /**
     * Insert one @ref PersistentObject instance into the database.
     * @param obj Pointer to the object to insert
     * @return true on success, false on failure
     */
    virtual bool InsertSingleObject(std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Update all fields on one @ref PersistentObject instance in the database.
     * @param obj Pointer to the object to insert
     * @return true on success, false on failure
     */
    virtual bool UpdateSingleObject(std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Delete one @ref PersistentObject instance from the database.
     * @param obj Pointer to the object to insert
     * @return true on success, false on failure
     */
    virtual bool DeleteSingleObject(std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Retrieve the last error raised by a database operation.
     * @return The last error that occurred
     */
    String GetLastError() const;
    
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
     * @param type C++ type of object to load
     * @param query Current query to use results from
     * @return Pointer to the new object
     */
    std::shared_ptr<PersistentObject> LoadSingleObjectFromRow(
        std::type_index type, DatabaseQuery& query);

    /// Last error raised by a database related action
    String mError;

    /// Pointer to the config file used to configure the database connection
    std::shared_ptr<objects::DatabaseConfig> mConfig;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASE_H
