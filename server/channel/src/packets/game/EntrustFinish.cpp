/**
 * @file server/channel/src/packets/game/EntrustFinish.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client by either entrust player to end the
 *  exchange in its current state.
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
#include "Prefecture.h"

using namespace channel;

bool Parsers::EntrustFinish::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  (void)pPacketManager;

  if (p.Size() != 0) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto prefecture = state->GetPrefecture();
  auto server = prefecture->GetServer();
  auto characterManager = server->GetCharacterManager();
  auto cState = state->GetCharacterState();
  auto exchangeSession = state->GetExchangeSession();

  if (!exchangeSession) {
    // Nothing to do
    return true;
  }

  characterManager->EndExchange(client, -3);

  // Since either character can cancel it, check both entities
  auto otherCState = std::dynamic_pointer_cast<CharacterState>(
      exchangeSession->GetOtherCharacterState());
  if (otherCState != cState ||
      exchangeSession->GetSourceEntityID() != cState->GetEntityID()) {
    auto connectionManager = server->GetManagerConnection();
    auto otherClient = connectionManager->GetEntityClient(
        otherCState != cState ? otherCState->GetEntityID()
                              : exchangeSession->GetSourceEntityID(),
        false);
    if (otherClient) {
      characterManager->EndExchange(otherClient, -3);
    }
  }

  return true;
}
