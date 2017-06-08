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

#include "LobbyClient.h"

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libtester Includes
#include <Login.h>

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <MessageConnectionClosed.h>
#include <MessageEncrypted.h>
#include <MessagePacket.h>
#include <MessageTimeout.h>

// object Includes
#include <PacketLogin.h>

using namespace libtester;

constexpr asio::steady_timer::duration LobbyClient::DEFAULT_TIMEOUT;

static const libcomp::String LOGIN_CLIENT_VERSION = "1.666";
static const uint32_t CLIENT_VERSION = 1666;

LobbyClient::LobbyClient() : mTimer(mService),
    mConnection(new libcomp::LobbyConnection(mService)),
    mMessageQueue(new libcomp::MessageQueue<libcomp::Message::Message*>())
{
}

LobbyClient::~LobbyClient()
{
    mService.stop();

    if(mServiceThread.joinable())
    {
        mServiceThread.join();
    }

    ClearMessages();
}

bool LobbyClient::Connect()
{
    mConnection->SetMessageQueue(mMessageQueue);

    bool result = mConnection->Connect("127.0.0.1", 10666);

    mServiceThread = std::thread([&]()
    {
        mService.run();
    });

    return result;
}

std::shared_ptr<libcomp::LobbyConnection> LobbyClient::GetConnection()
{
    return mConnection;
}

bool LobbyClient::HasDisconnectOrTimeout()
{
    for(auto msg : mReceivedMessages)
    {
        if(nullptr != dynamic_cast<libcomp::Message::ConnectionClosed*>(msg) ||
            nullptr != dynamic_cast<libcomp::Message::Timeout*>(msg))
        {
            return true;
        }
    }

    return false;
}

bool LobbyClient::WaitEncrypted(double& waitTime,
    asio::steady_timer::duration timeout)
{
    return WaitForMessage([](const MessageList& msgs){
        for(auto msg : msgs)
        {
            if(nullptr != dynamic_cast<libcomp::Message::Encrypted*>(msg))
            {
                return WaitStatus::Success;
            }
        }

        return WaitStatus::Wait;
    }, waitTime, timeout);
}

bool LobbyClient::WaitForDisconnect(double& waitTime,
    asio::steady_timer::duration timeout)
{
    return WaitForMessage([](const MessageList& msgs){
        for(auto msg : msgs)
        {
            printf("msg\n");
            if(nullptr != dynamic_cast<libcomp::Message::ConnectionClosed*>(msg))
            {
                return WaitStatus::Success;
            }
        }

        return WaitStatus::Wait;
    }, waitTime, timeout);
}

bool LobbyClient::WaitForPacket(LobbyToClientPacketCode_t code,
    libcomp::ReadOnlyPacket& p, double& waitTime,
    asio::steady_timer::duration timeout)
{
    bool result = WaitForMessage([&](const MessageList& msgs){
        for(auto msg : msgs)
        {
            auto pmsg = dynamic_cast<libcomp::Message::Packet*>(msg);

            if(nullptr != pmsg && pmsg->GetCommandCode() == to_underlying(code))
            {
                return WaitStatus::Success;
            }
        }

        return WaitStatus::Wait;
    }, waitTime, timeout);

    if(result)
    {
        bool foundPacket = false;
        int badMessages = 0;

        for(auto msg : mReceivedMessages)
        {
            auto pmsg = dynamic_cast<libcomp::Message::Packet*>(msg);

            if(nullptr != pmsg && pmsg->GetCommandCode() == to_underlying(code))
            {
                foundPacket = true;
                libcomp::ReadOnlyPacket copy(pmsg->GetPacket());
                p = copy;
            }
        }

        badMessages = static_cast<int>(mReceivedMessages.size()) -
            (foundPacket ? 1 : 0);

        if(!foundPacket)
        {
            result = false;
        }

        if(0 < badMessages)
        {
            LOG_WARNING(libcomp::String("Detected %1 other messages.\n").Arg(
                badMessages));
        }

        ClearMessages();
    }

    return result;
}

bool LobbyClient::WaitForMessage(std::function<WaitStatus(
    const MessageList&)> eventFilter, double& waitTime,
    asio::steady_timer::duration timeout)
{
    bool result = true;
    bool keepLooking = true;

    mTimer.expires_from_now(timeout);
    mTimer.async_wait([&](asio::error_code ec)
    {
        if(asio::error::operation_aborted != ec)
        {
            mMessageQueue->Enqueue(new libcomp::Message::Timeout);
        }
    });

    auto start = std::chrono::steady_clock::now();

    do
    {
        // Check if there is a failure condition.
        if(HasDisconnectOrTimeout())
        {
            result = false;
            keepLooking = false;
        }
        else
        {
            // Check if the desired event exists.
            WaitStatus status = eventFilter(mReceivedMessages);

            if(WaitStatus::Wait != status)
            {
                keepLooking = false;

                result = (WaitStatus::Success == status);
            }
        }

        // Get more messages.
        if(keepLooking)
        {
            std::list<libcomp::Message::Message*> msgs;
            mMessageQueue->DequeueAll(msgs);
            mReceivedMessages.splice(mReceivedMessages.end(), msgs);
        }
    } while(keepLooking);

    auto stop = std::chrono::steady_clock::now();

    mTimer.cancel();

    std::chrono::duration<double, std::milli> duration = stop - start;
    waitTime = duration.count();

    if(result)
    {
        LOG_DEBUG(libcomp::String("Wait took %1 ms\n").Arg(waitTime));
    }

    return result;
}

std::list<libcomp::Message::Message*> LobbyClient::TakeMessages()
{
    return std::move(mReceivedMessages);
}

void LobbyClient::ClearMessages()
{
    std::list<libcomp::Message::Message*> msgs = std::move(mReceivedMessages);

    for(auto msg : msgs)
    {
        delete msg;
    }
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

    ASSERT_TRUE(Connect());
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
            libcomp::Decrypt::HashPassword(password, salt), true);

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

    ASSERT_TRUE(Connect());
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
