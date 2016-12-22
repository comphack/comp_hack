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

class DatabaseSQLite3 : public Database
{
public:
    DatabaseSQLite3(const std::shared_ptr<objects::DatabaseConfigSQLite3>& config);
    virtual ~DatabaseSQLite3();

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
    virtual bool DeleteSingleObject(std::shared_ptr<PersistentObject>& obj);

    bool VerifyAndSetupSchema();
    bool UsingDefaultDatabaseFile();

private:
    String GetFilepath() const;

    String GetVariableType(const std::shared_ptr<libobjgen::MetaVariable> var);

    std::vector<char> ConvertToRawByteStream(const std::shared_ptr<libobjgen::MetaVariable>& var,
        const std::vector<char>& columnData);

    sqlite3 *mDatabase;

    std::shared_ptr<objects::DatabaseConfigSQLite3> mConfig;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASESQLITE3_H
