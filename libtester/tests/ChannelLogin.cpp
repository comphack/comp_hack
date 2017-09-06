/**
 * @file libcomp/tests/ChannelLogin.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test the channel server login.
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
#include <ChannelClient.h>
#include <ServerTest.h>
#include <TestConfig.h>

using namespace libcomp;

TEST(Lobby, GoodPassword)
{
    EXPECT_SERVER(libtester::ServerConfig::SingleChannel(), []()
    {
        std::shared_ptr<libtester::ChannelClient> client(
            new libtester::ChannelClient);

        client->Login(LOGIN_USERNAME, LOGIN_PASSWORD, "CrashDummy");
        client->SendData();

        std::shared_ptr<libtester::ChannelClient> client2(
            new libtester::ChannelClient);

        client2->Login(LOGIN_USERNAME2, LOGIN_PASSWORD2, "TheInstigator");
        client2->SendData();
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
