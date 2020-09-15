/**
 * @file server/channel/src/packets/game/FriendData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update friend list related data.
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

// objects Includes
#include <AccountLogin.h>
#include <CharacterLogin.h>
#include <FriendSettings.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::FriendData::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() < 2) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto state = client->GetClientState();
  auto cLogin = state->GetAccountLogin()->GetCharacterLogin();
  auto character = cLogin->GetCharacter().Get();
  auto fSettings = character->GetFriendSettings().Get();

  int8_t updateFlags = p.ReadS8();

  if (updateFlags & 0x01) {
    if (p.Left() < 1) {
      return false;
    }

    int8_t status = p.ReadS8();
    cLogin->SetStatus((objects::CharacterLogin::Status_t)status);
  }

  if (updateFlags & 0x02) {
    if (p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little()))) {
      return false;
    }

    libcomp::String message = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_CP932, true);
    fSettings->SetFriendMessage(message);
  }

  if (updateFlags & 0x04) {
    if (p.Left() < 2) {
      return false;
    }

    // Updates to zone privacy only affect local so no need to send out
    int8_t privacySet = p.ReadS8();
    int8_t publicToZone = p.ReadS8();
    (void)privacySet;

    fSettings->SetPublicToZone(publicToZone == 1);
  }

  fSettings->Update(server->GetWorldDatabase());

  if (updateFlags != 0x04) {
    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
    request.WriteS32Little(cLogin->GetWorldCID());

    // Set empty flags for now
    request.WriteU8(0);

    uint8_t charLoginFlags = 0;
    if (updateFlags & 0x01) {
      request.WriteS8((int8_t)cLogin->GetStatus());
      charLoginFlags =
          (uint8_t)(charLoginFlags |
                    (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_STATUS);
    }

    if (updateFlags & 0x02) {
      charLoginFlags =
          (uint8_t)(charLoginFlags |
                    (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_MESSAGE);
    }

    // Rewind and set the flags
    request.Seek(6);
    request.WriteU8(charLoginFlags);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
  }

  return true;
}
