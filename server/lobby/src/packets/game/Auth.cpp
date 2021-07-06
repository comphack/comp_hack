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

#include "Packets.h"

// libcomp Includes
#include <Crypto.h>
#include <ErrorCodes.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <CharacterLogin.h>

// lobby Includes
#include "AccountManager.h"
#include "LobbyServer.h"
#include "ManagerConnection.h"

using namespace lobby;

static bool LoginAuthError(
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    ErrorCodes_t errorCode) {
  libcomp::Packet reply;
  reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_AUTH);
  reply.WriteS32Little(to_underlying(errorCode));

  connection->SendPacket(reply);

  return true;
}

static bool CompleteLogin(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<LobbyServer>& server,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    const libcomp::String& sid, const libcomp::String& username,
    const libcomp::String& machineUUID = {}) {
  libcomp::String sid2;

  auto accountManager = server->GetAccountManager();

  ErrorCodes_t errorCode;

  auto conf = config(pPacketManager);
  int32_t maxClients = conf->GetMaxClients();

  if (sid.IsEmpty()) {
    // Login from NoWebAuth
    errorCode =
        accountManager->LobbyLogin(username, sid2, maxClients, machineUUID);
  } else {
    // Login from WebAuth
    errorCode = accountManager->LobbyLogin(username, sid, sid2, maxClients);
  }

  if (ErrorCodes_t::SUCCESS == errorCode) {
    LogGeneralDebug([&]() {
      return libcomp::String("New SID for user '%1': %2\n")
          .Arg(username)
          .Arg(sid2);
    });

    state(connection)->SetAuthenticated(true);

    // Register the client so they logout on disconnect.
    server->GetManagerConnection()->SetClientConnection(
        std::dynamic_pointer_cast<LobbyClientConnection>(connection));

    libcomp::Packet reply;
    reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_AUTH);
    reply.WriteS32Little(to_underlying(ErrorCodes_t::SUCCESS));
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8, sid2, true);

    connection->SendPacket(reply);

    if (objects::LobbyConfig::ClientPatchEnforcement_t::NONE !=
        conf->GetClientPatchEnforcement()) {
      libcomp::Packet reply2;
      reply2.WritePacketCode(
          LobbyToClientPacketCode_t::PACKET_AMALA_QUERY_CLIENT_PATCH);

      connection->SendPacket(reply2);
    }
  } else {
    return LoginAuthError(connection, errorCode);
  }

  return true;
}

static bool NoWebAuthParse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<LobbyServer>& server,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p, const libcomp::String& username) {
  libcomp::String machineUUID;

  // Get the hash provided by the client.
  libcomp::String hash =
      p.ReadString16Little(libcomp::Convert::ENCODING_UTF8, true).ToLower();

  auto conf = config(pPacketManager);
  int32_t maxClients = conf->GetMaxClients();

  // Sanity check the machine UUID.
  if (165 == hash.Length()) {
    // Check for the slash
    if ('/' != hash.At(128)) {
      return LoginAuthError(connection, ErrorCodes_t::NOT_AUTHORIZED);
    }

    machineUUID = hash.Mid(129);
    hash = hash.Left(128);

    static const std::regex uuidExpr(
        "^[0-9a-f]{8}\\-[0-9a-f]{4}\\-[0-9a-f]{4}\\-[0-9a-f]{4}\\-[0-9a-f]{12}"
        "$");

    // Check for the UUID format.
    if (!std::regex_match(machineUUID.ToUtf8(), uuidExpr)) {
      return LoginAuthError(connection, ErrorCodes_t::NOT_AUTHORIZED);
    }
  } else if (0 < maxClients) {
    // Report this error only if we have enabled a max number of clients.
    LogAccountManagerError([&]() {
      return libcomp::String(
                 "Account '%1' did not provide a machine UUID. They did not "
                 "update the comp_client.dll!\n")
          .Arg(username);
    });

    return LoginAuthError(connection, ErrorCodes_t::NOT_AUTHORIZED);
  }

  // Authentication hash provided by the patched client.
  LogGeneralDebug([&]() { return libcomp::String("Hash: %1\n").Arg(hash); });

  // Get the account so we may check the password hash.
  auto account = state(connection)->GetAccount();

  // Make sure the account is valid first.
  if (!account) {
    return LoginAuthError(connection, ErrorCodes_t::BAD_USERNAME_PASSWORD);
  }

  // Calculate the password hash with the challenge given.
  libcomp::String challenge = libcomp::Crypto::HashPassword(
      account->GetPassword(),
      libcomp::String("%1").Arg(state(connection)->GetChallenge()));

  // The hash from the client must match for a proper authentication.
  if (hash != challenge) {
    LogGeneralError([&]() {
      return libcomp::String(
                 "User '%1' password hash provided by the client was not "
                 "valid: %2\n")
          .Arg(username)
          .Arg(hash);
    });

    return LoginAuthError(connection, ErrorCodes_t::BAD_USERNAME_PASSWORD);
  }

  // Prevent game access for API only accounts
  if (account->GetAPIOnly()) {
    LogGeneralError([&]() {
      return libcomp::String(
                 "API only account '%1' attempted to login via NoWebAuth "
                 "method\n")
          .Arg(username);
    });

    return LoginAuthError(connection, ErrorCodes_t::NOT_AUTHORIZED);
  }

  return CompleteLogin(pPacketManager, server, connection, libcomp::String(),
                       username, machineUUID);
}

static bool WebAuthParse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<LobbyServer>& server,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p, const libcomp::String& username) {
  // Authentication token (session ID) provided by the web server.
  libcomp::String sid =
      p.ReadString16Little(libcomp::Convert::ENCODING_UTF8, true).ToLower();

  LogGeneralDebug([&]() {
    return libcomp::String("SID for user '%1': %2\n").Arg(username).Arg(sid);
  });

  return CompleteLogin(pPacketManager, server, connection, sid, username);
}

bool Parsers::Auth::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  auto username = state(connection)->GetUsername();

  auto server =
      std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());

  if ((p.Size() == 131 && p.PeekU16Little() == 129) ||
      (p.Size() == 168 && p.PeekU16Little() == 166)) {
    return NoWebAuthParse(pPacketManager, server, connection, p, username);
  } else if (p.Size() == 303 && p.PeekU16Little() == 301) {
    return WebAuthParse(pPacketManager, server, connection, p, username);
  } else {
    return false;
  }
}
