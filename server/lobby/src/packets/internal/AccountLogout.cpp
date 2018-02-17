/**
 * @file server/lobby/src/packets/internal/AccountLogout.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Parser to handle logging out an account.
 *
 * This file is part of the Lobby Server (lobby).
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
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// libcomp Includes
#include <AccountLogin.h>
#include <CharacterLogin.h>

// lobby Includes
#include "LobbyServer.h"

using namespace lobby;

bool Parsers::AccountLogout::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    libcomp::String username = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
    bool channelSwitch = p.Left() > 4 &&
        p.ReadU32Little() == (uint32_t)LogoutPacketAction_t::LOGOUT_CHANNEL_SWITCH;

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto accountManager = server->GetAccountManager();

    auto login = accountManager->GetUserLogin(username);
    if(!login)
    {
        return false;
    }

    auto cLogin = login->GetCharacterLogin();
    if(channelSwitch)
    {
        cLogin->SetChannelID(p.ReadS8());
        login->SetSessionKey(p.ReadU32Little());
    }
    else
    {
        // Do not log out the user if they connected back to the lobby
        if(cLogin->GetWorldID() != -1)
        {
            LOG_DEBUG(libcomp::String("Logging out user: '%1'\n").Arg(username));
            accountManager->LogoutUser(username, cLogin->GetWorldID());
        }
    }

    return true;
}
