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
#include "DatabaseBind.h"
#include "DatabaseQueryCassandra.h"
#include "Log.h"
#include "PersistentObject.h"

// SQLite3 Includes
#include <sqlite3.h>

using namespace libcomp;

DatabaseCassandra::DatabaseCassandra(const std::shared_ptr<
    objects::DatabaseConfigCassandra>& config) :
    Database(std::dynamic_pointer_cast<objects::DatabaseConfig>(config)),
    mCluster(nullptr), mSession(nullptr)
{
}

DatabaseCassandra::~DatabaseCassandra()
{
    Close();
}

bool DatabaseCassandra::Open()
{
    auto config = std::dynamic_pointer_cast<objects::DatabaseConfigCassandra>(mConfig);
    auto address = config->GetIP();
    auto username = config->GetUsername();
    auto password = config->GetPassword();

    // Make sure any previous connection is closed an that we have the base
    // necessary configuration to connect.
    bool result = Close() && address.Length() > 0;

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
    DatabaseQuery q = Prepare(String(
        "SELECT keyspace_name FROM system_schema.keyspaces WHERE keyspace_name = '%1';")
        .Arg(std::dynamic_pointer_cast<objects::DatabaseConfigCassandra>(
            mConfig)->GetKeyspace()));
    if(!q.Execute())
    {
        LOG_CRITICAL("Failed to query for keyspace.\n");

        return false;
    }

    std::list<std::unordered_map<std::string, std::vector<char>>> results;
    q.Next();

    return q.GetRows(results) && results.size() > 0;
}

bool DatabaseCassandra::Setup(bool rebuild)
{
    if(!IsOpen())
    {
        LOG_ERROR("Trying to setup a database that is not open!\n");

        return false;
    }

    auto keyspace = std::dynamic_pointer_cast<objects::DatabaseConfigCassandra>(
        mConfig)->GetKeyspace();
    if(!Exists())
    {
        // Delete the old keyspace if it exists.
        if(!Execute(String("DROP KEYSPACE IF EXISTS %1;").Arg(keyspace)))
        {
            LOG_ERROR("Failed to delete old keyspace.\n");

            return false;
        }

        // Now re-create the keyspace.
        if(!Execute(String("CREATE KEYSPACE %1 WITH REPLICATION = {"
            " 'class' : 'NetworkTopologyStrategy', 'datacenter1' : 1 };").Arg(keyspace)))
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

        if(UsingDefaultKeyspace() &&
            !Execute("CREATE TABLE objects ( uid uuid PRIMARY KEY, "
            "member_vars map<ascii, blob> );"))
        {
            LOG_ERROR("Failed to create the objects table.\n");

            return false;
        }
    }
    else if(!Use())
    {
        LOG_ERROR("Failed to use the existing keyspace.\n");

        return false;
    }

    LOG_DEBUG(String("Database connection established to '%1' keyspace.\n")
        .Arg(keyspace));

    if(!VerifyAndSetupSchema(rebuild))
    {
        LOG_ERROR("Schema verification and setup failed.\n");

        return false;
    }

    return true;
}

bool DatabaseCassandra::Use()
{
    // Use the keyspace.
    auto keyspace = std::dynamic_pointer_cast<objects::DatabaseConfigCassandra>(
        mConfig)->GetKeyspace();
    if(!Execute(String("USE %1;").Arg(keyspace)))
    {
        LOG_ERROR("Failed to use the keyspace.\n");

        return false;
    }

    return true;
}

std::list<std::shared_ptr<PersistentObject>> DatabaseCassandra::LoadObjects(
    size_t typeHash, DatabaseBind *pValue)
{
    std::list<std::shared_ptr<PersistentObject>> objects;

    auto metaObject = PersistentObject::GetRegisteredMetadata(typeHash);

    if(nullptr == metaObject)
    {
        LOG_ERROR("Failed to lookup MetaObject.\n");

        return {};
    }

    String cql = String("SELECT * FROM %1%2").Arg(
        String(metaObject->GetName()).ToLower()).Arg(
        (nullptr != pValue
            ? String(" WHERE %1 = ?").Arg(pValue->GetColumn().ToLower())
            : ""));

    DatabaseQuery query = Prepare(cql);

    if(!query.IsValid())
    {
        LOG_ERROR(String("Failed to prepare CQL query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return {};
    }

    if(nullptr != pValue && !pValue->Bind(query))
    {
        LOG_ERROR(String("Failed to bind value: %1\n").Arg(
            pValue->GetColumn()));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return {};
    }

    if(!query.Execute())
    {
        LOG_ERROR(String("Failed to execute query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return {};
    }

    int failures = 0;

    while(query.Next())
    {
        auto obj = LoadSingleObjectFromRow(typeHash, query);

        if(nullptr != obj)
        {
            objects.push_back(obj);
        }
        else
        {
            failures++;
        }
    }

    if(failures > 0)
    {
        LOG_ERROR(String("%1 '%2' row%3 failed to load.\n").Arg(failures).Arg(
            metaObject->GetName()).Arg(failures != 1 ? "s" : ""));
    }

    return objects;
}

bool DatabaseCassandra::InsertSingleObject(std::shared_ptr<PersistentObject>& obj)
{
    auto metaObject = obj->GetObjectMetadata();

    std::stringstream objstream;
    if(!obj->Save(objstream))
    {
        return false;
    }

    if(obj->GetUUID().IsNull() && !obj->Register(obj))
    {
        return false;
    }

    std::list<String> columnNames;
    columnNames.push_back("uid");

    std::list<String> columnBinds;
    columnBinds.push_back("?");

    auto values = obj->GetMemberBindValues();

    for(auto value : values)
    {
        columnNames.push_back(value->GetColumn());
        columnBinds.push_back("?");
    }

    String cql = String("INSERT INTO %1 (%2) VALUES (%3)").Arg(
        String(metaObject->GetName()).ToLower()).Arg(
        String::Join(columnNames, ", ")).Arg(
        String::Join(columnBinds, ", "));

    DatabaseQuery query = Prepare(cql);

    if(!query.IsValid())
    {
        LOG_ERROR(String("Failed to prepare CQL query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    if(!query.Bind("uid", obj->GetUUID()))
    {
        LOG_ERROR("Failed to bind value: uid\n");
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    for(auto value : values)
    {
        if(!value->Bind(query))
        {
            LOG_ERROR(String("Failed to bind value: %1\n").Arg(
                value->GetColumn()));
            LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

            return false;
        }

        delete value;
    }

    if(!query.Execute())
    {
        LOG_ERROR(String("Failed to execute query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    return true;
}

bool DatabaseCassandra::UpdateSingleObject(std::shared_ptr<PersistentObject>& obj)
{
    auto metaObject = obj->GetObjectMetadata();

    std::stringstream objstream;
    if(!obj->Save(objstream))
    {
        return false;
    }

    if(obj->GetUUID().IsNull())
    {
        return false;
    }

    auto values = obj->GetMemberBindValues();

    std::list<String> columnNames;

    for(auto value : values)
    {
        columnNames.push_back(String("%1 = ?").Arg(value->GetColumn()));
    }

    String cql = String("UPDATE %1 SET %2 WHERE uid = ?").Arg(
        String(metaObject->GetName()).ToLower()).Arg(
        String::Join(columnNames, ", "));

    DatabaseQuery query = Prepare(cql);

    if(!query.IsValid())
    {
        LOG_ERROR(String("Failed to prepare CQL query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    for(auto value : values)
    {
        if(!value->Bind(query))
        {
            LOG_ERROR(String("Failed to bind value: %1\n").Arg(
                value->GetColumn()));
            LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

            return false;
        }

        delete value;
    }

    if(!query.Bind("uid", obj->GetUUID()))
    {
        LOG_ERROR("Failed to bind value: uid\n");
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    if(!query.Execute())
    {
        LOG_ERROR(String("Failed to execute query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    return true;
}

bool DatabaseCassandra::DeleteObjects(std::list<std::shared_ptr<PersistentObject>>& objs)
{
    std::shared_ptr<libobjgen::MetaObject> metaObject;

    std::list<String> uidBindings;
    for(auto obj : objs)
    {
        auto uuid = obj->GetUUID();

        if(uuid.IsNull())
        {
            return false;
        }

        auto metaObj = obj->GetObjectMetadata();

        if(nullptr == metaObject)
        {
            metaObject = metaObj;
        }
        else if(metaObject != metaObj)
        {
            return false;
        }

        uidBindings.push_back("?");
    }

    String cql = String("DELETE FROM %1 WHERE uid in (%2)").Arg(
        String(metaObject->GetName()).ToLower()).Arg(
        String::Join(uidBindings, ", "));

    DatabaseQuery query = Prepare(cql);

    if(!query.IsValid())
    {
        LOG_ERROR(String("Failed to prepare CQL query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    for(auto obj : objs)
    {
        if(!query.Bind("uid", obj->GetUUID()))
        {
            LOG_ERROR("Failed to bind value: uid\n");
            LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

            return false;
        }
    }

    if(!query.Execute())
    {
        LOG_ERROR(String("Failed to execute query: %1\n").Arg(cql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    for(auto obj : objs)
    {
        obj->Unregister();
    }

    return true;
}

bool DatabaseCassandra::VerifyAndSetupSchema(bool recreateTables)
{
    auto keyspace = std::dynamic_pointer_cast<objects::DatabaseConfigCassandra>(
        mConfig)->GetKeyspace();
    std::vector<std::shared_ptr<libobjgen::MetaObject>> metaObjectTables;
    for(auto registrar : PersistentObject::GetRegistry())
    {
        std::string source = registrar.second->GetSourceLocation();
        if(source == keyspace || (source.length() == 0 && UsingDefaultKeyspace()))
        {
            metaObjectTables.push_back(registrar.second);
        }
    }

    if(metaObjectTables.size() == 0)
    {
        return true;
    }

    LOG_DEBUG("Verifying database table structure.\n");

    auto cmd = String("SELECT table_name, column_name, type"
                            " FROM system_schema.columns"
                            " WHERE keyspace_name = '%1';").Arg(keyspace);

    DatabaseQuery q = Prepare(cmd);
    if(!q.Execute())
    {
        LOG_CRITICAL("Failed to query for column schema.\n");

        return false;
    }

    std::unordered_map<std::string, std::unordered_map<std::string, String>> fieldMap;
    while(q.Next())
    {
        String tableName;
        String colName;
        String dataType;

        if(q.GetValue("table_name", tableName) &&
            q.GetValue("column_name", colName) &&
            q.GetValue("type", dataType))
        {
            std::unordered_map<std::string, String>& m = fieldMap[tableName.ToUtf8()];
            m[colName.ToUtf8()] = dataType;
        }
    }

    cmd = String("SELECT table_name, index_name"
        " FROM system_schema.indexes"
        " WHERE keyspace_name = '%1';").Arg(keyspace);

    q = Prepare(cmd);
    if(!q.Execute())
    {
        LOG_CRITICAL("Failed to query for column indexes.\n");

        return false;
    }

    //If we get zero rows, keep going anyway because execution didn't fail
    std::unordered_map<std::string, std::set<std::string>> indexedFields;
    while(q.Next())
    {
        String tableName;
        String indexName;

        if(q.GetValue("table_name", tableName) && q.GetValue("index_name", indexName))
        {
            std::set<std::string>& s = indexedFields[tableName.ToUtf8()];
            s.insert(indexName.ToUtf8());
        }
    }

    for(auto metaObjectTable : metaObjectTables)
    {
        auto metaObject = *metaObjectTable.get();
        auto objName = String(metaObject.GetName())
                            .ToLower().ToUtf8();

        std::vector<std::shared_ptr<libobjgen::MetaVariable>> vars;
        for(auto iter = metaObject.VariablesBegin();
            iter != metaObject.VariablesEnd(); iter++)
        {
            String type = GetVariableType(*iter);
            if(type.IsEmpty())
            {
                LOG_ERROR(String(
                    "Unsupported field type encountered: %1\n")
                    .Arg((*iter)->GetCodeType()));
                return false;
            }
            vars.push_back(*iter);
        }

        bool creating = false;
        bool archiving = false;
        std::set<std::string> needsIndex;
        auto tableIter = fieldMap.find(objName);
        if(tableIter == fieldMap.end())
        {
            creating = true;
        }
        else
        {
            archiving = recreateTables;

            std::unordered_map<std::string,
                String> columns = tableIter->second;
            if(columns.size() - 1 != vars.size()
                || columns.find("uid") == columns.end())
            {
                archiving = true;
            }
            else
            {
                auto indexes = indexedFields[objName];
                columns.erase("uid");
                for(auto var : vars)
                {
                    auto name = String(
                        var->GetName()).ToLower().ToUtf8();
                    auto type = GetVariableType(var);

                    if(columns.find(name) == columns.end()
                        || columns[name] != type)
                    {
                        archiving = true;
                    }

                    auto indexName = String("idx_%1_%2")
                        .Arg(objName).Arg(name).ToUtf8();
                    if(var->IsLookupKey() &&
                        indexes.find(indexName) == indexes.end())
                    {
                        needsIndex.insert(var->GetName());
                    }
                }
            }
        }

        if(archiving)
        {
            LOG_DEBUG(String("Archiving table '%1'...\n")
                .Arg(metaObject.GetName()));
            
            /// @todo: do this properly
            if(Execute(String("DROP TABLE %1;").Arg(objName)))
            {
                LOG_DEBUG("Archiving complete\n");
            }
            else
            {
                LOG_ERROR("Archiving failed\n");
                return false;
            }

            creating = true;
        }
            
        if(creating)
        {
            LOG_DEBUG(String("Creating table '%1'...\n")
                .Arg(metaObject.GetName()));

            bool success = false;

            std::stringstream ss;
            ss << "CREATE TABLE " << objName
                << " (uid uuid PRIMARY KEY";
            for(size_t i = 0; i < vars.size(); i++)
            {
                auto var = vars[i];
                String type = GetVariableType(var);

                ss << "," << std::endl << var->GetName() << " " << type;
            }
            ss << ");";

            success = Execute(ss.str());

            if(success)
            {
                LOG_DEBUG("Creation complete\n");
            }
            else
            {
                LOG_ERROR("Creation failed\n");
                return false;
            }

        }

        //If we made the table or are missing an index, make them now
        if(needsIndex.size() > 0 || creating)
        {
            for(size_t i = 0; i < vars.size(); i++)
            {
                auto var = vars[i];

                if(!var->IsLookupKey() ||
                    (!creating && needsIndex.find(var->GetName()) == needsIndex.end()))
                {
                    continue;
                }
                
                auto name = String("%1").Arg(var->GetName()).ToLower();

                auto indexStr = String("idx_%1_%2")
                    .Arg(objName).Arg(name);

                cmd = String("CREATE INDEX %1 ON %2(%3);")
                    .Arg(indexStr).Arg(objName).Arg(name);

                if(Execute(cmd))
                {
                    LOG_DEBUG(String("Created '%1' column index.\n")
                        .Arg(indexStr));
                }
                else
                {
                    LOG_ERROR(String("Creation of '%1' column index failed.\n")
                        .Arg(indexStr));
                    return false;
                }
            }
        }

        if(!creating && !archiving && needsIndex.size() == 0)
        {
            LOG_DEBUG(String("'%1': Verified\n")
                .Arg(metaObject.GetName()));
        }
    }

    LOG_DEBUG("Database verification complete.\n");

    return true;
}

bool DatabaseCassandra::UsingDefaultKeyspace()
{
    auto config = std::dynamic_pointer_cast<objects::DatabaseConfigCassandra>(mConfig);
    return config->GetKeyspace() == config->GetDefaultKeyspace();
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

String DatabaseCassandra::GetVariableType(const std::shared_ptr
    <libobjgen::MetaVariable> var)
{
    switch(var->GetMetaType())
    {
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_STRING:
            return "text";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_BOOL:
            return "boolean";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S8:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S16:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S32:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U8:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U16:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_ENUM:
            return "int";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S64:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U32:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U64:
            return "bigint";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT:
            return "float";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE:
            return "double";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_REF:
            return "uuid";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_ARRAY:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_LIST:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_MAP:
        default:
            break;
    }

    return "blob";
}

CassSession* DatabaseCassandra::GetSession() const
{
    return mSession;
}
