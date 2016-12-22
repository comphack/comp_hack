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

class DatabaseCassandra : public Database
{
public:
    friend class DatabaseQueryCassandra;

    DatabaseCassandra(const std::shared_ptr<objects::DatabaseConfigCassandra>& config);
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
    virtual bool DeleteSingleObject(std::shared_ptr<PersistentObject>& obj);

    bool VerifyAndSetupSchema();
    bool UsingDefaultKeyspace();

protected:
    bool WaitForFuture(CassFuture *pFuture);
    String GetVariableType(const std::shared_ptr<libobjgen::MetaVariable> var);

    std::vector<char> ConvertToRawByteStream(const std::shared_ptr<libobjgen::MetaVariable>& var,
        const std::vector<char>& columnData);

    CassSession* GetSession() const;

private:
    CassCluster *mCluster;
    CassSession *mSession;

    std::shared_ptr<objects::DatabaseConfigCassandra> mConfig;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASECASSANDRA_H
