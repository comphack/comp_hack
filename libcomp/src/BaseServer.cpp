/**
 * @file libcomp/src/BaseServer.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base server class.
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

#include "BaseServer.h"

#ifdef _WIN32
 // Standard C++ Includes
#include <algorithm>
#include <stdio.h>  /* defines FILENAME_MAX */
#include <direct.h>
#endif

#include <DatabaseCassandra.h>
#include <DatabaseSQLite3.h>
#include "Log.h"

using namespace libcomp;

BaseServer::BaseServer(std::shared_ptr<objects::ServerConfig> config, const String& configPath) :
    TcpServer("any", config->GetPort()), mConfig(config)
{
    ReadConfig(config, configPath);
}

bool BaseServer::Initialize(std::weak_ptr<BaseServer>& self)
{
    mSelf = self;
    switch(mConfig->GetDatabaseType())
    {
        case objects::ServerConfig::DatabaseType_t::SQLITE3:
            LOG_DEBUG("Using SQLite3 Database.\n");
            break;
        case objects::ServerConfig::DatabaseType_t::CASSANDRA:
            LOG_DEBUG("Using Cassandra Database.\n");
            break;
        default:
            LOG_CRITICAL("Invalid database type specified.\n");
            return false;
            break;
    }

    // Create the generic workers
    CreateWorkers();

    return true;
}

std::shared_ptr<Database> BaseServer::GetDatabase(
    const EnumMap<objects::ServerConfig::DatabaseType_t,
    std::shared_ptr<objects::DatabaseConfig>>& configMap, bool performSetup)
{
    auto type = mConfig->GetDatabaseType();
    
    auto configIter = configMap.find(type);
    bool configExists = configIter != configMap.end() &&
        configIter->second != nullptr;

    std::shared_ptr<Database> db;
    switch(type)
    {
        case objects::ServerConfig::DatabaseType_t::SQLITE3:
            if(configExists)
            {
                auto sqlConfig = std::dynamic_pointer_cast<
                    objects::DatabaseConfigSQLite3>(configIter->second);
                db = std::shared_ptr<libcomp::Database>(
                    new libcomp::DatabaseSQLite3(sqlConfig));
            }
            else
            {
                LOG_CRITICAL("No SQLite3 Database configuration specified.\n");
            }
            break;
        case objects::ServerConfig::DatabaseType_t::CASSANDRA:
            if(configExists)
            {
                auto cassandraConfig = std::dynamic_pointer_cast<
                    objects::DatabaseConfigCassandra>(configIter->second);
                db = std::shared_ptr<libcomp::Database>(
                    new libcomp::DatabaseCassandra(cassandraConfig));
            }
            else
            {
                LOG_CRITICAL("No Cassandra Database configuration specified.\n");
            }
            break;
        default:
            LOG_CRITICAL("Invalid database type specified.\n");
            break;
    }

    if(!db)
    {
        return nullptr;
    }

    // Open the database.
    if(!db->Open() || !db->IsOpen())
    {
        LOG_CRITICAL("Failed to open database.\n");
        return nullptr;
    }

    if(performSetup && !db->Setup())
    {
        LOG_CRITICAL("Failed to init database.\n");
        return nullptr;
    }

    return db;
}

BaseServer::~BaseServer()
{
    // Make sure the worker threads stop.
    for(auto worker : mWorkers)
    {
        worker->Join();
    }
    mWorkers.clear();
}

int BaseServer::Run()
{
    // Run the main worker in this thread, blocking until done.
    mMainWorker.Start(true);

    // Stop the network service (this will kill any existing connections).
    mService.stop();

    return 0;
}

void BaseServer::Shutdown()
{
    mMainWorker.Shutdown();

    for(auto worker : mWorkers)
    {
        worker->Shutdown();
    }
}

std::string BaseServer::GetDefaultConfigPath()
{
#ifdef _WIN32
    char buff[FILENAME_MAX];
    auto result = _getcwd(buff, FILENAME_MAX);
    std::string executingDirectory(buff);

    std::string filePath = executingDirectory + "\\config\\";
#else
    std::string filePath = "/etc/comp_hack/";
#endif

    return filePath;
}

bool BaseServer::ReadConfig(std::shared_ptr<objects::ServerConfig> config, libcomp::String filePath)
{
    tinyxml2::XMLDocument doc;
    if (tinyxml2::XML_SUCCESS != doc.LoadFile(filePath.C()))
    {
        LOG_WARNING(libcomp::String("Failed to parse config file: %1\n").Arg(
            filePath));
        return false;
    }
    else
    {
        LOG_DEBUG(libcomp::String("Reading config file: %1\n").Arg(
            filePath));
        return ReadConfig(config, doc);
    }
}

bool BaseServer::ReadConfig(std::shared_ptr<objects::ServerConfig> config, tinyxml2::XMLDocument& doc)
{
    const tinyxml2::XMLElement *pRoot = doc.RootElement();
    const tinyxml2::XMLElement *pObject = nullptr;

    if(nullptr != pRoot)
    {
        pObject = pRoot->FirstChildElement("object");
    }

    if(nullptr == pObject || !config->Load(doc, *pObject))
    {
        LOG_WARNING("Failed to load config file\n");
        return false;
    }
    else
    {
        //Set the shared members
        LOG_DEBUG(libcomp::String("DH Pair: %1\n").Arg(
            config->GetDiffieHellmanKeyPair()));

        SetDiffieHellman(LoadDiffieHellman(
            config->GetDiffieHellmanKeyPair()));

        if(nullptr == GetDiffieHellman())
        {
            LOG_WARNING("Failed to load DH key pair from config file\n");
            return false;
        }

        if(config->GetPort() == 0)
        {
            LOG_WARNING("No port specified\n");
            return false;
        }

        LOG_DEBUG(libcomp::String("Port: %1\n").Arg(
            config->GetPort()));
    }

    return true;
}

void BaseServer::CreateWorkers()
{
    unsigned int numberOfWorkers = 1;
    if(mConfig->GetMultithreadMode())
    {
        numberOfWorkers = std::thread::hardware_concurrency();
        switch(numberOfWorkers)
        {
            case 0:
                LOG_WARNING("The maximum hardware concurrency level of this"
                    " machine could not be detected. Multi-threaded processing"
                    " will be disabled.\n");
                numberOfWorkers = 1;
                break;
            case 1:
                break;
            default:
                // Leave one core for the main worker
                numberOfWorkers = numberOfWorkers - 1;
                break;
        }
    }

    for(unsigned int i = 0; i < numberOfWorkers; i++)
    {
        mWorkers.push_back(std::shared_ptr<Worker>(new Worker));
    }
}

bool BaseServer::AssignMessageQueue(std::shared_ptr<libcomp::EncryptedConnection>& connection)
{
    std::shared_ptr<libcomp::Worker> worker = mWorkers.size() != 1
        ? GetNextConnectionWorker() : mWorkers.front();

    if(!worker)
    {
        LOG_CRITICAL("The server failed to assign a worker to an incoming connection.\n");
        return false;
    }
    else if(!worker->IsRunning())
    {
        //Only spin up as needed
        LOG_DEBUG("Starting a new connection worker.\n");
        worker->Start();
    }

    connection->SetMessageQueue(worker->GetMessageQueue());
    return true;
}

std::shared_ptr<libcomp::Worker> BaseServer::GetNextConnectionWorker()
{
    //By default return the least busy worker by checking shared_ptr message queue use count
    long leastConnections = (long)mConnections.size() + 2;
    std::shared_ptr<libcomp::Worker> leastBusy = nullptr;
    for(auto worker : mWorkers)
    {
        long refCount = worker->AssignmentCount();
        if(refCount < leastConnections)
        {
            leastConnections = refCount;

            leastBusy = worker;
            if(refCount == 1)
            {
                //Only ref is within the worker
                break;
            }
        }
    }

    return leastBusy;
}

std::shared_ptr<objects::ServerConfig> BaseServer::GetConfig() const
{
    return mConfig;
}
