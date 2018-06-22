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

#include "Database.h"

// libcomp Includes
#include "BaseServer.h"
#include "DataStore.h"
#include "Log.h"
#include "ScriptEngine.h"

using namespace libcomp;

Database::Database(const std::shared_ptr<objects::DatabaseConfig>& config)
{
    mConfig = config;
}

Database::~Database()
{
}

bool Database::Execute(const String& query)
{
    return Prepare(query).Execute();
}

String Database::GetLastError()
{
    return mError;
}

std::shared_ptr<objects::DatabaseConfig> Database::GetConfig() const
{
    return mConfig;
}

bool Database::TableHasRows(const String& table)
{
    libcomp::DatabaseQuery query = Prepare(String(
        "SELECT COUNT(1) FROM %1").Arg(table));

    if(!query.IsValid())
    {
        return false;
    }

    if(!query.Execute())
    {
        return false;
    }

    if(!query.Next())
    {
        return false;
    }

    int64_t count;

    if(!query.GetValue(0, count))
    {
        return false;
    }

    return 0 < count;
}

std::shared_ptr<PersistentObject> Database::LoadSingleObject(size_t typeHash,
    DatabaseBind *pValue)
{
    auto objects = LoadObjects(typeHash, pValue);

    return objects.size() > 0 ? objects.front() : nullptr;
}

bool Database::DeleteSingleObject(std::shared_ptr<PersistentObject>& obj)
{
    std::list<std::shared_ptr<PersistentObject>> objs;
    objs.push_back(obj);
    return DeleteObjects(objs);
}

void Database::QueueInsert(std::shared_ptr<PersistentObject> obj,
    const libobjgen::UUID& uuid)
{
    auto s = DatabaseChangeSet::Create(uuid);
    s->Insert(obj);
    QueueChangeSet(s);
}

void Database::QueueUpdate(std::shared_ptr<PersistentObject> obj,
    const libobjgen::UUID& uuid)
{
    auto s = DatabaseChangeSet::Create(uuid);
    s->Update(obj);
    QueueChangeSet(s);
}

void Database::QueueDelete(std::shared_ptr<PersistentObject> obj,
    const libobjgen::UUID& uuid)
{
    auto s = DatabaseChangeSet::Create(uuid);
    s->Delete(obj);
    QueueChangeSet(s);
}

bool Database::QueueChangeSet(const std::shared_ptr<
    DatabaseChangeSet>& changes)
{
    auto uuid = changes->GetTransactionUUID();
    std::string key = uuid.ToString();

    auto opChanges = std::dynamic_pointer_cast<
        DBOperationalChangeSet>(changes);
    if(opChanges)
    {
        // Operational change queuing is not supported
        return false;
    }
    else
    {
        auto standardChanges = std::dynamic_pointer_cast<
            DBStandardChangeSet>(changes);

        if(standardChanges)
        {
            std::lock_guard<std::mutex> lock(mTransactionLock);
            auto queueEntry = mTransactionQueue[key];
            if(queueEntry == nullptr)
            {
                queueEntry = std::make_shared<DBStandardChangeSet>(uuid);
            }

            for(auto obj : standardChanges->GetInserts())
            {
                queueEntry->Insert(obj);
            }

            for(auto obj : standardChanges->GetUpdates())
            {
                queueEntry->Update(obj);
            }

            for(auto obj : standardChanges->GetDeletes())
            {
                queueEntry->Delete(obj);
            }

            mTransactionQueue[key] = queueEntry;

            return true;
        }
    }

    return false;
}

std::list<libobjgen::UUID> Database::ProcessTransactionQueue()
{
    std::list<libobjgen::UUID> failures;

    std::unordered_map<std::string,
        std::shared_ptr<DBStandardChangeSet>> queue;
    {
        std::lock_guard<std::mutex> lock(mTransactionLock);

        if(mTransactionQueue.size() == 0)
        {
            return failures;
        }

        queue = mTransactionQueue;
        mTransactionQueue.clear();
    }

    // Process the general queue transaction first
    auto nullKey = NULLUUID.ToString();
    if(queue.find(nullKey) != queue.end())
    {
        if(!ProcessChangeSet(queue[nullKey]))
        {
            failures.push_back(nullKey);
        }
        queue.erase(nullKey);
    }

    // Process the remaining transactions
    for(auto kv : queue)
    {
        if(!ProcessChangeSet(kv.second))
        {
            failures.push_back(kv.second->GetTransactionUUID());
        }
    }

    return failures;
}

bool Database::ProcessChangeSet(const std::shared_ptr<DatabaseChangeSet>& changes)
{
    auto opChanges = std::dynamic_pointer_cast<DBOperationalChangeSet>(changes);

    if(opChanges)
    {
        return ProcessOperationalChangeSet(opChanges);
    }
    else
    {
        auto standardChanges = std::dynamic_pointer_cast<DBStandardChangeSet>(changes);
        if(standardChanges)
        {
            return ProcessStandardChangeSet(standardChanges);
        }
    }

    return false;
}

bool Database::UsingDefaultDatabaseType()
{
    return mConfig->GetDatabaseType() == mConfig->GetDefaultDatabaseType();
}

std::shared_ptr<PersistentObject> Database::LoadSingleObjectFromRow(
    size_t typeHash, DatabaseQuery& query)
{
    bool isNew = false;

    std::shared_ptr<PersistentObject> obj;

    libobjgen::UUID uid;
    if(query.GetValue("UID", uid))
    {
        //If the object is already cached, refresh it
        obj = PersistentObject::GetObjectByUUID(uid);
    }

    if(nullptr == obj)
    {
        obj = PersistentObject::New(typeHash);
        isNew = true;
    }

    if(!obj->LoadDatabaseValues(query))
    {
        return nullptr;
    }

    if(isNew)
    {
        PersistentObject::Register(obj);
    }

    return obj;
}

std::vector<std::shared_ptr<libobjgen::MetaObject>> Database::GetMappedObjects()
{
    auto databaseType = mConfig->GetDatabaseType();
    std::vector<std::shared_ptr<libobjgen::MetaObject>> metaObjectTables;
    for(auto registrar : PersistentObject::GetRegistry())
    {
        std::string source = registrar.second->GetSourceLocation();
        if(source == databaseType || (source.length() == 0 && UsingDefaultDatabaseType()))
        {
            metaObjectTables.push_back(registrar.second);
        }
    }

    return metaObjectTables;
}

bool Database::ApplyMigration(const std::shared_ptr<BaseServer>& server,
    DataStore *pDataStore, const libcomp::String& migration,
    const libcomp::String& path)
{
    LOG_DEBUG(libcomp::String("Applying migration %1 to database.\n"
        ).Arg(migration));

    // Load the script.
    auto script = pDataStore->ReadFile(path);

    if(script.empty())
    {
        LOG_ERROR(libcomp::String("Failed to load migration script: "
            "%1\n").Arg(path));

        return false;
    }

    // Zero the string.
    script.push_back(0);

    // Create the script engine.
    auto engine = std::make_shared<ScriptEngine>();

    // Parse the script.
    if(!engine->Eval(&script[0], path))
    {
        LOG_ERROR(libcomp::String("Failed to run migration script: "
            "%1\n").Arg(path));

        return false;
    }

    // Pull in the Database module.
    engine->Import("database");

    // Run the up() function.
    auto ref = Sqrat::RootTable(engine->GetVM()).GetFunction(
        "up").Evaluate<bool>(shared_from_this(), server);

    if(!ref || !(*ref))
    {
        LOG_ERROR(libcomp::String("Migration script failed: "
            "%1\n").Arg(path));

        return false;
    }

    return true;
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<Database>()
    {
        if(!BindingExists("Database"))
        {
            Sqrat::Class<Database, Sqrat::NoConstructor<Database>> binding(
                mVM, "Database");
            Bind<Database>("Database", binding);

            // These are needed for some methods.
            Using<PersistentObject>();

            binding
                .Func("Open", &Database::Open)
                .Func("Close", &Database::Close)
                .Func("IsOpen", &Database::IsOpen)
                //.Func("Prepare", &Database::Prepare)
                .Func("Execute", &Database::Execute)
                .Func("Exists", &Database::Exists)
                .Func("Use", &Database::Use)
                .Func("TableHasRows", &Database::TableHasRows)
                .Func("TableExists", &Database::TableExists)
                .Func("GetLastError", &Database::GetLastError)
                .Func("InsertSingleObject", &Database::InsertSingleObject)
                .Func("UpdateSingleObject", &Database::UpdateSingleObject)
                .Func("DeleteSingleObject", &Database::DeleteSingleObject)
                ; // Last call to binding
        }

        return *this;
    }
}
