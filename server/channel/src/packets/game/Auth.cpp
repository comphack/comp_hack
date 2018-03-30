/**
 * @file server/channel/src/packets/game/Auth.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to authenticate.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"

using namespace channel;

void AuthenticateAccount(AccountManager* accountManager,
    const std::shared_ptr<ChannelClientConnection> client)
{
    accountManager->Authenticate(client);
}

bool Parsers::Auth::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    if(p.Size() != 43 || p.PeekU16Little() != 41)
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    server->QueueWork(AuthenticateAccount, server->GetAccountManager(), client);

    return true;
}
