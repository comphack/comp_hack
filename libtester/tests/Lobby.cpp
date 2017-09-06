/**
 * @file libcomp/tests/Lobby.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test the lobby server.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
 *
 * Copyright (C) 2014-2017 COMP_hack Team <compomega@tutanota.com>
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

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libtester Includes
#include <LobbyClient.h>
#include <Login.h>
#include <ServerTest.h>
#include <TestConfig.h>

using namespace libcomp;

TEST(Lobby, WebAuth)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        libcomp::String sid1;
        libcomp::String sid2;

        EXPECT_FALSE(libtester::Login::WebLogin(LOGIN_USERNAME, "12345",
            LOGIN_CLIENT_VERSION, sid1, sid2))
            << "Was able to authenticate with website using bad credentials.";

        EXPECT_TRUE(libtester::Login::WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD,
            LOGIN_CLIENT_VERSION, sid1, sid2))
            << "Failed to authenticate with website.";

        EXPECT_FALSE(libtester::Login::WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD,
            "1.001", sid1, sid2))
            << "Was able to authenticate with a bad client version.";
    });
}

TEST(Lobby, BadClientVersion)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->Login(LOGIN_USERNAME, LOGIN_PASSWORD,
            ErrorCodes_t::WRONG_CLIENT_VERSION, ErrorCodes_t::SUCCESS, 1);
    });
}

TEST(Lobby, BadUsername)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->Login("h@k3r", LOGIN_PASSWORD,
            ErrorCodes_t::BAD_USERNAME_PASSWORD);
    });
}

TEST(Lobby, BadSID)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        libcomp::String sid1 =
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000"
            "000000000000000000000000000000";

        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD, sid1, true);
    });
}

TEST(Lobby, GoodSID)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD);
    });
}

TEST(Lobby, BadPassword)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->Login(LOGIN_USERNAME, "letMeInAnyway",
            ErrorCodes_t::SUCCESS, ErrorCodes_t::BAD_USERNAME_PASSWORD);
    });
}

TEST(Lobby, GoodPassword)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->Login(LOGIN_USERNAME, LOGIN_PASSWORD);
    });
}

TEST(Lobby, PacketsWithoutAuth)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        double waitTime;

        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        ASSERT_TRUE(client->Connect(10666));
        ASSERT_TRUE(client->WaitEncrypted(waitTime));

        libcomp::Packet p;
        p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_WORLD_LIST);

        libcomp::ReadOnlyPacket reply;

        client->ClearMessages();
        client->GetConnection()->SendPacket(p);

        ASSERT_FALSE(client->WaitForPacket(
            LobbyToClientPacketCode_t::PACKET_WORLD_LIST, reply, waitTime));
    });
}

TEST(Lobby, PacketsWithBadAuth)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        double waitTime;

        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->Login(LOGIN_USERNAME, "letMeInAnyway",
            ErrorCodes_t::SUCCESS, ErrorCodes_t::BAD_USERNAME_PASSWORD);

        libcomp::Packet p;
        p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_WORLD_LIST);

        libcomp::ReadOnlyPacket reply;

        client->ClearMessages();
        client->GetConnection()->SendPacket(p);

        ASSERT_FALSE(client->WaitForPacket(
            LobbyToClientPacketCode_t::PACKET_WORLD_LIST, reply, waitTime));
    });
}

TEST(Lobby, DeleteInvalidCharacter)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        double waitTime;

        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->Login(LOGIN_USERNAME, LOGIN_PASSWORD);

        libcomp::Packet p;
        p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_DELETE_CHARACTER);
        p.WriteU8(1);

        libcomp::ReadOnlyPacket reply;

        client->ClearMessages();
        client->GetConnection()->SendPacket(p);

        ASSERT_FALSE(client->WaitForPacket(
            LobbyToClientPacketCode_t::PACKET_DELETE_CHARACTER,
            reply, waitTime));
    });
}

TEST(Lobby, DoubleLogin)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);
        std::shared_ptr<libtester::LobbyClient> client2(
            new libtester::LobbyClient);
        std::shared_ptr<libtester::LobbyClient> client3(
            new libtester::LobbyClient);

        client->Login(LOGIN_USERNAME, LOGIN_PASSWORD);
        client2->Login(LOGIN_USERNAME, LOGIN_PASSWORD,
            ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN);
        client.reset();
        client3->Login(LOGIN_USERNAME, LOGIN_PASSWORD);
    });
}

TEST(Lobby, DoubleWebAuth)
{
    EXPECT_SERVER(libtester::ServerConfig::LobbyOnly(), []()
    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);
        std::shared_ptr<libtester::LobbyClient> client2(
            new libtester::LobbyClient);

        client->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD);
        client2->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD,
            libcomp::String(), true);
        client.reset();
        client2->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD);
    });
}

static void SignalHandler(int signum)
{
    extern pthread_t gSelf;

    if(SIGUSR2 == signum)
    {
        pthread_kill(gSelf, SIGUSR2);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGUSR2, SignalHandler);

    try
    {
        ::testing::InitGoogleTest(&argc, argv);

        return RUN_ALL_TESTS();
    }
    catch(...)
    {
        return EXIT_FAILURE;
    }
}
