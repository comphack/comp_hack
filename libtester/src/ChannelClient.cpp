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

ChannelClient::ChannelClient() : TestClient(), mEntityID(-1),
    mPartnerEntityID(-1), mZoneID(-1), mActivationID(-1), mAccountDumpParts(0),
    mLastAccountDumpPart(0)
{
    mCharacter = std::make_shared<objects::Character>();
    mCharacter->SetCoreStats(std::make_shared<objects::EntityStats>());

    for(int i = 0; i < 10; ++i)
    {
        mDemonIDs[i] = -1;
    }

    SetConnection(std::make_shared<libcomp::ChannelConnection>(mService));
}

ChannelClient::ChannelClient(const ChannelClient& other) : TestClient(other)
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

bool ChannelClient::LoginWithKey(const libcomp::String& username,
    int32_t sessionKey)
{
    double waitTime;

    GetConnection()->SetName(libcomp::String("channel_%1").Arg(username));

    ASSERT_TRUE_OR_RETURN(Connect(14666));
    ASSERT_TRUE_OR_RETURN(WaitEncrypted(waitTime));

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_LOGIN);
    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
        username, true);
    p.WriteS32Little(sessionKey);

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));
    ASSERT_EQ_OR_RETURN(reply.ReadU32Little(), 1);

    p.Clear();
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_AUTH);
    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
        "0000000000000000000000000000000000000000", true);

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_AUTH, reply, waitTime));
    ASSERT_EQ_OR_RETURN(reply.ReadU32Little(),
        to_underlying(ErrorCodes_t::SUCCESS));

    return true;
}

bool ChannelClient::Login(const libcomp::String& username,
    const libcomp::String& password, const libcomp::String& characterName)
{
    int32_t sessionKey = -1;

    {
        std::shared_ptr<libtester::LobbyClient> client(
            new libtester::LobbyClient);

        client->SetWaitForLogout(true);
        ASSERT_TRUE_OR_RETURN(client->Login(username, password));

        if(!characterName.IsEmpty())
        {
            ASSERT_TRUE_OR_RETURN(client->CreateCharacter(characterName));
        }

        ASSERT_TRUE_OR_RETURN(client->StartGame());
        sessionKey = client->GetSessionKey();
    }

    return LoginWithKey(username, sessionKey);
}

bool ChannelClient::SendData()
{
    double waitTime;

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_SEND_DATA);

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE, reply, waitTime));

    return true;
}

bool ChannelClient::SendState()
{
    double waitTime;

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_STATE);

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_CHARACTER_DATA, reply, waitTime));

    return true;
}

bool ChannelClient::SendPopulateZone()
{
    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_POPULATE_ZONE);
    p.WriteS32Little(mEntityID);

    ClearMessages();
    GetConnection()->SendPacket(p);

    return true;
}

bool ChannelClient::AmalaRequestAccountDump()
{
    double waitTime;

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_AMALA_REQ_ACCOUNT_DUMP);
    p.HexDump();

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_HEADER,
        reply, waitTime));

    while(mLastAccountDumpPart < mAccountDumpParts)
    {
        ASSERT_TRUE_OR_RETURN(WaitForPacket(
            ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_PART,
            reply, waitTime));
    }

    return true;
}

bool ChannelClient::Say(const libcomp::String& msg)
{
    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_CHAT);
    p.WriteU16Little(to_underlying(ChatType_t::CHAT_SAY));
    p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
        msg, true);

    ClearMessages();
    GetConnection()->SendPacket(p);

    return true;
}

bool ChannelClient::ActivateSkill(int32_t entityID, uint32_t skillID,
    uint32_t targetType, int64_t demonID)
{
    // printf("ActivateSkill(%d, %u, %u, %lld)\n", entityID, skillID,
    //     targetType, demonID);

    double waitTime;

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_SKILL_ACTIVATE);
    p.WriteS32Little(entityID);
    p.WriteU32Little(skillID);
    p.WriteU32Little(targetType);
    p.WriteS64Little(demonID);

    libcomp::ReadOnlyPacket reply;

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_SKILL_ACTIVATED,
        reply, waitTime));

    /// @todo Handle this generic.
    (void)reply.ReadS32Little(); // entity id
    (void)reply.ReadU32Little(); // skill id
    mActivationID = reply.ReadS8();
    printf("got activation: %d\n", (int)mActivationID);

    return true;
}

bool ChannelClient::ExecuteSkill(int32_t entityID, int8_t activationID,
    int64_t demonID)
{
    // printf("ExecuteSkill(%d, %d, %lld)\n", entityID,
    //     (int)mActivationID, demonID);

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_SKILL_EXECUTE);
    p.WriteS32Little(entityID);
    p.WriteS8(activationID);
    p.WriteS64Little(demonID);

    ClearMessages();
    GetConnection()->SendPacket(p);

    return true;
}

bool ChannelClient::ContractDemon(uint32_t demonID)
{
    double waitTime;

    ASSERT_TRUE_OR_RETURN(Say(libcomp::String("@contract %1").Arg(demonID)));

    libcomp::ReadOnlyPacket reply;

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_DEMON_BOX_DATA,
        reply, waitTime));

    return true;
}

bool ChannelClient::SummonDemon(int64_t demonID)
{
    double waitTime;

    const uint32_t SKILL_SUMMON_DEMON = 5704;

    ASSERT_TRUE_OR_RETURN(ActivateSkill(mEntityID,
        SKILL_SUMMON_DEMON, ACTIVATION_DEMON, demonID));

    /// This skill auto-executes
    /*ASSERT_TRUE_OR_RETURN(ExecuteSkill(mEntityID,
        GetActivationID(), demonID));*/

    libcomp::ReadOnlyPacket reply;

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_PARTNER_SUMMONED,
        reply, waitTime));
    /// @todo parse this packet?

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_DEMON_DATA);

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(WaitForPacket(
        ChannelToClientPacketCode_t::PACKET_PARTNER_DATA,
        reply, waitTime));

    mPartnerEntityID = reply.ReadS32Little();

    p.Clear();
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_FIX_OBJECT_POSITION);
    p.WriteS32Little(mPartnerEntityID);
    p.WriteFloat(0.0f); // X
    p.WriteFloat(0.0f); // Y
    p.WriteFloat(0.0f); // client time

    ClearMessages();
    GetConnection()->SendPacket(p);

    return true;
}

bool ChannelClient::EventResponse(int32_t option)
{
    printf("EventResponse(%d)\n", option);

    libcomp::Packet p;
    p.WritePacketCode(ClientToChannelPacketCode_t::PACKET_EVENT_RESPONSE);
    p.WriteS32Little(option);

    ClearMessages();
    GetConnection()->SendPacket(p);

    return true;
}

void ChannelClient::HandlePacket(ChannelToClientPacketCode_t cmd,
    libcomp::ReadOnlyPacket& p)
{
    switch(cmd)
    {
        case ChannelToClientPacketCode_t::PACKET_ZONE_CHANGE:
            HandleZoneChange(p);
            break;
        case ChannelToClientPacketCode_t::PACKET_CHARACTER_DATA:
            HandleCharacterData(p);
            break;
        case ChannelToClientPacketCode_t::PACKET_DEMON_BOX_DATA:
            HandleDemonBoxData(p);
            break;

        case ChannelToClientPacketCode_t::PACKET_AMALA_SERVER_VERSION:
            HandleAmalaServerVersion(p);
            break;
        case ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_HEADER:
            HandleAmalaAccountDumpHeader(p);
            break;
        case ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_PART:
            HandleAmalaAccountDumpPart(p);
            break;

        default:
            break;
    }
}

int32_t ChannelClient::GetEntityID() const
{
    return mEntityID;
}

int8_t ChannelClient::GetActivationID() const
{
    return mActivationID;
}

int64_t ChannelClient::GetDemonID(int8_t slot) const
{
    if(0 > slot || 10 <= slot)
    {
        return -1;
    }

    return mDemonIDs[slot];
}

namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<ChannelClient>()
    {
        if(!BindingExists("ChannelClient"))
        {
            // Include the base class
            Using<TestClient>();

            Sqrat::DerivedClass<ChannelClient, TestClient> binding(
                mVM, "ChannelClient");
            binding.Func("Login", &ChannelClient::Login);
            binding.Func("LoginWithKey", &ChannelClient::LoginWithKey);
            binding.Func("SendData", &ChannelClient::SendData);
            binding.Func("SendState", &ChannelClient::SendState);
            binding.Func("SendPopulateZone",
                &ChannelClient::SendPopulateZone);
            binding.Func("AmalaRequestAccountDump",
                &ChannelClient::AmalaRequestAccountDump);
            binding.Func("GetEntityID", &ChannelClient::GetEntityID);
            binding.Func("GetActivationID", &ChannelClient::GetActivationID);
            binding.Func("GetDemonID", &ChannelClient::GetDemonID);
            binding.Func("ContractDemon", &ChannelClient::ContractDemon);
            binding.Func("SummonDemon", &ChannelClient::SummonDemon);
            binding.Func("Say", &ChannelClient::Say);
            binding.Func("ActivateSkill", &ChannelClient::ActivateSkill);
            binding.Func("ExecuteSkill", &ChannelClient::ExecuteSkill);
            binding.Func("EventResponse", &ChannelClient::EventResponse);

            Bind<ChannelClient>("ChannelClient", binding);
        }

        return *this;
    } // Using
} // namespace libcomp
