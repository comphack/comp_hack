/**
 * @file libcomp/src/DatabaseSQLite3.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to handle an SQLite3 database.
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

#ifndef LIBCOMP_SRC_DATABASESQLITE3_H
#define LIBCOMP_SRC_DATABASESQLITE3_H

 // libcomp Includes
#include "Database.h"
#include "DatabaseConfigSQLite3.h"

// libobjgen Includes
#include <MetaVariable.h>

typedef struct sqlite3 sqlite3;

namespace libcomp
{

/**
 * Represents a SQLite3 database connection associated to a specific
 * file via the supplied config.
 */
class DatabaseSQLite3 : public Database
{
public:
    /**
     * Create a new SQLite3 Database connection.
     * @param config Pointer to a database configuration
     */
    DatabaseSQLite3(const std::shared_ptr<objects::DatabaseConfigSQLite3>& config);

    /**
     * Close and clean up the database connection.
     */
    virtual ~DatabaseSQLite3();

    /**
     * Open or create the database file for use.
     * @return true on success, false on failure
     */
    virtual bool Open();

    /**
     * Close the database connection and file.
     * @return true on success, false on failure
     */
    virtual bool Close();
    virtual bool IsOpen() const;

    virtual DatabaseQuery Prepare(const String& query);

    /**
     * Check if the database file exists.
     * @return true if it exists, false if it does not
     */
    virtual bool Exists();
    virtual bool Setup();
    virtual bool Use();

    virtual std::list<std::shared_ptr<PersistentObject>> LoadObjects(
        std::type_index type, DatabaseBind *pValue);

    virtual bool InsertSingleObject(std::shared_ptr<PersistentObject>& obj);
    virtual bool UpdateSingleObject(std::shared_ptr<PersistentObject>& obj);
    virtual bool DeleteSingleObject(std::shared_ptr<PersistentObject>& obj);

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
     * Check if this database is configured to use the default file
     * for the SQLite3 database.  Non-default file connections are
     * only responsible for verifying the schema of their own tables where-as
     * default file connections may also have additional tables to maintain.
     * @return true if the default file is configured, false if it is not
     */
    bool UsingDefaultDatabaseFile();

private:
    /**
     * Get the path to the database file to use.
     * @return Path to the database file to use
     */
    String GetFilepath() const;

    /**
     * Get the SQLite3 type represented by a MetaVariable type.
     * @param var Metadata variable containing a type to conver to a SQLite3 type
     * @return Data type string representing a SQLite3 type
     */
    String GetVariableType(const std::shared_ptr<libobjgen::MetaVariable> var);

    /// Pointer to the SQLite3 representation of the database file connection
    sqlite3 *mDatabase;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASESQLITE3_H
