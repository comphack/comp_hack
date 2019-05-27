/**
 * @file server/lobby/src/LobbyServer.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby server class.
 *
 * This file is part of the Lobby Server (lobby).
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

#include "LobbyServer.h"

// libcomp Includes
#include <DatabaseConfigMariaDB.h>
#include <DatabaseConfigSQLite3.h>
#include <Decrypt.h>
#include <Log.h>
#include <PacketCodes.h>

// Object Includes
#include "Account.h"
#include "Character.h"
#include "LobbyConfig.h"
#include "RegisteredWorld.h"

// Standard C++11 Includes
#include <iostream>

// lobby Includes
#include "AccountManager.h"
#include "LobbyClientConnection.h"
#include "LobbySyncManager.h"
#include "ManagerClientPacket.h"
#include "ManagerConnection.h"
#include "Packets.h"

using namespace lobby;

LobbyServer::LobbyServer(const char *szProgram,
    std::shared_ptr<objects::ServerConfig> config,
    std::shared_ptr<libcomp::ServerCommandLineParser> commandLine,
    bool unitTestMode) : libcomp::BaseServer(szProgram, config, commandLine),
    mUnitTestMode(unitTestMode), mAccountManager(nullptr),
    mSyncManager(nullptr)
{
}

bool LobbyServer::Initialize()
{
    auto self = std::dynamic_pointer_cast<LobbyServer>(shared_from_this());

    if(!BaseServer::Initialize())
    {
        return false;
    }

    auto conf = std::dynamic_pointer_cast<objects::LobbyConfig>(mConfig);

    libcomp::EnumMap<objects::ServerConfig::DatabaseType_t,
        std::shared_ptr<objects::DatabaseConfig>> configMap;

    configMap[objects::ServerConfig::DatabaseType_t::SQLITE3]
        = conf->GetSQLite3Config();

    configMap[objects::ServerConfig::DatabaseType_t::MARIADB]
        = conf->GetMariaDBConfig();

    mDatabase = GetDatabase(configMap, true, GetDataStore(),
        "/migrations/lobby");

    if(nullptr == mDatabase)
    {
        return false;
    }

    if(!mUnitTestMode && !mDatabase->TableHasRows("Account"))
    {
        if(!Setup())
        {
            CreateFirstAccount();
        }
    }

    mManagerConnection = std::make_shared<ManagerConnection>(
        self, &mService, mMainWorker.GetMessageQueue());

    mAccountManager = new AccountManager(this);
    mSyncManager = new LobbySyncManager(self);

    if(!mSyncManager->Initialize())
    {
        return false;
    }

    // Reset the RegisteredWorld table and pull information from
    // known worlds into the connection manager
    if(!ResetRegisteredWorlds())
    {
        return false;
    }

    auto internalPacketManager = std::make_shared<libcomp::ManagerPacket>(self);
    internalPacketManager->AddParser<Parsers::SetWorldInfo>(
        to_underlying(InternalPacketCode_t::PACKET_SET_WORLD_INFO));
    internalPacketManager->AddParser<Parsers::SetChannelInfo>(
        to_underlying(InternalPacketCode_t::PACKET_SET_CHANNEL_INFO));
    internalPacketManager->AddParser<Parsers::AccountLogin>(
        to_underlying(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN));
    internalPacketManager->AddParser<Parsers::AccountLogout>(
        to_underlying(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT));
    internalPacketManager->AddParser<Parsers::DataSync>(to_underlying(
        InternalPacketCode_t::PACKET_DATA_SYNC));
    internalPacketManager->AddParser<Parsers::WebGame>(
        to_underlying(InternalPacketCode_t::PACKET_WEB_GAME));

    //Add the managers to the main worker.
    mMainWorker.AddManager(internalPacketManager);
    mMainWorker.AddManager(mManagerConnection);

    auto clientPacketManager = std::make_shared<ManagerClientPacket>(self);
    clientPacketManager->AddParser<Parsers::Login>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_LOGIN));
    clientPacketManager->AddParser<Parsers::Auth>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_AUTH));
    clientPacketManager->AddParser<Parsers::StartGame>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_START_GAME));
    clientPacketManager->AddParser<Parsers::CharacterList>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_CHARACTER_LIST));
    clientPacketManager->AddParser<Parsers::WorldList>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_WORLD_LIST));
    clientPacketManager->AddParser<Parsers::CreateCharacter>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_CREATE_CHARACTER));
    clientPacketManager->AddParser<Parsers::DeleteCharacter>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_DELETE_CHARACTER));
    clientPacketManager->AddParser<Parsers::QueryPurchaseTicket>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_QUERY_PURCHASE_TICKET));
    clientPacketManager->AddParser<Parsers::PurchaseTicket>(to_underlying(
        ClientToLobbyPacketCode_t::PACKET_PURCHASE_TICKET));

    // Add the managers to the generic workers.
    for(auto worker : mWorkers)
    {
        worker->AddManager(clientPacketManager);
        worker->AddManager(mManagerConnection);
    }

    return true;
}

LobbyServer::~LobbyServer()
{
    delete mAccountManager;
    delete mSyncManager;
}

std::list<std::shared_ptr<lobby::World>> LobbyServer::GetWorlds() const
{
    return mManagerConnection->GetWorlds();
}

std::shared_ptr<lobby::World> LobbyServer::GetWorldByID(uint8_t worldID)
{
    return mManagerConnection->GetWorldByID(worldID);
}

std::shared_ptr<lobby::World> LobbyServer::GetWorldByConnection(
    std::shared_ptr<libcomp::InternalConnection> connection)
{
    return mManagerConnection->GetWorldByConnection(connection);
}

const std::shared_ptr<lobby::World> LobbyServer::RegisterWorld(
    std::shared_ptr<lobby::World>& world)
{
    return mManagerConnection->RegisterWorld(world);
}

void LobbyServer::SendWorldList(const std::shared_ptr<
    libcomp::TcpConnection>& connection) const
{
    libcomp::Packet p;
    p.WritePacketCode(LobbyToClientPacketCode_t::PACKET_WORLD_LIST);

    auto worlds = GetWorlds();
    worlds.remove_if([](const std::shared_ptr<lobby::World>& world)
        {
            return world->GetRegisteredWorld()->GetStatus()
                == objects::RegisteredWorld::Status_t::INACTIVE;
        });

    // World count.
    p.WriteU8((uint8_t)worlds.size());

    // Add each world to the list.
    for(auto world : worlds)
    {
        auto worldServer = world->GetRegisteredWorld();

        // ID for this world.
        p.WriteU8(worldServer->GetID());

        // Name of the world.
        p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            worldServer->GetName(), true);

        auto channels = world->GetChannels();

        // Number of channels on this world.
        p.WriteU8((uint8_t)channels.size());

        // Add each channel for this world.
        for(auto channel : channels)
        {
            // Name of the channel. This used to be displayed in the channel
            // list that was hidden from the user.
            p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                channel->GetName(), true);

            // Ping time??? Again, something that used to be in the list.
            p.WriteU16Little(1);

            // 0 - Visible | 2 - Hidden (or PvP)
            // Pointless without the list.
            p.WriteU8(0);
        }
    }

    if(nullptr == connection)
    {
        // Send to all client connections
        auto connections = mManagerConnection->GetClientConnections();
        libcomp::TcpConnection::BroadcastPacket(connections, p);
    }
    else
    {
        connection->SendPacket(p);
    }
}

std::shared_ptr<libcomp::Database> LobbyServer::GetMainDatabase() const
{
    return mDatabase;
}

std::shared_ptr<ManagerConnection> LobbyServer::GetManagerConnection() const
{
    return mManagerConnection;
}

std::shared_ptr<libcomp::TcpConnection> LobbyServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    static int connectionID = 0;

    auto connection = std::make_shared<LobbyClientConnection>(
        socket, CopyDiffieHellman(GetDiffieHellman()));

    // Set a unique connection ID for the name of the connection.
    connection->SetName(libcomp::String("client:%1").Arg(connectionID++));

    if(AssignMessageQueue(connection))
    {
        // Give the connection a new client state object.
        connection->SetClientState(std::make_shared<ClientState>());

        // Make sure this is called after connecting.
        connection->ConnectionSuccess();
    }
    else
    {
        connection->Close();

        return nullptr;
    }

    return connection;
}

void LobbyServer::CreateFirstAccount()
{
    bool again = true;

    do
    {
        PromptCreateAccount();

        LOG_INFO("Create another account? [y/N] ");

        std::string val;
        std::cin >> val;

        again = val.length() >= 1 && tolower(val.at(0)) == 'y';
    } while(again);
}

void LobbyServer::PromptCreateAccount()
{
    libcomp::String username;
    libcomp::String password;
    libcomp::String email = "no.thanks@bother_me_not.net";
    libcomp::String displayName = "AnonymousCoward";
    libcomp::String salt = libcomp::Decrypt::GenerateRandom(10);

    auto conf = std::dynamic_pointer_cast<objects::LobbyConfig>(mConfig);

    uint32_t cp = conf->GetRegistrationCP();
    uint8_t ticketCount = conf->GetRegistrationTicketCount();
    int32_t userLevel = conf->GetRegistrationUserLevel();
    bool enabled = conf->GetRegistrationAccountEnabled();

    do
    {
        LOG_INFO("Username: ");

        std::string uname;
        std::cin >> uname;

        username = libcomp::String(uname).ToLower();

        /// @todo Use a regular expression.
        if(3 > username.Length())
        {
            LOG_ERROR("Username is not valid.\n");
        }
    } while(3 > username.Length());

    while(true)
    {
#ifdef WIN32
        LOG_INFO("Password: ");

        std::string pass;
        std::cin >> pass;

        libcomp::String password1 = libcomp::String(pass);
#else // !WIN32
        libcomp::String password1 = libcomp::String(getpass("Password: "));
#endif // WIN32

        while(8 > password1.Length())
        {
            LOG_ERROR("Account password must be at "
                "least 8 characters.\n");

#ifdef WIN32
            LOG_INFO("Password: ");

            std::cin >> pass;

            password1 = libcomp::String(pass);
#else // !WIN32
            password1 = libcomp::String(getpass("Password: "));
#endif // WIN32
        }

#ifdef WIN32
        LOG_INFO("Verify Password: ");

        std::cin >> pass;

        libcomp::String password2 = libcomp::String(pass);
#else // !WIN32
        libcomp::String password2 = libcomp::String(
            getpass("Verify Password: "));
#endif // WIN32

        if(password1 == password2)
        {
            password = libcomp::Decrypt::HashPassword(password1, salt);
            break;
        }

        LOG_ERROR("Account password did not match.\n");
    }

    LOG_INFO("Default values will be used for this account unless you "
        "enter\nmore details. Would you like to enter more details? [y/N] ");

    std::string details;
    std::cin >> details;

    if(details.length() >= 1 && tolower(details.at(0)) == 'y')
    {
        libcomp::String enteredDisplayName;
        libcomp::String enteredEmail;
        uint8_t enteredTicketCount = 0;
        uint32_t enteredCP = static_cast<uint32_t>(-1);
        int32_t enteredUserLevel = -1;

        while(3 > enteredDisplayName.Length())
        {
            LOG_INFO("Display name: ");

            std::string val;
            std::cin >> val;

            enteredDisplayName = val;

            if(3 > enteredDisplayName.Length())
            {
                LOG_ERROR("You must enter a longer display name.\n");
            }
        }

        /// @todo Make this a better check for a valid email.
        while(!enteredEmail.Contains("@"))
        {
            LOG_INFO("Email: ");

            std::string val;
            std::cin >> val;

            enteredEmail = val;

            if(!enteredEmail.Contains("@"))
            {
                LOG_ERROR("You must enter a valid email address.\n");
            }
        }

        while(1 > enteredTicketCount || 20 < enteredTicketCount)
        {
            LOG_INFO("Character ticket count: ");

            std::string val;
            std::cin >> val;

            bool ok = false;

            enteredTicketCount = libcomp::String(val).ToInteger<uint8_t>(&ok);

            if(!ok || 1 > enteredTicketCount || 20 < enteredTicketCount)
            {
                LOG_ERROR("You must enter a value between 0 and 20.\n");
            }
        }

        while(1000000 < enteredCP)
        {
            LOG_INFO("CP (Cash Points): ");

            std::string val;
            std::cin >> val;

            bool ok = false;

            enteredCP = libcomp::String(val).ToInteger<uint32_t>(&ok);

            if(!ok || 1000000 < enteredCP)
            {
                LOG_ERROR("You must enter a value between 1 and 1,000,000.\n");
            }
        }

        while(0 > enteredUserLevel || 1000 < enteredUserLevel)
        {
            LOG_INFO("User level (0=normal user; 1,000=full GM): ");

            std::string val;
            std::cin >> val;

            bool ok = false;

            enteredUserLevel = libcomp::String(val).ToInteger<int32_t>(&ok);

            if(!ok || 0 > enteredUserLevel || 1000 < enteredUserLevel)
            {
                LOG_ERROR("You must enter a value between 0 and 1,000.\n");
            }
        }

        {
            LOG_INFO("Enable this account? [Y/n] ");

            std::string val;
            std::cin >> val;

            enabled = (val.size() < 1 || tolower(val.at(0)) != 'n');
        }

        // Save the input.
        displayName = enteredDisplayName;
        email = enteredEmail;
        ticketCount = enteredTicketCount;
        cp = enteredCP;
        userLevel = enteredUserLevel;
    }

    std::shared_ptr<objects::Account> account(new objects::Account);

    account->SetUsername(username);
    account->SetDisplayName(displayName);
    account->SetEmail(email);
    account->SetPassword(password);
    account->SetSalt(salt);
    account->SetCP(cp);
    account->SetTicketCount(ticketCount);
    account->SetUserLevel(userLevel);
    account->SetEnabled(enabled);
    account->Register(std::dynamic_pointer_cast<
        libcomp::PersistentObject>(account));

    if(!account->Insert(mDatabase))
    {
        LOG_ERROR("Failed to create account!\n");
    }
}

bool LobbyServer::Setup()
{
    std::string configPath = GetConfigPath() + "setup.xml";
    return InsertDataFromFile(configPath, mDatabase,
        std::set<std::string>{ "Account" });
}

AccountManager* LobbyServer::GetAccountManager()
{
    return mAccountManager;
}

LobbySyncManager* LobbyServer::GetLobbySyncManager() const
{
    return mSyncManager;
}

bool LobbyServer::ResetRegisteredWorlds()
{
    //Set all the default World information
    auto worldServers = libcomp::PersistentObject::LoadAll<objects::RegisteredWorld>(mDatabase);

    for(auto worldServer : worldServers)
    {
        if(worldServer->GetStatus() == objects::RegisteredWorld::Status_t::ACTIVE)
        {
            LOG_DEBUG(libcomp::String("Resetting registered world (%1) '%2' which did not exit"
                " cleanly during the previous execution.\n")
                .Arg(worldServer->GetID())
                .Arg(worldServer->GetName()));
            worldServer->SetStatus(objects::RegisteredWorld::Status_t::INACTIVE);
            if(!worldServer->Update(mDatabase))
            {
                LOG_CRITICAL("Registered world update failed.\n");
                return false;
            }
        }

        auto world = std::shared_ptr<World>(new World);
        world->RegisterWorld(worldServer);
        RegisterWorld(world);
    }

    return true;
}

libcomp::String LobbyServer::GetFakeAccountSalt(
    const libcomp::String& username)
{
    // Lock the muxtex.
    std::lock_guard<std::mutex> lock(mFakeSaltsLock);

    auto it = mFakeSalts.find(username);

    if(mFakeSalts.end() == it)
    {
        auto salt = libcomp::Decrypt::GenerateRandom(10);

        mFakeSalts[username] = salt;

        return salt;
    }
    else
    {
        return it->second;
    }
}

libcomp::String LobbyServer::ImportAccount(const libcomp::String& data,
    uint8_t worldID)
{
    tinyxml2::XMLDocument doc;

    if(tinyxml2::XML_SUCCESS != doc.Parse(data.C()))
    {
        return "Failed to parse account data.";
    }

    const tinyxml2::XMLElement *pImportObject = doc.RootElement(
        )->FirstChildElement("object");

    std::shared_ptr<libcomp::Database> lobbyDB, worldDB;

    {
        lobbyDB = GetMainDatabase();

        auto world = GetWorldByID(worldID);

        if(world)
        {
            worldDB = world->GetWorldDatabase();
        }
    }

    if(!lobbyDB || !worldDB)
    {
        return "Failed to connect to database.";
    }

    std::list<std::pair<libobjgen::UUID,
        std::shared_ptr<libcomp::PersistentObject>>> lobbyObjects;
    std::list<std::pair<libobjgen::UUID,
        std::shared_ptr<libcomp::PersistentObject>>> worldObjects;

    while(nullptr != pImportObject)
    {
        std::string objectType(pImportObject->Attribute("name"));

        auto typeExists = false;
        auto typeHash = libcomp::PersistentObject::GetTypeHashByName(
            objectType, typeExists);

        if(!typeExists)
        {
            return libcomp::String("Failed to parse unknown "
                "object '%1'.").Arg(objectType);
        }

        // Grab the UUID for the object.
        std::string uuidText;
        libobjgen::UUID uuid;

        const tinyxml2::XMLElement *pMember =
            pImportObject->FirstChildElement("member");

        while(nullptr != pMember)
        {
            if("uuid" == libcomp::String(pMember->Attribute(
                "name")).ToLower())
            {
                uuidText = pMember->GetText();
                uuid = libobjgen::UUID(uuidText);

                break;
            }

            pMember = pMember->NextSiblingElement("member");
        }

        // Make sure every object has a UUID.
        if(uuid.IsNull())
        {
            return libcomp::String("Bad UUID '%1' for object "
                "'%2'").Arg(uuidText).Arg(objectType);
        }

        auto obj = libcomp::PersistentObject::New(typeHash);

        if(!obj || !obj->Load(doc, *pImportObject))
        {
            return libcomp::String("Failed to load object '%1' with "
                "UUID %2.").Arg(objectType).Arg(uuid.ToString());
        }

        std::shared_ptr<libcomp::Database> db;

        if("Account" == objectType)
        {
            db = lobbyDB;

            lobbyObjects.push_back(std::make_pair(uuid, obj));
        }
        else
        {
            db = worldDB;

            worldObjects.push_back(std::make_pair(uuid, obj));
        }

        if(!db)
        {
            return "Failed to connect to database.";
        }

        auto existingObject = libcomp::PersistentObject::LoadObjectByUUID(
            typeHash, db, uuid);

        if(existingObject)
        {
            return libcomp::String("Object with UUID '%1' already exists "
                "in database.").Arg(uuid.ToString());
        }

        libcomp::String importError = CheckImportObject(objectType, obj,
            lobbyDB, worldDB);

        if(!importError.IsEmpty())
        {
            return importError;
        }

        pImportObject = pImportObject->NextSiblingElement("object");
    }

    for(auto pair : lobbyObjects)
    {
        if(!pair.second->Register(pair.second, pair.first))
        {
            return "Failed to register an object.";
        }
    }

    for(auto pair : worldObjects)
    {
        if(!pair.second->Register(pair.second, pair.first))
        {
            return "Failed to register an object.";
        }
    }

    auto lobbyChangeSet = libcomp::DatabaseChangeSet::Create();

    for(auto pair : lobbyObjects)
    {
        lobbyChangeSet->Insert(pair.second);
    }

    if(!lobbyDB->ProcessChangeSet(lobbyChangeSet))
    {
        LOG_ERROR(libcomp::String("Import failed with lobby database error: "
            "%1\n").Arg(lobbyDB->GetLastError()));

        return "Failed to write account into database.";
    }

    auto worldChangeSet = libcomp::DatabaseChangeSet::Create();

    for(auto pair : worldObjects)
    {
        worldChangeSet->Insert(pair.second);
    }

    if(!worldDB->ProcessChangeSet(worldChangeSet))
    {
        LOG_ERROR(libcomp::String("Import failed with world database error: "
            "%1\n").Arg(worldDB->GetLastError()));

        return "Failed to write account into database.";
    }

    return {};
}

libcomp::String LobbyServer::CheckImportObject(
    const libcomp::String& objectType,
    const std::shared_ptr<libcomp::PersistentObject>& obj,
    const std::shared_ptr<libcomp::Database>& lobbyDB,
    const std::shared_ptr<libcomp::Database>& worldDB)
{
    if("Account" == objectType)
    {
        auto account = std::dynamic_pointer_cast<objects::Account>(obj);
        auto conf = std::dynamic_pointer_cast<objects::LobbyConfig>(mConfig);

        if(objects::Account::LoadAccountByUsername(
                lobbyDB, account->GetUsername()) ||
            objects::Account::LoadAccountByEmail(
                lobbyDB, account->GetEmail()))
        {
            return libcomp::String("Account '%1' exists").Arg(
                account->GetUsername());
        }

        if(conf->GetImportStripCP())
        {
            account->SetCP(0);
        }

        if(conf->GetImportStripUserLevel())
        {
            account->SetUserLevel(0);
        }
    }

    if("Character" == objectType)
    {
        auto character = std::dynamic_pointer_cast<objects::Character>(obj);

        if(objects::Character::LoadCharacterByName(
            worldDB, character->GetName()))
        {
            return libcomp::String("Character '%1' exists").Arg(
                character->GetName());
        }
    }

    return {};
}
