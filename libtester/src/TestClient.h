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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBTESTER_SRC_TESTCLIENT_H
#define LIBTESTER_SRC_TESTCLIENT_H

// libcomp Includes
#include <ErrorCodes.h>
#include <EncryptedConnection.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <functional>
#include <thread>

namespace libtester
{

typedef std::list<libcomp::Message::Message*> MessageList;

class TestClient
{
public:
    static constexpr asio::steady_timer::duration DEFAULT_TIMEOUT =
        std::chrono::seconds(60);

    enum class WaitStatus
    {
        Success,
        Failure,
        Wait,
    };

    TestClient();
    TestClient(const TestClient& other);
    virtual ~TestClient();

    bool Connect(uint16_t port);

    void Disconnect();

    bool WaitEncrypted(double& waitTime, asio::steady_timer::duration
        timeout = DEFAULT_TIMEOUT);

    bool WaitForDisconnect(double& waitTime,
        asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);

    bool WaitForPacket(uint16_t code,
        libcomp::ReadOnlyPacket& p, double& waitTime,
        asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);

    bool WaitForMessage(std::function<WaitStatus(
        const MessageList&)> eventFilter, double& waitTime,
        asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);
    MessageList TakeMessages();
    void ClearMessages();

    std::shared_ptr<libcomp::EncryptedConnection> GetConnection();

protected:
    void SetConnection(const std::shared_ptr<
        libcomp::EncryptedConnection>& conn);
    bool HasDisconnectOrTimeout();

    virtual void HandlePacket(ChannelToClientPacketCode_t cmd,
        libcomp::ReadOnlyPacket& p);

    asio::io_service mService;
    std::thread mServiceThread;
    asio::steady_timer mTimer;

    std::shared_ptr<libcomp::EncryptedConnection> mConnection;
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;

    MessageList mReceivedMessages;
};

} // namespace libtester

#endif // LIBTESTER_SRC_TESTCLIENT_H
