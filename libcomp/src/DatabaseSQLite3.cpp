/**
 * @file libcomp/src/DatabaseSQLite3.cpp
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

#include "DatabaseSQLite3.h"

// libcomp Includes
#include "DatabaseBind.h"
#include "DatabaseQuerySQLite3.h"
#include "Log.h"

// libobjgen Includes
#include <MetaVariableString.h>

// SQLite3 Includes
#include <sqlite3.h>

using namespace libcomp;

DatabaseSQLite3::DatabaseSQLite3(const std::shared_ptr<
    objects::DatabaseConfigSQLite3>& config) :
    Database(std::dynamic_pointer_cast<objects::DatabaseConfig>(config)),
    mDatabase(nullptr)
{
}

DatabaseSQLite3::~DatabaseSQLite3()
{
    Close();
}

bool DatabaseSQLite3::Open()
{
    auto filepath = GetFilepath();

    bool result = true;

    if(SQLITE_OK != sqlite3_open(filepath.C(), &mDatabase))
    {
        result = false;

        LOG_ERROR(String("Failed to open database connection: %1\n").Arg(
            sqlite3_errmsg(mDatabase)));

        (void)Close();
    }

    return result;
}

bool DatabaseSQLite3::Close()
{
    bool result = true;

    if(nullptr != mDatabase)
    {
        if(SQLITE_OK != sqlite3_close(mDatabase))
        {
            result = false;

            LOG_ERROR("Failed to close database connection.\n");
        }

        mDatabase = nullptr;
    }

    return result;
}

bool DatabaseSQLite3::IsOpen() const
{
    return nullptr != mDatabase;
}

DatabaseQuery DatabaseSQLite3::Prepare(const String& query)
{
    return DatabaseQuery(new DatabaseQuerySQLite3(mDatabase), query);
}

bool DatabaseSQLite3::Exists()
{
    auto filepath = GetFilepath();

    // Check if the database file exists
    if(FILE *file = fopen(filepath.C(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}

bool DatabaseSQLite3::Setup()
{
    if(!IsOpen())
    {
        LOG_ERROR("Trying to setup a database that is not open!\n");

        return false;
    }

    auto filename = std::dynamic_pointer_cast<objects::DatabaseConfigSQLite3>(
        mConfig)->GetDatabaseName();
    if(!Exists())
    {
        LOG_ERROR("Database file was not created!\n");
    }

    if(UsingDefaultDatabaseFile())
    {
        std::list<std::unordered_map<std::string, std::vector<char>>> results;
        auto q = Prepare("SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'objects';");
        if(!q.IsValid() || !q.Execute() || !q.GetRows(results))
        {
            LOG_ERROR("Failed to query the master table for schema.\n");

            return false;
        }
        
        if(results.size() == 0 &&
            !Execute("CREATE TABLE objects (uid string PRIMARY KEY, member_vars blob);"))
        {
            LOG_ERROR("Failed to create the objects table.\n");

            return false;
        }
    }

    LOG_DEBUG(String("Database connection established to '%1' file.\n")
        .Arg(filename));

    if(!VerifyAndSetupSchema())
    {
        LOG_ERROR("Schema verification and setup failed.\n");

        return false;
    }

    return true;
}

bool DatabaseSQLite3::Use()
{
    // Since each database is its own file there is nothing to do here.
    return true;
}

std::list<std::shared_ptr<PersistentObject>> DatabaseSQLite3::LoadObjects(
    std::type_index type, DatabaseBind *pValue)
{
    std::list<std::shared_ptr<PersistentObject>> objects;

    auto metaObject = PersistentObject::GetRegisteredMetadata(type);

    if(nullptr == metaObject)
    {
        LOG_ERROR("Failed to lookup MetaObject.\n");

        return {};
    }

    String sql = String("SELECT * FROM %1 WHERE %2 = :%2").Arg(
        metaObject->GetName()).Arg(
        pValue->GetColumn());

    DatabaseQuery query = Prepare(sql);

    if(!query.IsValid())
    {
        LOG_ERROR(String("Failed to prepare SQL query: %1\n").Arg(sql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return {};
    }

    if(!pValue->Bind(query))
    {
        LOG_ERROR(String("Failed to bind value: %1\n").Arg(
            pValue->GetColumn()));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return {};
    }

    if(!query.Execute())
    {
        LOG_ERROR(String("Failed to execute query: %1\n").Arg(sql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return {};
    }

    int failures = 0;

    while(query.Next())
    {
        auto obj = LoadSingleObjectFromRow(type, query);

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

bool DatabaseSQLite3::InsertSingleObject(std::shared_ptr<PersistentObject>& obj)
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
    columnBinds.push_back(String("'%1'").Arg(obj->GetUUID().ToString()));

    auto values = obj->GetMemberBindValues();

    for(auto value : values)
    {
        auto columnName = value->GetColumn();
        columnNames.push_back(columnName);
        columnBinds.push_back(String(":%1").Arg(columnName));
    }

    String sql = String("INSERT INTO %1 (%2) VALUES (%3);").Arg(
        metaObject->GetName()).Arg(
        String::Join(columnNames, ", ")).Arg(
        String::Join(columnBinds, ", "));

    DatabaseQuery query = Prepare(sql);

    if(!query.IsValid())
    {
        LOG_ERROR(String("Failed to prepare SQL query: %1\n").Arg(sql));
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
        LOG_ERROR(String("Failed to execute query: %1\n").Arg(sql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    return true;
}

bool DatabaseSQLite3::UpdateSingleObject(std::shared_ptr<PersistentObject>& obj)
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
        columnNames.push_back(String("%1 = :%1").Arg(value->GetColumn()));
    }

    String sql = String("UPDATE %1 SET %2 WHERE uid = '%3';").Arg(
        metaObject->GetName()).Arg(
        String::Join(columnNames, ", ")).Arg(
        obj->GetUUID().ToString());

    DatabaseQuery query = Prepare(sql);

    if(!query.IsValid())
    {
        LOG_ERROR(String("Failed to prepare SQL query: %1\n").Arg(sql));
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
        LOG_ERROR(String("Failed to execute query: %1\n").Arg(sql));
        LOG_ERROR(String("Database said: %1\n").Arg(GetLastError()));

        return false;
    }

    return true;
}

bool DatabaseSQLite3::DeleteSingleObject(std::shared_ptr<PersistentObject>& obj)
{
    auto uuid = obj->GetUUID();
    if(uuid.IsNull())
    {
        return false;
    }

    std::string uuidStr = obj->GetUUID().ToString();

    auto metaObject = obj->GetObjectMetadata();

    if (Execute(String("DELETE FROM %1 WHERE uid = '%2';")
        .Arg(metaObject->GetName())
        .Arg(uuidStr)))
    {
        obj->Unregister();
        return true;
    }

    return false;
}

bool DatabaseSQLite3::VerifyAndSetupSchema()
{
    auto databaseName = std::dynamic_pointer_cast<objects::DatabaseConfigSQLite3>(
        mConfig)->GetDatabaseName();
    std::vector<std::shared_ptr<libobjgen::MetaObject>> metaObjectTables;
    for(auto registrar : PersistentObject::GetRegistry())
    {
        std::string source = registrar.second->GetSourceLocation();
        if(source == databaseName || (source.length() == 0 && UsingDefaultDatabaseFile()))
        {
            metaObjectTables.push_back(registrar.second);
        }
    }

    if(metaObjectTables.size() == 0)
    {
        return true;
    }

    LOG_DEBUG("Verifying database table structure.\n");

    DatabaseQuery q = Prepare("SELECT name, type, tbl_name FROM sqlite_master"
        " where type in ('table', 'index') and name <> 'objects';");
    if(!q.Execute())
    {
        LOG_CRITICAL("Failed to query for existing columns.\n");

        return false;
    }

    std::unordered_map<std::string,
        std::unordered_map<std::string, String>> fieldMap;
    std::unordered_map<std::string, std::set<std::string>> indexedFields;
    while(q.Next())
    {
        String name;
        String type;

        if(!q.GetValue("name", name) || !q.GetValue("type", type))
        {
            LOG_CRITICAL(String("Invalid query results returned from sqlite_master table.\n")
                .Arg(name));

            return false;
        }

        if(type == "table")
        {
            std::unordered_map<std::string, String>& m = fieldMap[name.ToUtf8()];

            DatabaseQuery sq = Prepare(String("PRAGMA table_info('%1');").Arg(name));
            if(!sq.Execute() || !sq.Next())
            {
                LOG_CRITICAL(String("Failed to query for '%1' columns.\n").Arg(name));

                return false;
            }

            do
            {
                String colName;
                String dataType;

                if(sq.GetValue("name", colName) && sq.GetValue("type", dataType))
                {
                    m[colName.ToUtf8()] = dataType;
                }
            } while (sq.Next());
        }
        else if(type == "index")
        {
            String tableName;

            if(q.GetValue("tbl_name", tableName))
            {
                std::set<std::string>& s = indexedFields[tableName.ToUtf8()];
                s.insert(name.ToUtf8());
            }
        }
    }
    
    for(auto metaObjectTable : metaObjectTables)
    {
        auto metaObject = *metaObjectTable.get();
        auto objName = metaObject.GetName();

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
                    auto name = var->GetName();
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
                        needsIndex.insert(name);
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
                << " (uid blob PRIMARY KEY";
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

                auto indexStr = String("idx_%1_%2")
                    .Arg(objName).Arg(var->GetName());

                auto cmd = String("CREATE INDEX %1 ON %2(%3);")
                    .Arg(indexStr).Arg(objName).Arg(var->GetName());

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

bool DatabaseSQLite3::UsingDefaultDatabaseFile()
{
    auto config = std::dynamic_pointer_cast<objects::DatabaseConfigSQLite3>(mConfig);
    return config->GetDatabaseName() == config->GetDefaultDatabaseName();
}

String DatabaseSQLite3::GetFilepath() const
{
    auto config = std::dynamic_pointer_cast<objects::DatabaseConfigSQLite3>(mConfig);
    auto directory = config->GetFileDirectory();
    auto filename = config->GetDatabaseName();

    return String("%1%2.sqlite3").Arg(directory).Arg(filename);
}

String DatabaseSQLite3::GetVariableType(const std::shared_ptr
    <libobjgen::MetaVariable> var)
{
    switch(var->GetMetaType())
    {
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_STRING:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_REF:
            return "string";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_BOOL:
            return "bit";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S8:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S16:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S32:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U8:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U16:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_ENUM:
            return "int";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U32:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_S64:
            return "bigint";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT:
            return "float";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE:
            return "double";
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_U64:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_ARRAY:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_LIST:
        case libobjgen::MetaVariable::MetaVariableType_t::TYPE_MAP:
        default:
            break;
    }

    return "blob";
}
