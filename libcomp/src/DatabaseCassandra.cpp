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

#include "DatabaseCassandra.h"

 // libcomp Includes
#include "DatabaseQueryCassandra.h"
#include "Log.h"

// SQLite3 Includes
#include <sqlite3.h>

using namespace libcomp;

DatabaseCassandra::DatabaseCassandra(const String& keyspace)
    : mCluster(nullptr), mSession(nullptr), mKeyspace(keyspace.ToUtf8())
{
}

DatabaseCassandra::~DatabaseCassandra()
{
    Close();
}

bool DatabaseCassandra::Open(const String& address, const String& username,
    const String& password)
{
    // Make sure any previous connection is closed.
    bool result = Close();

    // Now make a new connection.
    if(result)
    {
        mSession = cass_session_new();
        mCluster = cass_cluster_new();

        cass_cluster_set_contact_points(mCluster, address.C());

        if(!username.IsEmpty())
        {
            cass_cluster_set_credentials(mCluster, username.C(), password.C());
        }

        result = WaitForFuture(cass_session_connect(mSession, mCluster));
    }

    return result;
}

bool DatabaseCassandra::Close()
{
    bool result = true;

    if(nullptr != mSession)
    {
        result = WaitForFuture(cass_session_close(mSession));

        cass_session_free(mSession);
        mSession = nullptr;
    }

    if(nullptr != mCluster)
    {
        cass_cluster_free(mCluster);
        mCluster = nullptr;
    }

    if(result)
    {
        mError.Clear();
    }

    return result;
}

bool DatabaseCassandra::IsOpen() const
{
    return nullptr != mSession;
}

DatabaseQuery DatabaseCassandra::Prepare(const String& query)
{
    return DatabaseQuery(new DatabaseQueryCassandra(this), query);
}

bool DatabaseCassandra::Exists()
{
    DatabaseQuery q = Prepare(libcomp::String("SELECT keyspace_name FROM system_schema.keyspaces WHERE keyspace_name = '%1';").Arg(mKeyspace));
    if(!q.Execute())
    {
        LOG_CRITICAL("Failed to query for keyspace.\n");

        return false;
    }

    std::list<std::unordered_map<std::string, std::vector<char>>> results;
    q.Next();

    return q.GetRows(results) && results.size() > 0;
}

bool DatabaseCassandra::Setup()
{
    if(!IsOpen())
    {
        LOG_ERROR("Trying to setup a database that is not open!\n");

        return false;
    }

    // Delete the old keyspace if it exists.
    if(!Execute(libcomp::String("DROP KEYSPACE IF EXISTS %1;").Arg(mKeyspace)))
    {
        LOG_ERROR("Failed to delete old keyspace.\n");

        return false;
    }

    // Now re-create the keyspace.
    if(!Execute(libcomp::String("CREATE KEYSPACE %1 WITH REPLICATION = {"
        " 'class' : 'NetworkTopologyStrategy', 'datacenter1' : 1 };").Arg(mKeyspace)))
    {
        LOG_ERROR("Failed to create keyspace.\n");

        return false;
    }

    // Use the keyspace.
    if(!Use())
    {
        LOG_ERROR("Failed to use the keyspace.\n");

        return false;
    }

    if(!Execute("CREATE TABLE objects ( uid uuid PRIMARY KEY, "
        "member_vars map<ascii, blob> );"))
    {
        LOG_ERROR("Failed to create the objects table.\n");

        return false;
    }

    return true;
}

bool DatabaseCassandra::Use()
{
    // Use the keyspace.
    if(!Execute(libcomp::String("USE %1;").Arg(mKeyspace)))
    {
        LOG_ERROR("Failed to use the keyspace.\n");

        return false;
    }

    return true;
}

bool DatabaseCassandra::WaitForFuture(CassFuture *pFuture)
{
    bool result = true;

    cass_future_wait(pFuture);

    CassError errorCode = cass_future_error_code(pFuture);

    // Handle an error.
    if(CASS_OK != errorCode)
    {
        const char *szMessage;
        size_t messageLength;

        // Get.
        cass_future_error_message(pFuture, &szMessage, &messageLength);
  
        // Save.
        mError = String(szMessage, messageLength);

        result = false;
    }

    cass_future_free(pFuture);

    return result;
}

CassSession* DatabaseCassandra::GetSession() const
{
    return mSession;
}
