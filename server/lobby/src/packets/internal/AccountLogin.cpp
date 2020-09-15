/**
 * @file server/lobby/src/packets/internal/AccountLogin.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Parser to handle the response for retrieving a channel for the
 * client to log into.
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
#include <InternalConnection.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// lobby Includes
#include "AccountManager.h"
#include "LobbyServer.h"
#include "ManagerConnection.h"

using namespace lobby;

void UpdateAccountLogin(std::shared_ptr<LobbyServer> server,
                        const std::shared_ptr<objects::AccountLogin> login) {
  auto cLogin = login->GetCharacterLogin();
  auto character = cLogin->GetCharacter();
  auto worldID = cLogin->GetWorldID();
  auto channelID = cLogin->GetChannelID();

  if (0 > worldID || 0 > channelID) {
    LogGeneralError([&]() {
      return libcomp::String(
                 "Invalid channel (%1) or world (%2) ID received for "
                 "AccountLogin.\n")
          .Arg(channelID)
          .Arg(worldID);
    });

    return;
  }

  auto world = server->GetWorldByID((uint8_t)worldID);

  if (!world) {
    return;
  }

  // Should be the same one we passed in.
  auto account = login->GetAccount().Get(server->GetMainDatabase());

  if (!account) {
    return;
  }

  auto channel = world->GetChannelByID((uint8_t)channelID);

  if (!channel) {
    LogGeneralErrorMsg("Unknown channel ID returned from the world.\n");

    return;
  }

  auto username = account->GetUsername();
  auto accountManager = server->GetAccountManager();

  int8_t currentWorldID;

  if (!accountManager->IsLoggedIn(username, currentWorldID)) {
    return;
  }

  auto clientConnection = server->GetManagerConnection()->GetClientConnection(
      account->GetUsername());

  if (clientConnection && currentWorldID == -1) {
    // Initial login response from the world.
    LogGeneralDebug([&]() {
      return libcomp::String(
                 "Login character with UUID '%1' into world %2, channel %3 "
                 "using session key: %4\n")
          .Arg(character.GetUUID().ToString())
          .Arg(worldID)
          .Arg(channelID)
          .Arg(login->GetSessionKey());
    });

    libcomp::Packet reply;
    reply.WritePacketCode(LobbyToClientPacketCode_t::PACKET_START_GAME);
    reply.WriteU32Little(login->GetSessionKey());
    reply.WriteString16Little(
        libcomp::Convert::ENCODING_UTF8,
        libcomp::String("%1:%2").Arg(channel->GetIP()).Arg(channel->GetPort()),
        true);
    reply.WriteS32Little(channelID);

    clientConnection->SendPacket(reply);

    // Switch channels now.
    accountManager->SwitchToChannel(username, worldID, channelID);
  } else {
    // We are now in the channel so update the login state.
    accountManager->CompleteChannelLogin(username, worldID, channelID);
  }
}

bool Parsers::AccountLogin::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() < 1) {
    LogGeneralErrorMsg("Invalid response received for AccountLogin.\n");

    return false;
  }

  auto server =
      std::dynamic_pointer_cast<LobbyServer>(pPacketManager->GetServer());

  int8_t errorCode = p.ReadS8();
  if (errorCode == 1) {
    // No error
    auto response =
        std::shared_ptr<objects::AccountLogin>(new objects::AccountLogin);

    if (!response->LoadPacket(p, false)) {
      p.Rewind();

      if (sizeof(int8_t) == p.Size() && 0 == p.PeekS8()) {
        // This error is expected, ignore it.
        return true;
      } else {
        LogGeneralErrorMsg(
            "Invalid response received for AccountLogin (lobby).\n");

        p.HexDump();

        return false;
      }
    }

    server->QueueWork(UpdateAccountLogin, server, response);
  } else if (p.Left() > 2 && p.Left() == (uint16_t)(2 + p.PeekU16Little())) {
    // Failure, disconnect the client if they're here
    auto username =
        p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
    auto client = server->GetManagerConnection()->GetClientConnection(username);
    if (nullptr != client) {
      client->Close();
    }
  } else {
    LogGeneralErrorMsg(
        "World server sent a malformed AccountLogin message! Killing the "
        "connection...\n");

    connection->Close();
  }

  return true;
}
