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

// Cassandra Includes
#include <cassandra.h>

namespace libcomp
{

class DatabaseCassandra : public Database
{
public:
    friend class DatabaseQueryCassandra;

    DatabaseCassandra(const String& keyspace);
    virtual ~DatabaseCassandra();

    virtual bool Open(const String& address, const String& username = String(),
        const String& password = String());
    virtual bool Close();
    virtual bool IsOpen() const;

    virtual DatabaseQuery Prepare(const String& query);
    virtual bool Exists();
    virtual bool Setup();
    virtual bool Use();

protected:
    bool WaitForFuture(CassFuture *pFuture);

    CassSession* GetSession() const;

private:
    CassCluster *mCluster;
    CassSession *mSession;

    std::string mKeyspace;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASECASSANDRA_H
