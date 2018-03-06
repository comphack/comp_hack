/**
 * @file libtester/src/LobbyClient.h
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to create a lobby test connection.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#include "LobbyClient.h"

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libtester Includes
#include <Login.h>

// libcomp Includes
#include <Decrypt.h>
#include <LobbyConnection.h>

// object Includes
#include <Character.h>
#include <PacketLogin.h>

#include "ServerTest.h"

using namespace libtester;

static const libcomp::String LOGIN_CLIENT_VERSION = "1.666";
static const uint32_t CLIENT_VERSION = 1666;

LobbyClient::LobbyClient() : TestClient(), mSessionKey(-1),
    mWaitForLogout(false)
{
    SetConnection(std::make_shared<libcomp::LobbyConnection>(mService));
}

LobbyClient::~LobbyClient()
{
}

bool LobbyClient::WaitForPacket(LobbyToClientPacketCode_t code,
    libcomp::ReadOnlyPacket& p, double& waitTime,
    asio::steady_timer::duration timeout)
{
    return TestClient::WaitForPacket(to_underlying(code),
        p, waitTime, timeout);
}

void LobbyClient::Login(const libcomp::String& username,
    const libcomp::String& password, ErrorCodes_t loginErrorCode,
    ErrorCodes_t authErrorCode, uint32_t clientVersion)
{
    double waitTime;

    if(0 == clientVersion)
    {
        clientVersion = CLIENT_VERSION;
    }

    ASSERT_TRUE(Connect(10666));
    ASSERT_TRUE(WaitEncrypted(waitTime));

    objects::PacketLogin obj;
    obj.SetClientVersion(clientVersion);
    obj.SetUsername(username);

    libcomp::Packet p;
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_LOGIN);

    ASSERT_TRUE(obj.SavePacket(p));

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE(WaitForPacket(
        LobbyToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));

    if(ErrorCodes_t::SUCCESS == loginErrorCode)
    {
        int tries = 1;

        while(mWaitForLogout && 100000 > tries && to_underlying(
            ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN) ==
            (int32_t)reply.PeekU32Little())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            ClearMessages();
            GetConnection()->SendPacket(p);

            ASSERT_TRUE(WaitForPacket(
                LobbyToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));

            tries++;
        }

        ASSERT_EQ(reply.Left(), sizeof(int32_t) + sizeof(uint32_t) +
            sizeof(uint16_t) + 5 * 2);
        ASSERT_EQ(reply.ReadS32Little(),
            to_underlying(ErrorCodes_t::SUCCESS));

        uint32_t challenge = reply.ReadU32Little();

        ASSERT_NE(challenge, 0);

        libcomp::String salt = reply.ReadString16Little(
            libcomp::Convert::ENCODING_UTF8);

        ASSERT_EQ(salt.Length(), 10);

        p.Clear();
        p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_AUTH);
        p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
            libcomp::Decrypt::HashPassword(libcomp::Decrypt::HashPassword(
                password, salt), libcomp::String("%1").Arg(challenge)), true);

        ClearMessages();
        GetConnection()->SendPacket(p);

        ASSERT_TRUE(WaitForPacket(
            LobbyToClientPacketCode_t::PACKET_AUTH, reply, waitTime));

        if(ErrorCodes_t::SUCCESS == authErrorCode)
        {
            ASSERT_EQ(reply.ReadS32Little(),
                to_underlying(ErrorCodes_t::SUCCESS));
            ASSERT_EQ(reply.ReadString16Little(
                libcomp::Convert::ENCODING_UTF8, true).Length(), 300);
        }
        else
        {
            ASSERT_EQ(reply.ReadS32Little(),
                to_underlying(authErrorCode));
        }

        ASSERT_EQ(reply.Left(), 0);
    }
    else
    {
        ASSERT_EQ(reply.Left(), sizeof(int32_t));
        ASSERT_EQ(reply.ReadS32Little(),
            to_underlying(loginErrorCode));
    }
}

void LobbyClient::WebLogin(const libcomp::String& username,
    const libcomp::String& password, const libcomp::String& sid,
    bool expectError)
{
    if(sid.IsEmpty() && !password.IsEmpty())
    {
        if(expectError)
        {
            ASSERT_FALSE(libtester::Login::WebLogin(username, password,
                LOGIN_CLIENT_VERSION, mSID1, mSID2))
                << "Authenticated with the website when an error was expected.";

            return;
        }
        else
        {
            ASSERT_TRUE(libtester::Login::WebLogin(username, password,
                LOGIN_CLIENT_VERSION, mSID1, mSID2))
                << "Failed to authenticate with the website.";
        }
    }
    else if(!sid.IsEmpty())
    {
        mSID1 = sid;
    }

    double waitTime;

    ASSERT_TRUE(Connect(10666));
    ASSERT_TRUE(WaitEncrypted(waitTime));

    objects::PacketLogin obj;
    obj.SetClientVersion(CLIENT_VERSION);
    obj.SetUsername(username);

    libcomp::Packet p;
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_LOGIN);

    ASSERT_TRUE(obj.SavePacket(p));

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE(WaitForPacket(
        LobbyToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));
    ASSERT_EQ(reply.Left(), sizeof(int32_t) + sizeof(uint32_t) +
            sizeof(uint16_t) + 5 * 2);
    ASSERT_EQ(reply.ReadS32Little(),
        to_underlying(ErrorCodes_t::SUCCESS));

    p.Clear();
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_AUTH);
    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8, mSID1, true);

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE(WaitForPacket(LobbyToClientPacketCode_t::PACKET_AUTH,
        reply, waitTime));

    if(!expectError)
    {
        ASSERT_EQ(reply.ReadS32Little(),
            to_underlying(ErrorCodes_t::SUCCESS));

        libcomp::String newSID = reply.ReadString16Little(
            libcomp::Convert::ENCODING_UTF8, true);

        ASSERT_EQ(newSID.Length(), 300);

        mSID1 = newSID;
    }
    else
    {
        ASSERT_EQ(reply.ReadS32Little(),
            to_underlying(ErrorCodes_t::BAD_USERNAME_PASSWORD));
    }

    ASSERT_EQ(reply.Left(), 0);
}

void LobbyClient::CreateCharacter(const libcomp::String& name)
{
    double waitTime;

    int8_t world = 0;

    objects::Character::Gender_t gender = objects::Character::Gender_t::MALE;

    uint32_t skinType  = 0x00000065;
    uint32_t faceType  = 0x00000001;
    uint32_t hairType  = 0x00000001;
    uint32_t hairColor = 0x00000008;
    uint32_t eyeColor  = 0x00000008;

    uint32_t equipTop    = 0x00000C3F;
    uint32_t equipBottom = 0x00000D64;
    uint32_t equipFeet   = 0x00000DB4;
    uint32_t equipComp   = 0x00001131;
    uint32_t equipWeapon = 0x000004B1;

    libcomp::Packet p;
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_CREATE_CHARACTER);
    p.WriteS8(world);
    p.WriteString16Little(libcomp::Convert::ENCODING_CP932, name, true);
    p.WriteS8(to_underlying(gender));
    p.WriteU32Little(skinType);
    p.WriteU32Little(faceType);
    p.WriteU32Little(hairType);
    p.WriteU32Little(hairColor);
    p.WriteU32Little(eyeColor);
    p.WriteU32Little(equipTop);
    p.WriteU32Little(equipBottom);
    p.WriteU32Little(equipFeet);
    p.WriteU32Little(equipComp);
    p.WriteU32Little(equipWeapon);

    ClearMessages();
    GetConnection()->SendPacket(p);

    libcomp::ReadOnlyPacket reply;

    ASSERT_TRUE(WaitForPacket(
        LobbyToClientPacketCode_t::PACKET_CREATE_CHARACTER,
        reply, waitTime));

    ASSERT_EQ(reply.Left(), 4);
    ASSERT_EQ(reply.ReadS32Little(),
        to_underlying(ErrorCodes_t::SUCCESS));
}

void LobbyClient::StartGame()
{
    double waitTime;

    uint8_t cid = 0;
    int8_t worldID = 0;

    libcomp::Packet p;
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_START_GAME);
    p.WriteU8(cid);
    p.WriteS8(worldID);

    ClearMessages();
    GetConnection()->SendPacket(p);

    libcomp::ReadOnlyPacket reply;

    UPHOLD_TRUE(WaitForPacket(
        LobbyToClientPacketCode_t::PACKET_START_GAME,
        reply, waitTime));

    int32_t sessionKey;
    uint8_t cid2;

    UPHOLD_GT(reply.Left(), sizeof(sessionKey) + sizeof(uint16_t) +
        sizeof(cid2));

    sessionKey = reply.ReadS32Little();

    libcomp::String server = reply.ReadString16Little(
        libcomp::Convert::ENCODING_UTF8);

    cid2 = reply.ReadU8();

    UPHOLD_EQ(cid, cid2);
    UPHOLD_FALSE(server.IsEmpty());
    UPHOLD_GT(sessionKey, -1);

    // Save the session key.
    mSessionKey = sessionKey;
}

int32_t LobbyClient::GetSessionKey() const
{
    return mSessionKey;
}

void LobbyClient::SetWaitForLogout(bool wait)
{
    mWaitForLogout = wait;
}
