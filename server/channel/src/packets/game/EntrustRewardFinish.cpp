/**
 * @file server/channel/src/packets/game/EntrustRewardFinish.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client by the entrust target to finish
 *  rewards and await confirmation.
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
#include "CharacterManager.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::EntrustRewardFinish::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 4) {
    return false;
  }

  int32_t choice = p.ReadS32Little();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto exchangeSession = state->GetExchangeSession();

  auto otherClient = exchangeSession && exchangeSession->GetSourceEntityID() !=
                                            cState->GetEntityID()
                         ? server->GetManagerConnection()->GetEntityClient(
                               exchangeSession->GetSourceEntityID(), false)
                         : nullptr;

  libcomp::Packet reply;
  reply.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_ENTRUST_REWARD_FINISH);
  reply.WriteS8((int8_t)choice);

  if (otherClient) {
    exchangeSession->SetLocked(true);
    otherClient->SendPacketCopy(reply);

    reply.WriteS32Little(0);
  } else {
    reply.WriteS32Little(-1);
  }

  client->SendPacket(reply);

  if (exchangeSession && !otherClient) {
    server->GetCharacterManager()->EndExchange(client, 0);
  }

  return true;
}
