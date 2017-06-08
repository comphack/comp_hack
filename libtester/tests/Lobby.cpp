/**
 * @file libcomp/tests/DiffieHellman.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test the Diffie-Hellman key exchange.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2014-2016 COMP_hack Team <compomega@tutanota.com>
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
#include <HttpConnection.h>
#include <LobbyClient.h>
#include <Login.h>

// libcomp Includes
#include <Decrypt.h>
#include <ErrorCodes.h>
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <PacketLogin.h>

using namespace libcomp;

static const libcomp::String LOGIN_USERNAME = "testalpha";
static const libcomp::String LOGIN_PASSWORD = "same_as_my_luggage"; // 12345
static const libcomp::String LOGIN_CLIENT_VERSION = "1.666";

TEST(Lobby, WebAuth)
{
    libcomp::String sid1;
    libcomp::String sid2;

    EXPECT_FALSE(libtester::Login::WebLogin(LOGIN_USERNAME, "12345",
        LOGIN_CLIENT_VERSION, sid1, sid2))
        << "Was able to authenticate with the website using bad credentials.";

    EXPECT_TRUE(libtester::Login::WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD,
        LOGIN_CLIENT_VERSION, sid1, sid2))
        << "Failed to authenticate with the website.";

    EXPECT_FALSE(libtester::Login::WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD,
        "1.001", sid1, sid2))
        << "Was able to authenticate with a bad client version.";
}

TEST(Lobby, BadClientVersion)
{
    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->Login(LOGIN_USERNAME, LOGIN_PASSWORD,
        ErrorCodes_t::WRONG_CLIENT_VERSION, ErrorCodes_t::SUCCESS, 1);
}

TEST(Lobby, BadUsername)
{
    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->Login("h@k3r", LOGIN_PASSWORD, ErrorCodes_t::BAD_USERNAME_PASSWORD);
}

TEST(Lobby, BadSID)
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

    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD, sid1, true);
}

TEST(Lobby, GoodSID)
{
    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD);
}

TEST(Lobby, BadPassword)
{
    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->Login(LOGIN_USERNAME, "letMeInAnyway",
        ErrorCodes_t::SUCCESS, ErrorCodes_t::BAD_USERNAME_PASSWORD);
}

TEST(Lobby, GoodPassword)
{
    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->Login(LOGIN_USERNAME, LOGIN_PASSWORD);
}

TEST(Lobby, PacketsWithoutAuth)
{
    double waitTime;

    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    ASSERT_TRUE(client->Connect());
    ASSERT_TRUE(client->WaitEncrypted(waitTime));

    libcomp::Packet p;
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_WORLD_LIST);

    libcomp::ReadOnlyPacket reply;

    client->ClearMessages();
    client->GetConnection()->SendPacket(p);

    ASSERT_FALSE(client->WaitForPacket(
        LobbyToClientPacketCode_t::PACKET_WORLD_LIST, reply, waitTime));
}

TEST(Lobby, PacketsWithBadAuth)
{
    double waitTime;

    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->Login(LOGIN_USERNAME, "letMeInAnyway",
        ErrorCodes_t::SUCCESS, ErrorCodes_t::BAD_USERNAME_PASSWORD);

    libcomp::Packet p;
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_WORLD_LIST);

    libcomp::ReadOnlyPacket reply;

    client->ClearMessages();
    client->GetConnection()->SendPacket(p);

    ASSERT_FALSE(client->WaitForPacket(
        LobbyToClientPacketCode_t::PACKET_WORLD_LIST, reply, waitTime));
}

TEST(Lobby, DeleteInvalidCharacter)
{
    double waitTime;

    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    client->Login(LOGIN_USERNAME, LOGIN_PASSWORD);

    libcomp::Packet p;
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_DELETE_CHARACTER);
    p.WriteU8(1);

    libcomp::ReadOnlyPacket reply;

    client->ClearMessages();
    client->GetConnection()->SendPacket(p);

    ASSERT_FALSE(client->WaitForPacket(
        LobbyToClientPacketCode_t::PACKET_DELETE_CHARACTER, reply, waitTime));
}

TEST(Lobby, DoubleLogin)
{
    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);
    std::shared_ptr<libtester::LobbyClient> client2(new libtester::LobbyClient);
    std::shared_ptr<libtester::LobbyClient> client3(new libtester::LobbyClient);

    client->Login(LOGIN_USERNAME, LOGIN_PASSWORD);
    client2->Login(LOGIN_USERNAME, LOGIN_PASSWORD,
        ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN);
    client.reset();
    client3->Login(LOGIN_USERNAME, LOGIN_PASSWORD);
}

TEST(Lobby, DoubleWebAuth)
{
    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);
    std::shared_ptr<libtester::LobbyClient> client2(new libtester::LobbyClient);

    client->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD);
    client2->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD,
        libcomp::String(), true);
    client.reset();
    client2->WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD);
}

int main(int argc, char *argv[])
{
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
