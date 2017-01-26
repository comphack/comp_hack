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

#ifndef LIBTESTER_SRC_LOBBYCLIENT_H
#define LIBTESTER_SRC_LOBBYCLIENT_H

// libcomp Includes
#include <LobbyConnection.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <functional>
#include <thread>

namespace libtester
{

typedef std::list<libcomp::Message::Message*> MessageList;

class LobbyClient
{
public:
    static constexpr asio::steady_timer::duration DEFAULT_TIMEOUT =
        std::chrono::seconds(10);

    enum class WaitStatus
    {
        Success,
        Failure,
        Wait,
    };

    LobbyClient();
    ~LobbyClient();

    bool Connect();

    bool WaitEncrypted(double& waitTime, asio::steady_timer::duration timeout =
        DEFAULT_TIMEOUT);

    bool WaitForPacket(LobbyToClientPacketCode_t code,
        libcomp::ReadOnlyPacket& p, double& waitTime,
        asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);

    bool WaitForMessage(std::function<WaitStatus(
        const MessageList&)> eventFilter, double& waitTime,
        asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);
    MessageList TakeMessages();
    void ClearMessages();

    std::shared_ptr<libcomp::LobbyConnection> GetConnection();

private:
    bool HasDisconnectOrTimeout();

    asio::io_service mService;
    std::thread mServiceThread;
    asio::steady_timer mTimer;

    std::shared_ptr<libcomp::LobbyConnection> mConnection;
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;

    MessageList mReceivedMessages;
};

} // namespace libtester

#endif // LIBTESTER_SRC_LOBBYCLIENT_H
