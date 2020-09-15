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

#ifndef LIBTESTER_SRC_LOBBYCLIENT_H
#define LIBTESTER_SRC_LOBBYCLIENT_H

// libtester Includes
#include "TestClient.h"

// Standard C++ Includes
#include <unordered_map>

namespace libtester {

typedef std::list<libcomp::Message::Message*> MessageList;

class LobbyClient : public TestClient {
 public:
  struct Character {
    uint8_t cid;
    uint8_t wid;
    libcomp::String name;
    uint8_t gender;
    uint32_t killTime;
    uint32_t cutscene;
    int8_t lastChannel;
    int8_t level;
    uint8_t skinType;
    uint8_t hairType;
    uint8_t eyeType;
    uint8_t faceType;
    uint8_t hairColor;
    uint8_t leftEyeColor;
    uint8_t rightEyeColor;
    uint8_t unk1;
    uint8_t unk2;
    uint32_t equips[15];
    std::vector<uint32_t> va;
  };

  LobbyClient();
  LobbyClient(const LobbyClient& other);
  virtual ~LobbyClient();

  bool WaitForPacket(LobbyToClientPacketCode_t code, libcomp::ReadOnlyPacket& p,
                     double& waitTime,
                     asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);

  bool Login(const libcomp::String& username, const libcomp::String& password,
             ErrorCodes_t loginErrorCode = ErrorCodes_t::SUCCESS,
             ErrorCodes_t authErrorCode = ErrorCodes_t::SUCCESS,
             uint32_t clientVersion = 0);
  bool ClassicLogin(const libcomp::String& username,
                    const libcomp::String& password);
  bool WebLogin(const libcomp::String& username,
                const libcomp::String& password = libcomp::String(),
                const libcomp::String& sid = libcomp::String(),
                bool expectError = false);

  bool GetCharacterList();
  bool CreateCharacter(const libcomp::String& name);
  bool DeleteCharacter(uint8_t cid);
  bool QueryTicketPurchase();
  bool StartGame(uint8_t cid = 0, int8_t wid = 0);
  void SetWaitForLogout(bool wait);

  int32_t GetSessionKey() const;

  int8_t GetCharacterID(const std::string& name);
  int8_t GetWorldID(const std::string& name);

  uint32_t GetLoginTime() const;
  uint8_t GetTicketCount() const;
  uint32_t GetTicketCost() const;
  uint32_t GetCP() const;

  void SetLobbyAddress(const libcomp::String& address);
  void SetLobbyPort(uint16_t port);

  libcomp::String GetChannelAddress() const;
  uint16_t GetChannelPort() const;

 private:
  libcomp::String mSID1, mSID2;
  int32_t mSessionKey;
  bool mWaitForLogout;
  uint32_t mLoginTime;
  uint8_t mTicketCount;
  uint32_t mTicketCost;
  uint32_t mCP;

  libcomp::String mLobbyAddress;
  uint16_t mLobbyPort;

  libcomp::String mChannelAddress;
  uint16_t mChannelPort;

  std::vector<std::shared_ptr<Character>> mCharacters;
  std::unordered_map<std::string, std::shared_ptr<Character>> mCharacterLookup;
};

}  // namespace libtester

#endif  // LIBTESTER_SRC_LOBBYCLIENT_H
