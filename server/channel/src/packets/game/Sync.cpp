/**
 * @file server/channel/src/packets/game/Sync.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to sync with the server time.
 *
 * This file is part of the Channel Server (channel).
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
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerPacket.h"

using namespace channel;

bool Parsers::Sync::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();

  uint32_t timeFromClient = p.ReadU32Little();
  ServerTime currentServerTime = ChannelServer::GetServerTime();
  ClientTime currentClientTime = state->ToClientTime(currentServerTime);

  // Respond with the time received from the client (appears to be based off
  // the executing system time) then the amount of time elapsed since the
  // client state connection started.
  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SYNC);
  reply.WriteU32Little(timeFromClient);
  reply.WriteFloat(currentClientTime);

  connection->SendPacket(reply);

  ServerTime currentClientTimeInServerTime = (ServerTime)timeFromClient * 1000;
  ServerTime lastServerTime = state->GetLastServerTimestamp();
  ServerTime lastClientTime = state->GetLastClientTimestamp();

  if (lastServerTime) {
    auto server =
        std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

    ServerTime serverDelta = currentServerTime - lastServerTime;
    ServerTime clientDelta = currentClientTimeInServerTime - lastClientTime;

    float serverDeltaF = (float)serverDelta;
    float clientDeltaF = (float)clientDelta;
    float delta = clientDeltaF;  // - serverDeltaF;

    auto worldSharedConfig = server->GetWorldSharedConfig();
    float threshold = worldSharedConfig->GetClockSkewThreshold() * serverDeltaF;
    float skew = delta;  // + serverDeltaF;

    if (threshold > 0.0f && skew >= threshold) {
      auto account = state->GetAccountLogin()->GetAccount();
      auto skewCount = state->GetClockSkewCount();

      if (skewCount >= worldSharedConfig->GetClockSkewCount()) {
        LogGeneralError([&]() {
          auto username = account->GetUsername();

          return libcomp::String(
                     "Account %1 is running a clock that is %2x normal. This "
                     "is over the limit and they have been kicked/banned.\n")
              .Arg(username)
              .Arg(skew / serverDeltaF);
        });

        if (worldSharedConfig->GetAutobanClockSkew()) {
          account->SetEnabled(false);
          account->SetBanReason(
              "Account is running the client too fast (possible cheating).");
          account->SetBanInitiator("<channel server>");
          account->Update(server->GetLobbyDatabase());
        }

        client->Close();
      } else {
        skewCount++;

        LogGeneralInfo([&]() {
          auto username = account->GetUsername();

          return libcomp::String(
                     "Account %1 is running a clock that is %2x normal. This "
                     "is over the limit. This has happened %3/%4 times.\n")
              .Arg(username)
              .Arg(skew / serverDeltaF)
              .Arg(skewCount)
              .Arg(worldSharedConfig->GetClockSkewCount());
        });

        state->SetClockSkewCount(skewCount);
      }
    }
  }

  state->SetLastServerTimestamp(currentServerTime);
  state->SetLastClientTimestamp(currentClientTimeInServerTime);

  return true;
}
