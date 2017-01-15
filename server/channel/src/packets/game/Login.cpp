/**
 * @file server/channel/src/packets/Login.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to log in.
 *
 * This file is part of the Channel Server (channel).
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

#include "Packets.h"

// libcomp Includes
#include <ManagerPacket.h>
#include <Packet.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelClientConnection.h"
#include "ChannelServer.h"

using namespace channel;

void LoginAccount(AccountManager* accountManager,
    std::shared_ptr<ChannelClientConnection> client, const libcomp::String username,
    uint32_t sessionKey)
{
    accountManager->Login(client, username, sessionKey);
}

bool Parsers::Login::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    // Classic authentication method: username followed by the session key
    libcomp::String username = p.ReadString16(libcomp::Convert::ENCODING_UTF8);
    uint32_t sessionKey = p.ReadU32Little();

    // Remove null terminator
    username = username.C();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    server->QueueWork(LoginAccount, server->GetAccountManager(), client, username, sessionKey);

    return true;
}
