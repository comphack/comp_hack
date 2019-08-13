/**
 * @file libtester/src/TestClient.h
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to create a test connection.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
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

#include "TestClient.h"

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <MessageConnectionClosed.h>
#include <MessageEncrypted.h>
#include <MessagePacket.h>
#include <MessageTimeout.h>
#include <ScriptEngine.h>

using namespace libtester;

constexpr asio::steady_timer::duration TestClient::DEFAULT_TIMEOUT;

TestClient::TestClient() : mTimer(mService), mMessageQueue(
    new libcomp::MessageQueue<libcomp::Message::Message*>())
{
}

TestClient::TestClient(const TestClient& other) : mTimer(mService),
    mMessageQueue(new libcomp::MessageQueue<libcomp::Message::Message*>())
{
    (void)other;

    assert(false);
}

TestClient::~TestClient()
{
    mService.stop();

    if(mServiceThread.joinable())
    {
        mServiceThread.join();
    }

    ClearMessages();
}

void TestClient::SetConnection(const std::shared_ptr<
    libcomp::EncryptedConnection>& conn)
{
    mConnection = conn;
}

bool TestClient::Connect(uint16_t port)
{
    mConnection->SetMessageQueue(mMessageQueue);

    bool result = mConnection->Connect("127.0.0.1", port);

    mServiceThread = std::thread([&]()
    {
        mService.run();
    });

    return result;
}

void TestClient::Disconnect()
{
    mConnection->Close();
}

std::shared_ptr<libcomp::EncryptedConnection> TestClient::GetConnection()
{
    return mConnection;
}

bool TestClient::HasDisconnectOrTimeout()
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

bool TestClient::WaitEncrypted(double& waitTime,
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

bool TestClient::WaitForDisconnect(double& waitTime,
    asio::steady_timer::duration timeout)
{
    return WaitForMessage([](const MessageList& msgs){
        for(auto msg : msgs)
        {
            if(nullptr != dynamic_cast<
                libcomp::Message::ConnectionClosed*>(msg))
            {
                return WaitStatus::Success;
            }
        }

        return WaitStatus::Wait;
    }, waitTime, timeout);
}

bool TestClient::WaitForPacket(uint16_t code,
    libcomp::ReadOnlyPacket& p, double& waitTime,
    asio::steady_timer::duration timeout)
{
    bool result = WaitForMessage([&](const MessageList& msgs){
        for(auto msg : msgs)
        {
            auto pmsg = dynamic_cast<libcomp::Message::Packet*>(msg);

            if(nullptr != pmsg && pmsg->GetCommandCode() == code)
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

            if(nullptr != pmsg)
            {
                auto cmdCode = pmsg->GetCommandCode();
                libcomp::ReadOnlyPacket copy(pmsg->GetPacket());
                libcomp::ReadOnlyPacket copy2(pmsg->GetPacket());

                HandlePacket(static_cast<ChannelToClientPacketCode_t>(
                    cmdCode), copy);

                if(cmdCode == code)
                {
                    foundPacket = true;
                    p = copy2;
                }
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
            LogGeneralWarning([&]()
            {
                return libcomp::String("Detected %1 other messages.\n")
                    .Arg(badMessages);
            });
        }

        ClearMessages();
    }

    return result;
}

bool TestClient::WaitForMessage(std::function<WaitStatus(
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
        LogGeneralDebug([&]()
        {
            return libcomp::String("Wait took %1 ms\n").Arg(waitTime);
        });
    }

    return result;
}

std::list<libcomp::Message::Message*> TestClient::TakeMessages()
{
    return std::move(mReceivedMessages);
}

void TestClient::ClearMessages()
{
    std::list<libcomp::Message::Message*> msgs = std::move(mReceivedMessages);

    for(auto msg : msgs)
    {
        delete msg;
    }
}

void TestClient::HandlePacket(ChannelToClientPacketCode_t cmd,
    libcomp::ReadOnlyPacket& p)
{
    (void)cmd;
    (void)p;
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<TestClient>()
    {
        if(!BindingExists("TestClient"))
        {

            Sqrat::Class<TestClient> binding(mVM, "TestClient");
            binding.Func("Disconnect", &TestClient::Disconnect);

            Bind<TestClient>("TestClient", binding);
        }

        return *this;
    } // Using
} // namespace libcomp
