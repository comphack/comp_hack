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

#include <LobbyConnection.h>
#include <Log.h>
#include <MessageEncrypted.h>

#include <thread>

using namespace libcomp;

TEST(Lobby, Connection)
{
    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    asio::io_service service;

    std::shared_ptr<libcomp::LobbyConnection> connection(
        new libcomp::LobbyConnection(service));

    std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>>
        messageQueue(new libcomp::MessageQueue<libcomp::Message::Message*>());

    connection->SetSelf(connection);
    connection->SetMessageQueue(messageQueue);
    connection->Connect("127.0.0.1", 10666);

    std::thread serviceThread([&service]()
    {
        service.run();
    });

    asio::steady_timer timer(service);
    timer.expires_from_now(std::chrono::seconds(10));
    timer.async_wait([&service](asio::error_code)
    {
        service.stop();
    });

    std::mutex resultLock;
    int goodEventCount = 0;
    int badEventCount = 0;

    new std::thread([&](std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> queue)
    {
        while(1)
        {
            std::list<libcomp::Message::Message*> msgs;
            queue->DequeueAll(msgs);

            for(auto msg : msgs)
            {
                libcomp::Message::Encrypted *pMessage = dynamic_cast<
                    libcomp::Message::Encrypted*>(msg);

                std::lock_guard<std::mutex> guard(resultLock);

                if(0 != pMessage)
                {
                    goodEventCount++;
                }
                else
                {
                    badEventCount++;
                    service.stop();
                }
                delete msg;
            }
        }
    }, messageQueue);

    serviceThread.join();

    std::lock_guard<std::mutex> guard(resultLock);

    EXPECT_EQ(badEventCount, 0);
    EXPECT_EQ(goodEventCount, 1);
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
