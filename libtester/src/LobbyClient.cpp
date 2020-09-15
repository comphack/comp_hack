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

#include "LobbyClient.h"

#include <PopIgnore.h>
#include <PushIgnore.h>
#include <gtest/gtest.h>

// libtester Includes
#include <Login.h>

// libcomp Includes
#include <Crypto.h>
#include <LobbyConnection.h>
#include <Log.h>
#include <ScriptEngine.h>

// object Includes
#include <Character.h>
#include <PacketLogin.h>

#include "ServerTest.h"

using namespace libtester;

static const libcomp::String LOGIN_CLIENT_VERSION = "1.666";
static const uint32_t CLIENT_VERSION = 1666;

LobbyClient::LobbyClient()
    : TestClient(),
      mSessionKey(-1),
      mWaitForLogout(false),
      mLoginTime(0),
      mTicketCount(0),
      mTicketCost(0),
      mCP(0) {
  SetConnection(std::make_shared<libcomp::LobbyConnection>(mService));
}

LobbyClient::LobbyClient(const LobbyClient& other) : TestClient(other) {
  (void)other;

  assert(false);
}

LobbyClient::~LobbyClient() {}

bool LobbyClient::WaitForPacket(LobbyToClientPacketCode_t code,
                                libcomp::ReadOnlyPacket& p, double& waitTime,
                                asio::steady_timer::duration timeout) {
  return TestClient::WaitForPacket(to_underlying(code), p, waitTime, timeout);
}

bool LobbyClient::Login(const libcomp::String& username,
                        const libcomp::String& password,
                        ErrorCodes_t loginErrorCode, ErrorCodes_t authErrorCode,
                        uint32_t clientVersion) {
  double waitTime;

  if (0 == clientVersion) {
    clientVersion = CLIENT_VERSION;
  }

  GetConnection()->SetName(libcomp::String("lobby_%1").Arg(username));

  ASSERT_TRUE_OR_RETURN(Connect(10666));
  ASSERT_TRUE_OR_RETURN(WaitEncrypted(waitTime));

  objects::PacketLogin obj;
  obj.SetClientVersion(clientVersion);
  obj.SetUsername(username);

  libcomp::Packet p;
  p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_LOGIN);

  ASSERT_TRUE_OR_RETURN(obj.SavePacket(p));

  libcomp::ReadOnlyPacket reply;

  ClearMessages();
  GetConnection()->SendPacket(p);

  ASSERT_TRUE_OR_RETURN(
      WaitForPacket(LobbyToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));

  if (ErrorCodes_t::SUCCESS == loginErrorCode) {
    int tries = 1;

    while (mWaitForLogout && 100000 > tries &&
           to_underlying(ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN) ==
               (int32_t)reply.PeekU32Little()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      ClearMessages();
      GetConnection()->SendPacket(p);

      ASSERT_TRUE_OR_RETURN(WaitForPacket(
          LobbyToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));

      tries++;
    }

    ASSERT_EQ_OR_RETURN(reply.Left(), sizeof(int32_t) + sizeof(uint32_t) +
                                          sizeof(uint16_t) + 5 * 2);
    ASSERT_EQ_OR_RETURN(reply.ReadS32Little(),
                        to_underlying(ErrorCodes_t::SUCCESS));

    uint32_t challenge = reply.ReadU32Little();

    ASSERT_NE_OR_RETURN(challenge, 0);

    libcomp::String salt =
        reply.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

    ASSERT_EQ_OR_RETURN(salt.Length(), 10);

    p.Clear();
    p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_AUTH);
    p.WriteString16Little(libcomp::Convert::ENCODING_UTF8,
                          libcomp::Crypto::HashPassword(
                              libcomp::Crypto::HashPassword(password, salt),
                              libcomp::String("%1").Arg(challenge)),
                          true);

    ClearMessages();
    GetConnection()->SendPacket(p);

    ASSERT_TRUE_OR_RETURN(
        WaitForPacket(LobbyToClientPacketCode_t::PACKET_AUTH, reply, waitTime));

    if (ErrorCodes_t::SUCCESS == authErrorCode) {
      ASSERT_EQ_OR_RETURN(reply.ReadS32Little(),
                          to_underlying(ErrorCodes_t::SUCCESS));
      ASSERT_EQ_OR_RETURN(
          reply.ReadString16Little(libcomp::Convert::ENCODING_UTF8, true)
              .Length(),
          300);
    } else {
      ASSERT_EQ_OR_RETURN(reply.ReadS32Little(), to_underlying(authErrorCode));
    }

    ASSERT_EQ_OR_RETURN(reply.Left(), 0);
  } else {
    ASSERT_EQ_OR_RETURN(reply.Left(), sizeof(int32_t));
    ASSERT_EQ_OR_RETURN(reply.ReadS32Little(), to_underlying(loginErrorCode));
  }

  return true;
}

bool LobbyClient::ClassicLogin(const libcomp::String& username,
                               const libcomp::String& password) {
  return Login(username, password);
}

bool LobbyClient::WebLogin(const libcomp::String& username,
                           const libcomp::String& password,
                           const libcomp::String& sid, bool expectError) {
  if (sid.IsEmpty() && !password.IsEmpty()) {
    if (expectError) {
      ASSERT_FALSE_OR_RETURN_MSG(
          libtester::Login::WebLogin(username, password, LOGIN_CLIENT_VERSION,
                                     mSID1, mSID2),
          "Authenticated with the website when an error was expected.");

      return true;
    } else {
      ASSERT_TRUE_OR_RETURN_MSG(
          libtester::Login::WebLogin(username, password, LOGIN_CLIENT_VERSION,
                                     mSID1, mSID2),
          "Failed to authenticate with the website.");
    }
  } else if (!sid.IsEmpty()) {
    mSID1 = sid;
  }

  double waitTime;

  GetConnection()->SetName(libcomp::String("lobby_%1").Arg(username));

  ASSERT_TRUE_OR_RETURN(Connect(10666));
  ASSERT_TRUE_OR_RETURN(WaitEncrypted(waitTime));

  objects::PacketLogin obj;
  obj.SetClientVersion(CLIENT_VERSION);
  obj.SetUsername(username);

  libcomp::Packet p;
  p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_LOGIN);

  ASSERT_TRUE_OR_RETURN(obj.SavePacket(p));

  libcomp::ReadOnlyPacket reply;

  ClearMessages();
  GetConnection()->SendPacket(p);

  ASSERT_TRUE_OR_RETURN(
      WaitForPacket(LobbyToClientPacketCode_t::PACKET_LOGIN, reply, waitTime));
  ASSERT_EQ_OR_RETURN(reply.Left(), sizeof(int32_t) + sizeof(uint32_t) +
                                        sizeof(uint16_t) + 5 * 2);
  ASSERT_EQ_OR_RETURN(reply.ReadS32Little(),
                      to_underlying(ErrorCodes_t::SUCCESS));

  p.Clear();
  p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_AUTH);
  p.WriteString16Little(libcomp::Convert::ENCODING_UTF8, mSID1, true);

  ClearMessages();
  GetConnection()->SendPacket(p);

  ASSERT_TRUE_OR_RETURN(
      WaitForPacket(LobbyToClientPacketCode_t::PACKET_AUTH, reply, waitTime));

  if (!expectError) {
    ASSERT_EQ_OR_RETURN(reply.ReadS32Little(),
                        to_underlying(ErrorCodes_t::SUCCESS));

    libcomp::String newSID =
        reply.ReadString16Little(libcomp::Convert::ENCODING_UTF8, true);

    ASSERT_EQ_OR_RETURN(newSID.Length(), 300);

    mSID1 = newSID;
  } else {
    ASSERT_EQ_OR_RETURN(reply.ReadS32Little(),
                        to_underlying(ErrorCodes_t::BAD_USERNAME_PASSWORD));
  }

  ASSERT_EQ_OR_RETURN(reply.Left(), 0);

  return true;
}

bool LobbyClient::GetCharacterList() {
  double waitTime;

  mCharacters.clear();
  mCharacterLookup.clear();

  libcomp::Packet p;
  p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_CHARACTER_LIST);

  ClearMessages();
  GetConnection()->SendPacket(p);

  libcomp::ReadOnlyPacket reply;

  ASSERT_TRUE_OR_RETURN(WaitForPacket(
      LobbyToClientPacketCode_t::PACKET_CHARACTER_LIST, reply, waitTime));

  ASSERT_GE_OR_RETURN(reply.Left(), 6);

  uint32_t loginTime = reply.ReadU32Little();
  uint8_t ticketCount = reply.ReadU8();

  // We don't need these.
  mLoginTime = loginTime;
  mTicketCount = ticketCount;

  uint8_t characterCount = reply.ReadU8();

  LogGeneralDebug([&]() {
    return libcomp::String("Character Count: %1\n").Arg(characterCount);
  });

  for (uint8_t i = 0; i < characterCount; ++i) {
    auto c = std::make_shared<LobbyClient::Character>();

    c->cid = reply.ReadU8();
    c->wid = reply.ReadU8();

    c->name = reply.ReadString16Little(libcomp::Convert::ENCODING_CP932);

    c->gender = reply.ReadU8();
    c->killTime = reply.ReadU32Little();
    c->cutscene = reply.ReadU32Little();
    c->lastChannel = reply.ReadS8();
    c->level = reply.ReadS8();
    c->skinType = reply.ReadU8();
    c->hairType = reply.ReadU8();
    c->eyeType = reply.ReadU8();
    c->faceType = reply.ReadU8();
    c->hairColor = reply.ReadU8();
    c->leftEyeColor = reply.ReadU8();
    c->rightEyeColor = reply.ReadU8();
    c->unk1 = reply.ReadU8();
    c->unk2 = reply.ReadU8();

    for (int j = 0; j < 15; ++j) {
      c->equips[j] = reply.ReadU32Little();
    }

    uint32_t vaCount = reply.ReadU32Little();

    for (uint32_t j = 0; j < vaCount; ++j) {
      (void)reply.ReadS8();  // index
      c->va.push_back(reply.ReadU32Little());
    }

    mCharacters.push_back(c);
    mCharacterLookup[c->name.ToUtf8()] = c;
  }

  ASSERT_EQ_OR_RETURN(0, reply.Left());

  return true;
}

int8_t LobbyClient::GetCharacterID(const std::string& name) {
  auto it = mCharacterLookup.find(name);

  if (mCharacterLookup.end() == it) {
    return -1;
  } else {
    return (int8_t)it->second->cid;
  }
}

int8_t LobbyClient::GetWorldID(const std::string& name) {
  auto it = mCharacterLookup.find(name);

  if (mCharacterLookup.end() == it) {
    return -1;
  } else {
    return (int8_t)it->second->wid;
  }
}

uint32_t LobbyClient::GetLoginTime() const { return mLoginTime; }

uint8_t LobbyClient::GetTicketCount() const { return mTicketCount; }

uint32_t LobbyClient::GetTicketCost() const { return mTicketCount; }

uint32_t LobbyClient::GetCP() const { return mCP; }

bool LobbyClient::CreateCharacter(const libcomp::String& name) {
  double waitTime;

  int8_t world = 0;

  objects::Character::Gender_t gender = objects::Character::Gender_t::MALE;

  uint32_t skinType = 0x00000065;
  uint32_t faceType = 0x00000001;
  uint32_t hairType = 0x00000001;
  uint32_t hairColor = 0x00000008;
  uint32_t eyeColor = 0x00000008;

  uint32_t equipTop = 0x00000C3F;
  uint32_t equipBottom = 0x00000D64;
  uint32_t equipFeet = 0x00000DB4;
  uint32_t equipComp = 0x00001131;
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

  ASSERT_TRUE_OR_RETURN(WaitForPacket(
      LobbyToClientPacketCode_t::PACKET_CREATE_CHARACTER, reply, waitTime));

  ASSERT_EQ_OR_RETURN(reply.Left(), 4);
  ASSERT_EQ_OR_RETURN(reply.ReadS32Little(),
                      to_underlying(ErrorCodes_t::SUCCESS));

  return true;
}

bool LobbyClient::DeleteCharacter(uint8_t cid) {
  double waitTime;

  libcomp::Packet p;
  p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_DELETE_CHARACTER);
  p.WriteU8(cid);

  ClearMessages();
  GetConnection()->SendPacket(p);

  libcomp::ReadOnlyPacket reply;

  ASSERT_TRUE_OR_RETURN(WaitForPacket(
      LobbyToClientPacketCode_t::PACKET_DELETE_CHARACTER, reply, waitTime));

  ASSERT_EQ_OR_RETURN(reply.Left(), 1);
  ASSERT_EQ_OR_RETURN(reply.ReadS8(), cid);

  return true;
}

bool LobbyClient::QueryTicketPurchase() {
  double waitTime;

  libcomp::Packet p;
  p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_QUERY_PURCHASE_TICKET);
  p.WriteU8(1);

  ClearMessages();
  GetConnection()->SendPacket(p);

  libcomp::ReadOnlyPacket reply;

  ASSERT_TRUE_OR_RETURN(
      WaitForPacket(LobbyToClientPacketCode_t::PACKET_QUERY_PURCHASE_TICKET,
                    reply, waitTime));

  ASSERT_EQ_OR_RETURN(reply.Left(), 13);
  reply.Skip(5);
  mTicketCost = reply.ReadU32Little();
  mCP = reply.ReadU32Little();

  return true;
}

bool LobbyClient::StartGame(uint8_t cid, int8_t worldID) {
  double waitTime;

  libcomp::Packet p;
  p.WritePacketCode(ClientToLobbyPacketCode_t::PACKET_START_GAME);
  p.WriteU8(cid);
  p.WriteS8(worldID);

  ClearMessages();
  GetConnection()->SendPacket(p);

  libcomp::ReadOnlyPacket reply;

  UPHOLD_TRUE(WaitForPacket(LobbyToClientPacketCode_t::PACKET_START_GAME, reply,
                            waitTime));

  int32_t sessionKey;
  uint8_t cid2;

  ASSERT_GT_OR_RETURN(reply.Left(),
                      sizeof(sessionKey) + sizeof(uint16_t) + sizeof(cid2));

  sessionKey = reply.ReadS32Little();

  libcomp::String server =
      reply.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

  cid2 = reply.ReadU8();

  ASSERT_EQ_OR_RETURN(cid, cid2);
  ASSERT_FALSE_OR_RETURN(server.IsEmpty());
  ASSERT_GT_OR_RETURN(sessionKey, -1);

  // Save the session key.
  mSessionKey = sessionKey;

  return true;
}

int32_t LobbyClient::GetSessionKey() const { return mSessionKey; }

void LobbyClient::SetWaitForLogout(bool wait) { mWaitForLogout = wait; }

namespace libcomp {
template <>
ScriptEngine& ScriptEngine::Using<LobbyClient>() {
  if (!BindingExists("LobbyClient")) {
    // Include the base class
    Using<TestClient>();

    Sqrat::DerivedClass<LobbyClient, TestClient> binding(mVM, "LobbyClient");
    binding.Func("ClassicLogin", &LobbyClient::ClassicLogin);
    binding.Func("WebLogin", &LobbyClient::WebLogin);
    binding.Func("GetCharacterList", &LobbyClient::GetCharacterList);
    binding.Func("CreateCharacter", &LobbyClient::CreateCharacter);
    binding.Func("DeleteCharacter", &LobbyClient::DeleteCharacter);
    binding.Func("QueryTicketPurchase", &LobbyClient::QueryTicketPurchase);
    binding.Func("StartGame", &LobbyClient::StartGame);
    binding.Func("GetSessionKey", &LobbyClient::GetSessionKey);
    binding.Func("GetCharacterID", &LobbyClient::GetCharacterID);
    binding.Func("GetLoginTime", &LobbyClient::GetLoginTime);
    binding.Func("GetTicketCount", &LobbyClient::GetTicketCount);
    binding.Func("GetTicketCost", &LobbyClient::GetTicketCost);
    binding.Func("GetCP", &LobbyClient::GetCP);

    Bind<LobbyClient>("LobbyClient", binding);
  }

  return *this;
}  // Using
}  // namespace libcomp
