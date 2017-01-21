/**
 * @file server/channel/src/packets/Logout.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to log out.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelClientConnection.h"
#include "ChannelServer.h"

using namespace channel;

void LogoutAccount(AccountManager* accountManager,
    const std::shared_ptr<ChannelClientConnection>& client,
    LogoutCode_t code, uint8_t channel)
{
    accountManager->HandleLogoutRequest(client, code, channel);
}

bool Parsers::Logout::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 4)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);

    uint32_t codeValue = p.ReadU32Little();
    if(LogoutCode_t::LOGOUT_CODE_UNKNOWN_MIN >= codeValue ||
        LogoutCode_t::LOGOUT_CODE_UNKNOWN_MAX <= codeValue)
    {
        LOG_ERROR(libcomp::String("Unknown logout code: %1\n").Arg(codeValue));
        return false;
    }

    LogoutCode_t code = (LogoutCode_t)codeValue;
    if((code == LogoutCode_t::LOGOUT_CODE_SWITCH && p.Size() != 5) ||
        (code != LogoutCode_t::LOGOUT_CODE_SWITCH && p.Size() != 4))
    {
        return false;
    }

    bool sendReply = false;
    uint8_t channelID = 0;
    switch(code)
    {
        case LogoutCode_t::LOGOUT_CODE_QUIT:
            sendReply = true;
            break;
        case LogoutCode_t::LOGOUT_CODE_CANCEL:
            // Cancel is not currently supported
            break;
        case LogoutCode_t::LOGOUT_CODE_SWITCH:
            channelID = p.ReadU8();
            sendReply = true;
            return false;   /// @todo: handle switch properly
            break;
        default:
            break;
    }

    if(sendReply)
    {
        server->QueueWork(LogoutAccount, server->GetAccountManager(), client,
            code, channelID);
    }

    return true;
}
