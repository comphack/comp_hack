/**
 * @file libtester/src/ChannelClient.h
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to create a channel test connection.
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

#include "ChannelClient.h"

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libtester Includes
#include <LobbyClient.h>
#include <Login.h>
#include <TestConfig.h>

// libcomp Includes
#include <ChannelConnection.h>
#include <ScriptEngine.h>

// object Includes
#include <PacketLogin.h>

#include "ServerTest.h"

using namespace libtester;

ChannelClient::ChannelClient() : TestClient(), mEntityID(-1)
{
    SetConnection(std::make_shared<libcomp::ChannelConnection>(mService));
}

ChannelClient::ChannelClient(const ChannelClient& other)
{
    (void)other;

    assert(false);
}

ChannelClient::~ChannelClient()
{
}

bool ChannelClient::WaitForPacket(ChannelToClientPacketCode_t code,
    libcomp::ReadOnlyPacket& p, double& waitTime,
    asio::steady_timer::duration timeout)
{
    return TestClient::WaitForPacket(to_underlying(code),
        p, waitTime, timeout);
}

void ChannelClient::Login(const libcomp::String& username,
    const libcomp::String& password, const libcomp::String& characterName)
{
    double waitTime;

    int32_t sessionKey = -1;

    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->SetWaitForLogout(true);
        client->Login(username, password);

        if(!characterName.IsEmpty())
        {
            client->CreateCharacter(characterName);
        }

        client->StartGame();
        sessionKey = client->GetSessionKey();
    }

    ASSERT_TRUE(Connect(14666));
    ASSERT_TRUE(WaitEncrypted(waitTime));

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_LOGIN);
    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
        username, true);
    p.WriteS32Little(sessionKey);

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    UPHOLD_TRUE(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));
    UPHOLD_EQ(reply.ReadU32Little(), 1);

    p.Clear();
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_AUTH);
    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
        "0000000000000000000000000000000000000000", true);

    ClearMessages();
    GetConnection()->SendPacket(p);

    UPHOLD_TRUE(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_AUTH, reply, waitTime));
    UPHOLD_EQ(reply.ReadU32Little(), to_underlying(ErrorCodes_t::SUCCESS));
}

void ChannelClient::SendData()
{
    double waitTime;

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_SEND_DATA);

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    UPHOLD_TRUE(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE, reply, waitTime));
}

void ChannelClient::HandlePacket(ChannelToClientPacketCode_t cmd,
    libcomp::ReadOnlyPacket& p)
{
    switch(cmd)
    {
        case ChannelToClientPacketCode_t::PACKET_CHARACTER_DATA:
            HandleCharacterData(p);
            break;
        default:
            break;
    }
}

int32_t ChannelClient::GetEntityID() const
{
    return mEntityID;
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<ChannelClient>()
    {
        if(!BindingExists("ChannelClient"))
        {

            Sqrat::Class<ChannelClient> binding(mVM, "ChannelClient");
            binding.Func("Login", &ChannelClient::Login);

            Bind<ChannelClient>("ChannelClient", binding);
        }

        return *this;
    } // Using
} // namespace libcomp
