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
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <PacketLogin.h>

using namespace libcomp;

static const libcomp::String LOGIN_USERNAME = "testalpha";
static const libcomp::String LOGIN_PASSWORD = "same_as_my_luggage"; // 12345
static const libcomp::String LOGIN_CLIENT_VERSION = "1.666";
static const uint32_t CLIENT_VERSION = 1666;

TEST(Lobby, Connection)
{
    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    libcomp::String sid1;
    libcomp::String sid2;

    EXPECT_FALSE(libtester::Login::WebLogin(LOGIN_USERNAME, "12345",
        LOGIN_CLIENT_VERSION, sid1, sid2))
        << "Was able to authenticate with the website using bad credentials.";

    ASSERT_TRUE(libtester::Login::WebLogin(LOGIN_USERNAME, LOGIN_PASSWORD,
        LOGIN_CLIENT_VERSION, sid1, sid2))
        << "Failed to authenticate with the website.";

    double waitTime;

    std::shared_ptr<libtester::LobbyClient> client(new libtester::LobbyClient);

    ASSERT_TRUE(client->Connect());
    ASSERT_TRUE(client->WaitEncrypted(waitTime));

    objects::PacketLogin obj;
    obj.SetClientVersion(CLIENT_VERSION);
    obj.SetUsername(LOGIN_USERNAME);

    libcomp::Packet p;
    p.WritePacketCode(LobbyClientPacketCode_t::PACKET_LOGIN);

    ASSERT_TRUE(obj.SavePacket(p));

    libcomp::ReadOnlyPacket reply;

    client->ClearMessages();
    client->GetConnection()->SendPacket(p);

    ASSERT_TRUE(client->WaitForPacket(
        LobbyClientPacketCode_t::PACKET_LOGIN_RESPONSE, reply, waitTime));
    ASSERT_EQ(reply.ReadU32Little(), 0);
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
