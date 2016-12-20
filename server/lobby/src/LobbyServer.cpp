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

#include "LobbyServer.h"

// lobby Includes
#include "LobbyConnection.h"
#include "Packets.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>

// Object Includes
#include "Account.h"
#include "LobbyConfig.h"

// Standard C++11 Includes
#include <iostream>

using namespace lobby;

LobbyServer::LobbyServer(std::shared_ptr<objects::ServerConfig> config,
    const libcomp::String& configPath) : libcomp::BaseServer(config, configPath)
{
    if(true||!mDatabase->TableHasRows("Account"))
    {
        CreateFirstAccount();
    }

    auto conf = std::dynamic_pointer_cast<objects::LobbyConfig>(mConfig);

    mManagerConnection = std::shared_ptr<ManagerConnection>(new ManagerConnection(mSelf, std::shared_ptr<asio::io_service>(&mService), mMainWorker.GetMessageQueue()));

    auto connectionManager = std::shared_ptr<libcomp::Manager>(mManagerConnection);

    auto internalPacketManager = std::shared_ptr<libcomp::ManagerPacket>(new libcomp::ManagerPacket(mSelf));
    internalPacketManager->AddParser<Parsers::SetWorldDescription>(PACKET_SET_WORLD_DESCRIPTION);
    internalPacketManager->AddParser<Parsers::SetChannelDescription>(PACKET_SET_CHANNEL_DESCRIPTION);

    //Add the managers to the main worker.
    mMainWorker.AddManager(internalPacketManager);
    mMainWorker.AddManager(connectionManager);

    auto clientPacketManager = std::shared_ptr<libcomp::ManagerPacket>(new libcomp::ManagerPacket(mSelf));
    clientPacketManager->AddParser<Parsers::Login>(PACKET_LOGIN);
    clientPacketManager->AddParser<Parsers::Auth>(PACKET_AUTH);
    clientPacketManager->AddParser<Parsers::StartGame>(PACKET_START_GAME);
    clientPacketManager->AddParser<Parsers::CharacterList>(PACKET_CHARACTER_LIST);
    clientPacketManager->AddParser<Parsers::WorldList>(PACKET_WORLD_LIST);
    clientPacketManager->AddParser<Parsers::CreateCharacter>(PACKET_CREATE_CHARACTER);
    clientPacketManager->AddParser<Parsers::DeleteCharacter>(PACKET_DELETE_CHARACTER);
    clientPacketManager->AddParser<Parsers::QueryPurchaseTicket>(PACKET_QUERY_PURCHASE_TICKET);
    clientPacketManager->AddParser<Parsers::PurchaseTicket>(PACKET_PURCHASE_TICKET);

    // Add the managers to the generic workers.
    mWorker.AddManager(clientPacketManager);
    mWorker.AddManager(connectionManager);

    // Start the worker.
    mWorker.Start();
}

LobbyServer::~LobbyServer()
{
    // Make sure the worker threads stop.
    mWorker.Join();
}

void LobbyServer::Shutdown()
{
    BaseServer::Shutdown();

    /// @todo Add more workers.
    mWorker.Shutdown();
}

std::list<std::shared_ptr<lobby::World>> LobbyServer::GetWorlds()
{
    return mManagerConnection->GetWorlds();
}

std::shared_ptr<lobby::World> LobbyServer::GetWorldByConnection(std::shared_ptr<libcomp::InternalConnection> connection)
{
    return mManagerConnection->GetWorldByConnection(connection);
}

std::shared_ptr<libcomp::TcpConnection> LobbyServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::shared_ptr<libcomp::TcpConnection>(
        new libcomp::LobbyConnection(socket, CopyDiffieHellman(
            GetDiffieHellman())
        )
    );

    // Assign this to the only worker available.
    std::dynamic_pointer_cast<libcomp::LobbyConnection>(
        connection)->SetMessageQueue(mWorker.GetMessageQueue());

    // Make sure this is called after connecting.
    connection->SetSelf(connection);
    connection->ConnectionSuccess();

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
    uint32_t cp = 1000000;
    uint8_t ticketCount = 1;
    int32_t userLevel = 1000;
    uint8_t enabled = 1; /// @todo Make this a boolean.

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

        while(0 > enteredTicketCount || 20 < enteredTicketCount)
        {
            LOG_INFO("Character ticket count: ");

            std::string val;
            std::cin >> val;

            bool ok = false;

            enteredTicketCount = libcomp::String(val).ToInteger<uint8_t>(&ok);

            if(!ok || 0 > enteredTicketCount || 20 < enteredTicketCount)
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

    account->SetUserName(username);
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

    if(!account->Insert())
    {
        LOG_ERROR("Failed to create account!\n");
    }
}
