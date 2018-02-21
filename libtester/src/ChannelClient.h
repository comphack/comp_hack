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

#ifndef LIBTESTER_SRC_CHANNELCLIENT_H
#define LIBTESTER_SRC_CHANNELCLIENT_H

// libtester Includes
#include "TestClient.h"

namespace libtester
{

typedef std::list<libcomp::Message::Message*> MessageList;

class ChannelClient : public TestClient
{
public:
    ChannelClient();
    ChannelClient(const ChannelClient& other);
    ~ChannelClient();

    bool WaitForPacket(ChannelToClientPacketCode_t code,
        libcomp::ReadOnlyPacket& p, double& waitTime,
        asio::steady_timer::duration timeout = DEFAULT_TIMEOUT);

    void Login(const libcomp::String& username,
        const libcomp::String& password,
        const libcomp::String& characterName = libcomp::String());
    void SendData();

    int32_t GetEntityID() const;

protected:
    virtual void HandlePacket(ChannelToClientPacketCode_t cmd,
        libcomp::ReadOnlyPacket& p);

private:
    void HandleCharacterData(libcomp::ReadOnlyPacket& p);

    int32_t mEntityID;
};

} // namespace libtester

#endif // LIBTESTER_SRC_CHANNELCLIENT_H
