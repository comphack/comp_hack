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

// Standard C++11 Includes
#include <algorithm>

#ifdef _WIN32
#include <cstdio> // defines FILENAME_MAX
#include <direct.h>
#endif // _WIN32

// libcomp Includes
#include <DatabaseMariaDB.h>
#include <DatabaseSQLite3.h>
#include <Decrypt.h>
#include <Log.h>
#include <MessageInit.h>
#include <ServerCommandLineParser.h>
#include <ServerConstants.h>

// object Includes
#include <Account.h>

using namespace libcomp;

std::string BaseServer::sConfigPath;

BaseServer::BaseServer(const char *szProgram,
    std::shared_ptr<objects::ServerConfig> config,
    std::shared_ptr<ServerCommandLineParser> commandLine) :
    TcpServer("any", config->GetPort()), mConfig(config),
    mCommandLine(commandLine), mDataStore(szProgram)
{
}

bool BaseServer::Initialize()
{
    SetDiffieHellman(LoadDiffieHellman(
        mConfig->GetDiffieHellmanKeyPair()));

    if(nullptr == GetDiffieHellman())
    {
        LOG_WARNING("No DH key pair set in the config file, it will"
            " need to be generated on startup.\n");
    }

    if(mConfig->GetPort() == 0)
    {
        LOG_WARNING("No port specified.\n");
        return false;
    }

    LOG_DEBUG(libcomp::String("Port: %1\n").Arg(
        mConfig->GetPort()));

    libcomp::String constantsPath = mConfig->GetServerConstantsPath();
    if(constantsPath.IsEmpty())
    {
        constantsPath = libcomp::String("%1constants.xml")
            .Arg(GetConfigPath());
    }

    if(!libcomp::ServerConstants::Initialize(constantsPath))
    {
        LOG_CRITICAL(libcomp::String("Server side constants failed to load"
            " from file path: %1\n").Arg(constantsPath));

        return false;
    }

    if(0 == mConfig->DataStoreCount())
    {
        LOG_CRITICAL("At least one data store path must be specified.\n");

        return false;
    }

    if(!mDataStore.AddSearchPaths(mConfig->GetDataStore()))
    {
        return false;
    }

    switch(mConfig->GetDatabaseType())
    {
        case objects::ServerConfig::DatabaseType_t::SQLITE3:
            LOG_DEBUG("Using SQLite3 Database.\n");
            break;
        case objects::ServerConfig::DatabaseType_t::MARIADB:
            LOG_DEBUG("Using MariaDB Database.\n");
            break;
        default:
            LOG_CRITICAL("Invalid database type specified.\n");
            return false;
            break;
    }

    // Create the generic workers
    CreateWorkers();

    // Add the server as a system manager for libcomp::Message::Init.
    mMainWorker.AddManager(std::dynamic_pointer_cast<Manager>(
        shared_from_this()));

    // Get the message queue.
    auto msgQueue = mMainWorker.GetMessageQueue();

    // The message queue better exist!
    if(nullptr == msgQueue)
    {
        LOG_CRITICAL("Main message queue is missing.\n");

        return false;
    }

    // Add the init message into the main worker queue.
    msgQueue->Enqueue(new libcomp::Message::Init);

    return true;
}

void BaseServer::FinishInitialize()
{
}

std::shared_ptr<Database> BaseServer::GetDatabase(
    objects::ServerConfig::DatabaseType_t dbType,
    const EnumMap<objects::ServerConfig::DatabaseType_t,
    std::shared_ptr<objects::DatabaseConfig>>& configMap)
{
    auto configIter = configMap.find(dbType);
    bool configExists = configIter != configMap.end() &&
        configIter->second != nullptr;

    std::shared_ptr<Database> db;
    switch(dbType)
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
        case objects::ServerConfig::DatabaseType_t::MARIADB:
            if(configExists)
            {
                auto sqlConfig = std::dynamic_pointer_cast<
                    objects::DatabaseConfigMariaDB>(configIter->second);
                db = std::shared_ptr<libcomp::Database>(
                    new libcomp::DatabaseMariaDB(sqlConfig));
            }
            else
            {
                LOG_CRITICAL("No MariaDB Database configuration specified.\n");
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

    return db;
}

std::shared_ptr<Database> BaseServer::GetDatabase(
    const EnumMap<objects::ServerConfig::DatabaseType_t,
    std::shared_ptr<objects::DatabaseConfig>>& configMap, bool performSetup)
{
    auto dbType = mConfig->GetDatabaseType();

    std::shared_ptr<Database> db = GetDatabase(dbType, configMap);
    if(!db)
    {
        return nullptr;
    }

    bool initFailure = false;
    if(performSetup)
    {
        auto configIter = configMap.find(dbType);

        bool createMockData = configIter->second->GetMockData();
        initFailure = !db->Setup(createMockData);
        if(!initFailure && createMockData)
        {
            auto configFile = configIter->second->GetMockDataFilename();
            if(configFile.IsEmpty())
            {
                LOG_CRITICAL("Data mocking enabled but no setup file"
                    " specified.\n");
                initFailure = true;
            }
            else
            {
                std::string configPath = GetConfigPath() +
                    configFile.ToUtf8();
                if(!InsertDataFromFile(configPath, db))
                {
                    LOG_CRITICAL("Mock data failed to insert.\n");
                    initFailure = true;
                }
            }
        }
    }
    else
    {
        initFailure = !db->Use();
    }

    if(initFailure)
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

gsl::not_null<DataStore*> BaseServer::GetDataStore()
{
    return &mDataStore;
}

int BaseServer::Run()
{
    // Run the asycn worker in its own thread.
    mQueueWorker.Start("async_worker");

    // Run the main worker in this thread, blocking until done.
    mMainWorker.Start("main_worker", true);

    // Stop the network service (this will kill any existing connections).
    mService.stop();

    return 0;
}

void BaseServer::ServerReady()
{
    TcpServer::ServerReady();

    int32_t pid = mCommandLine->GetNotifyProcess();

    if(0 < pid)
    {
#ifndef _WIN32
        LOG_DEBUG(String("Sending startup notification to "
            "PID %1\n").Arg(pid));

        kill((pid_t)pid, SIGUSR2);
#endif // !_WIN32
    }
}

void BaseServer::Shutdown()
{
    mMainWorker.Shutdown();
    mQueueWorker.Shutdown();

    for(auto worker : mWorkers)
    {
        worker->Shutdown();
    }
}

std::string BaseServer::GetConfigPath()
{
    if(!sConfigPath.empty())
    {
        return sConfigPath;
    }
    else
    {
        return GetDefaultConfigPath();
    }
}

void BaseServer::SetConfigPath(const std::string& path)
{
    sConfigPath = path;
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
        return false;
    }
    else
    {
        auto log = libcomp::Log::GetSingletonPtr();

        log->SetLogLevelEnabled(libcomp::Log::LOG_LEVEL_DEBUG,
            config->GetLogDebug());
        log->SetLogLevelEnabled(libcomp::Log::LOG_LEVEL_INFO,
            config->GetLogInfo());
        log->SetLogLevelEnabled(libcomp::Log::LOG_LEVEL_WARNING,
            config->GetLogWarning());
        log->SetLogLevelEnabled(libcomp::Log::LOG_LEVEL_ERROR,
            config->GetLogError());
        log->SetLogLevelEnabled(libcomp::Log::LOG_LEVEL_CRITICAL,
            config->GetLogCritical());

        if(!config->GetLogFile().IsEmpty())
        {
            log->SetLogPath(config->GetLogFile(), !config->GetLogFileAppend());
            log->SetLogFileTimestampsEnabled(config->GetLogFileTimestamp());
        }
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
                // Leave one core for the main worker and one for the async worker
                numberOfWorkers = (numberOfWorkers == 2) ? 1 : (numberOfWorkers - 2);
                break;
        }
    }

    for(unsigned int i = 0; i < numberOfWorkers; i++)
    {
        auto worker = std::shared_ptr<Worker>(new Worker);
        worker->Start(libcomp::String("worker%1").Arg(i));
        mWorkers.push_back(worker);
    }
}

bool BaseServer::AssignMessageQueue(const std::shared_ptr<
    libcomp::EncryptedConnection>& connection)
{
    std::shared_ptr<libcomp::Worker> worker = mWorkers.size() != 1
        ? GetNextConnectionWorker() : mWorkers.front();

    if(!worker)
    {
        LOG_CRITICAL("The server failed to assign a worker to an incoming connection.\n");
        return false;
    }

    connection->SetMessageQueue(worker->GetMessageQueue());
    return true;
}

std::shared_ptr<libcomp::Worker> BaseServer::GetNextConnectionWorker()
{
    //By default return the least busy worker by checking shared_ptr message queue use count
    long leastConnections = 0;
    std::shared_ptr<libcomp::Worker> leastBusy = nullptr;
    for(auto worker : mWorkers)
    {
        long refCount = worker->AssignmentCount();
        if(nullptr == leastBusy || refCount < leastConnections)
        {
            leastConnections = refCount;

            leastBusy = worker;
            if(refCount <= 2)
            {
                //Only ref is within the worker and the thread
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

std::list<libcomp::Message::MessageType> BaseServer::GetSupportedTypes() const
{
    static std::list<libcomp::Message::MessageType> supportedTypes = {
        libcomp::Message::MessageType::MESSAGE_TYPE_SYSTEM
    };

    return supportedTypes;
}

bool BaseServer::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    auto msg = dynamic_cast<const libcomp::Message::Init*>(pMessage);

    // Check if this is an init message.
    if(nullptr != msg)
    {
        FinishInitialize();

        return true;
    }

    return false;
}

bool BaseServer::InsertDataFromFile(const libcomp::String& filePath,
    const std::shared_ptr<Database>& db, const std::set<std::string>& specificTypes)
{
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS != doc.LoadFile(filePath.C()))
    {
        return false;
    }

    LOG_DEBUG(libcomp::String("Inserting records from file '%1'...\n").Arg(filePath));

    const tinyxml2::XMLElement *objXml = doc.RootElement()->FirstChildElement("object");

    while(nullptr != objXml)
    {
        bool typeExists = true;

        std::string name(objXml->Attribute("name"));
        if(specificTypes.size() > 0 && specificTypes.find(name) == specificTypes.end())
        {
            continue;
        }

        auto typeHash = libcomp::PersistentObject::GetTypeHashByName(name, typeExists);
        if(!typeExists)
        {
            return false;
        }

        // Retrieve the UID if specified
        libobjgen::UUID uuid;
        const tinyxml2::XMLElement *memberXml = objXml->FirstChildElement("member");
        while(nullptr != memberXml)
        {
            if("uid" == libcomp::String(memberXml->Attribute("name")).ToLower())
            {
                std::string uuidText(memberXml->GetText());
                uuid = libobjgen::UUID(uuidText);
                if(uuid.IsNull())
                {
                    return false;
                }
                break;
            }
            memberXml = memberXml->NextSiblingElement("member");
        }

        auto record = libcomp::PersistentObject::New(typeHash);
        if(!record->Load(doc, *objXml))
        {
            return false;
        }

        if(name == "Account")
        {
            // Check that there is a password and salt it
            auto account = std::dynamic_pointer_cast<objects::Account>(record);

            if(account->GetUsername().IsEmpty() ||
                account->GetPassword().IsEmpty())
            {
                LOG_ERROR("Attempted to insert an account with no username"
                    " or no password.\n");
                return false;
            }

            libcomp::String salt = libcomp::Decrypt::GenerateRandom(10);
            account->SetSalt(salt);
            account->SetPassword(libcomp::Decrypt::HashPassword(
                account->GetPassword(), salt));
        }

        if(!record->Register(record, uuid) ||
            !record->Insert(db))
        {
            return false;
        }

        // Don't cache the records until they are needed
        record->Unregister();

        objXml = objXml->NextSiblingElement("object");
    }

    // Allow no records as a means to clear out the DB on restart
    return true;
}
