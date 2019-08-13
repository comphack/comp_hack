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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// libcomp Includes
#include <AccountLogin.h>
#include <CharacterLogin.h>

// lobby Includes
#include "AccountManager.h"
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
        LogGeneralError([&]()
        {
            return libcomp::String("World requested logout for an account that"
                " is not currently logged in: '%1'\n").Arg(username);
        });

        return true;
    }

    auto cLogin = login->GetCharacterLogin();
    if(channelSwitch)
    {
        int8_t channelID = p.ReadS8();
        uint32_t sessionKey = p.ReadU32Little();

        if(!accountManager->ChannelToChannelSwitch(username, channelID, sessionKey))
        {
            LogGeneralError([&]()
            {
                return libcomp::String("Failed to set channel to channel "
                    "switch for account: '%1'\n").Arg(username);
            });
        }
    }
    else
    {
        // Do not log out the user if they connected back to the lobby
        if(cLogin->GetWorldID() != -1)
        {
            LogGeneralDebug([&]()
            {
                return libcomp::String("Logging out user: '%1'\n")
                    .Arg(username);
            });

            accountManager->Logout(username);
        }
    }

    return true;
}
