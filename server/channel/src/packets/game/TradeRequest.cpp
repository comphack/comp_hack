/**
 * @file server/channel/src/packets/game/TradeRequest.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to request a trade with another player.
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <PlayerExchangeSession.h>

// channel Includes
#include "ChannelServer.h"
#include "ClientState.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::TradeRequest::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 4) {
    return false;
  }

  uint32_t targetEntityID = p.ReadU32Little();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto otherClient =
      server->GetManagerConnection()->GetEntityClient((int32_t)targetEntityID);
  auto otherState = otherClient ? otherClient->GetClientState() : nullptr;
  auto otherCState = otherState ? otherState->GetCharacterState() : nullptr;

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_REQUEST);

  if (!otherClient || state->GetExchangeSession() ||
      otherState->GetExchangeSession() ||
      cState->GetDistance(otherCState->GetCurrentX(),
                          otherCState->GetCurrentY()) > MAX_INTERACT_DISTANCE) {
    reply.WriteS32Little(-1);
    client->SendPacket(reply);
    return true;
  }

  // Set the trade session info
  auto exchangeSession = std::make_shared<objects::PlayerExchangeSession>();
  exchangeSession->SetSourceEntityID(cState->GetEntityID());
  exchangeSession->SetType(objects::PlayerExchangeSession::Type_t::TRADE);
  exchangeSession->SetOtherCharacterState(otherState->GetCharacterState());
  state->SetExchangeSession(exchangeSession);

  auto otherSession = std::make_shared<objects::PlayerExchangeSession>();
  exchangeSession->SetSourceEntityID(
      otherState->GetCharacterState()->GetEntityID());
  otherSession->SetType(objects::PlayerExchangeSession::Type_t::TRADE);
  otherSession->SetOtherCharacterState(cState);
  otherState->SetExchangeSession(otherSession);

  reply.WriteS32Little(0);

  libcomp::Packet p2;
  p2.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_REQUESTED);
  p2.WriteS32Little(cState->GetEntityID());

  otherClient->SendPacket(p2);

  client->SendPacket(reply);

  return true;
}
