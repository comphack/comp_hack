/**
 * @file server/channel/src/packets/game/EntrustAccept.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to accept an entrust request.
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
#include <ErrorCodes.h>
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

bool Parsers::EntrustAccept::Parse(
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

  auto otherClient = exchangeSession && exchangeSession->GetSourceEntityID() !=
                                            cState->GetEntityID()
                         ? server->GetManagerConnection()->GetEntityClient(
                               exchangeSession->GetSourceEntityID(), false)
                         : nullptr;

  EntrustErrorCodes_t responseCode = EntrustErrorCodes_t::SUCCESS;

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENTRUST_ACCEPT);

  if (!otherClient ||
      otherClient->GetClientState()->GetExchangeSession() != exchangeSession ||
      exchangeSession->GetOtherCharacterState() != cState) {
    responseCode = EntrustErrorCodes_t::SYSTEM_ERROR;
  } else if (exchangeSession->GetType() ==
             objects::PlayerExchangeSession::Type_t::CRYSTALLIZE) {
    auto demon = state->GetDemonState()->GetEntity();
    if (!demon) {
      responseCode = EntrustErrorCodes_t::INVALID_CHAR_STATE;
    } else if (characterManager->GetFamiliarityRank(demon->GetFamiliarity()) <
               3) {
      responseCode = EntrustErrorCodes_t::INVALID_DEMON_TARGET;
    } else {
      // Make sure the demon has not been reunioned
      for (int8_t reunionRank : demon->GetReunion()) {
        if (reunionRank) {
          responseCode = EntrustErrorCodes_t::INVALID_DEMON_TARGET;
          break;
        }
      }
    }
  }

  reply.WriteS32Little((int32_t)responseCode);

  client->QueuePacketCopy(reply);

  if (responseCode == EntrustErrorCodes_t::SUCCESS) {
    otherClient->SendPacketCopy(reply);

    characterManager->SetStatusIcon(client, 8);
    characterManager->SetStatusIcon(otherClient, 8);
  } else {
    characterManager->EndExchange(client);

    if (otherClient) {
      characterManager->EndExchange(otherClient);
    }
  }

  client->FlushOutgoing();

  return true;
}
