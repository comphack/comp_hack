/**
 * @file server/lobby/src/packets/Login.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to a request to login to the lobby.
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

// lobby Includes
#include "LobbyClientConnection.h"
#include "LobbyServer.h"

// libcomp Includes
#include <ErrorCodes.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// object Includes
#include <Account.h>
#include <PacketLogin.h>
#include <PacketResponseCode.h>

using namespace lobby;

bool Parsers::Login::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;

    objects::PacketLogin obj;

    auto conf = config(pPacketManager);

    if(!obj.LoadPacket(p) || nullptr == conf)
    {
        return false;
    }

    objects::PacketResponseCode reply;
    reply.SetCommandCode(to_underlying(
        LobbyClientPacketCode_t::PACKET_LOGIN_RESPONSE));

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto accountManager = server->GetAccountManager();
    auto mainDB = server->GetMainDatabase();

    /// @todo Ask the world server is the user is already logged in.
    auto account = objects::Account::LoadAccountByUsername(mainDB, obj.GetUsername());

    uint32_t clientVersion = static_cast<uint32_t>(
        conf->GetClientVersion() * 1000.0f);

    if(clientVersion != obj.GetClientVersion())
    {
        reply.SetResponseCode(to_underlying(
            ErrorCodes_t::WRONG_CLIENT_VERSION));
    }
    else if(nullptr == account)
    {
        reply.SetResponseCode(to_underlying(
            ErrorCodes_t::BAD_USERNAME_PASSWORD));
    }
    else
    {
        auto login = std::shared_ptr<objects::AccountLogin>(new objects::AccountLogin);
        login->SetAccount(account);
        accountManager->LoginUser(obj.GetUsername(), login);

        state(connection)->SetAccount(account);
        server->GetManagerConnection()->SetClientConnection(
            std::dynamic_pointer_cast<LobbyClientConnection>(connection));

        reply.SetResponseCode(to_underlying(
            ErrorCodes_t::SUCCESS));
    }

    return connection->SendObject(reply);
}
