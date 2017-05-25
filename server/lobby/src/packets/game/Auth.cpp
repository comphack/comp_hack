/**
 * @file server/lobby/src/packets/game/Auth.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet parser to handle authorizing a session with the lobby.
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
#include "LobbyServer.h"

// libcomp Includes
#include <Decrypt.h>
#include <ErrorCodes.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Account.h>

using namespace lobby;

static bool LoginError(const std::shared_ptr<
    libcomp::TcpConnection>& connection)
{
    /// @todo Return different error codes like an account is banned.
    libcomp::Packet reply;
    reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_AUTH);
    reply.WriteS32Little(to_underlying(ErrorCodes_t::BAD_USERNAME_PASSWORD));

    connection->SendPacket(reply);

    return true;
}

static bool CompleteLogin(
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    const std::shared_ptr<LobbyServer>& server,
    const libcomp::ObjectReference<objects::Account>& account,
    const libcomp::String& username,
    const libcomp::String& sid2,
    bool isLoggedIn, int8_t loginWorldID)
{
    auto accountManager = server->GetAccountManager();

    if(!isLoggedIn || loginWorldID != -1)
    {
        if(isLoggedIn)
        {
            accountManager->LogoutUser(username, loginWorldID);
        }

        auto login = accountManager->GetUserLogin(username);
        if(nullptr == login)
        {
            login = std::shared_ptr<objects::AccountLogin>(new objects::AccountLogin);
            login->SetAccount(account);
        }

        login->SetWorldID(-1);
        login->SetChannelID(-1);

        if(!accountManager->LoginUser(username, login))
        {
            LOG_ERROR(libcomp::String("User '%1' could not be logged in.\n").Arg(username));
            return false;
        }
    }

    state(connection)->SetAuthenticated(true);

    libcomp::Packet reply;
    reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_AUTH);

    // Status code (see the Login handler for a list).
    reply.WriteS32Little(to_underlying(ErrorCodes_t::SUCCESS));

    accountManager->UpdateSessionID(username, sid2);

    LOG_DEBUG(libcomp::String("New SID: %1\n").Arg(sid2));

    // Write a new session ID to be used when the client switches channels.
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8, sid2, true);

    connection->SendPacket(reply);

    return true;
}

static bool NoWebAuthParse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p)
{
    if(p.Size() != 131 || p.PeekU16Little() != 129)
    {
        return false;
    }

    libcomp::String hash = p.ReadString16Little(
        libcomp::Convert::ENCODING_UTF8, true).ToLower();

    // Authentication hash provided by the patched client.
    LOG_DEBUG(libcomp::String("Hash: %1\n").Arg(hash));

    /// @todo retrieve the challenge and add to hash before checking.

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto account = state(connection)->GetAccount();
    auto username = account->GetUsername();
    auto accountManager = server->GetAccountManager();
    auto sessionManager = server->GetSessionManager();

    libcomp::String sid2;
    int8_t loginWorldID;
    bool isLoggedIn = accountManager->IsLoggedIn(username, loginWorldID);

    if(hash != account->GetPassword())
    {
        LOG_ERROR(libcomp::String("User '%1' password hash provided by the "
            "client was not valid: %2\n").Arg(username).Arg(hash));
        accountManager->LogoutUser(username, loginWorldID);
        return LoginError(connection);
    }

    auto sids = sessionManager->GenerateSIDs(username);

    if(!sessionManager->CheckSID(username, sids.first, sid2))
    {
        LOG_ERROR(libcomp::String("User '%1' session ID provided by the server"
            " was not valid: %2\n").Arg(username).Arg(sids.first));
        accountManager->LogoutUser(username, loginWorldID);
        return LoginError(connection);
    }

    return CompleteLogin(connection, server, account, username, sid2,
        isLoggedIn, loginWorldID);
}

bool Parsers::Auth::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 303 || p.PeekU16Little() != 301)
    {
        return NoWebAuthParse(pPacketManager, connection, p);
    }

    auto server = std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());
    auto account = state(connection)->GetAccount();
    auto username = account->GetUsername();
    auto accountManager = server->GetAccountManager();
    auto sessionManager = server->GetSessionManager();

    // Authentication token (session ID) provided by the web server.
    libcomp::String sid = p.ReadString16Little(
        libcomp::Convert::ENCODING_UTF8, true).ToLower();

    LOG_DEBUG(libcomp::String("SID: %1\n").Arg(sid));

    libcomp::String sid2;
    int8_t loginWorldID;
    bool isLoggedIn = accountManager->IsLoggedIn(username, loginWorldID);

    if(!sessionManager->CheckSID(username, sid, sid2))
    {
        LOG_ERROR(libcomp::String("User '%1' session ID provided by the client"
            " was not valid: %2\n").Arg(username).Arg(sid));
        accountManager->LogoutUser(username, loginWorldID);
        return LoginError(connection);
    }

    return CompleteLogin(connection, server, account, username, sid2,
        isLoggedIn, loginWorldID);
}
