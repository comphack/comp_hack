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

#include <thread>

using namespace libcomp;

TEST(Lobby, Connection)
{
    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    asio::io_service service;

    std::thread serviceThread([&service]()
    {
        service.run();
    });

    libcomp::LobbyConnection connection(service);
    connection.Connect("127.0.0.1", 10666);

    asio::steady_timer timer(service);
    timer.expires_from_now(std::chrono::seconds(30));
    timer.async_wait([&service](asio::error_code)
    {
        service.stop();
    });

    serviceThread.join();
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
