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

#ifndef LIBTESTER_SRC_LOBBYCLIENT_H
#define LIBTESTER_SRC_LOBBYCLIENT_H

// libtester Includes
#include "TestClient.h"

namespace libtester
{

typedef std::list<libcomp::Message::Message*> MessageList;

class LobbyClient : public TestClient
{
public:
    LobbyClient();
    ~LobbyClient();

    bool WaitForPacket(LobbyToClientPacketCode_t code,
        libcomp::ReadOnlyPacket& p, double& waitTime,
        asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);

    void Login(const libcomp::String& username,
        const libcomp::String& password, ErrorCodes_t loginErrorCode =
            ErrorCodes_t::SUCCESS, ErrorCodes_t authErrorCode =
            ErrorCodes_t::SUCCESS, uint32_t clientVersion = 0);
    void WebLogin(const libcomp::String& username,
        const libcomp::String& password = libcomp::String(),
        const libcomp::String& sid = libcomp::String(),
        bool expectError = false);
    void CreateCharacter(const libcomp::String& name);
    void StartGame();
    void SetWaitForLogout(bool wait);

    int32_t GetSessionKey() const;

private:
    libcomp::String mSID1, mSID2;
    int32_t mSessionKey;
    bool mWaitForLogout;
};

} // namespace libtester

#endif // LIBTESTER_SRC_LOBBYCLIENT_H
