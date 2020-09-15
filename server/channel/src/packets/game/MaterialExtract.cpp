/**
 * @file server/channel/src/packets/game/MaterialExtract.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to extract materials from the
 *  tank to the inventory.
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
#include <DefinitionManager.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <Item.h>
#include <ItemBox.h>
#include <MiItemData.h>
#include <MiPossessionData.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::MaterialExtract::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 8) {
    return false;
  }

  uint32_t itemType = p.ReadU32Little();
  int32_t stackCount = p.ReadS32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  auto itemDef = definitionManager->GetItemData(itemType);

  // Ignore material tank valuable check (since there seems to only be the
  // inventory full error message). If they somehow have materials, no tank and
  // can send this packet, let them have it :P

  int32_t stacksAdded = 0;
  bool success = false;
  int32_t existingCount = (int32_t)character->GetMaterials(itemType);
  if (existingCount >= stackCount && itemDef) {
    auto inventory = character->GetItemBoxes(0).Get();
    int32_t maxStack = (int32_t)itemDef->GetPossession()->GetStackSize();

    // If a free slot exists in the inventory, any amount can be added
    int32_t addStacks = stackCount;
    bool canAdd = false;
    for (auto item : inventory->GetItems()) {
      if (item.IsNull()) {
        canAdd = true;
        break;
      }
    }

    // If a free slot does not exist in the inventory, check if existing
    // stacks can be added to
    if (!canAdd) {
      addStacks = 0;
      for (auto existing :
           characterManager->GetExistingItems(character, itemType, inventory)) {
        addStacks += (int32_t)(maxStack - existing->GetStackSize());
      }

      if (addStacks > stackCount) {
        addStacks = stackCount;
      }

      canAdd = addStacks > 0;
    }

    if (canAdd) {
      std::unordered_map<uint32_t, uint32_t> items;
      items[itemType] = (uint32_t)addStacks;
      if (characterManager->AddRemoveItems(client, items, true)) {
        character->SetMaterials(itemType,
                                (uint16_t)(existingCount - addStacks));
        success = true;
        stacksAdded = addStacks;

        server->GetWorldDatabase()->QueueUpdate(character,
                                                state->GetAccountUID());
      }
    }
  }

  libcomp::Packet reply;
  reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MATERIAL_EXTRACT);
  reply.WriteU32Little(itemType);
  reply.WriteS32Little(stackCount);
  reply.WriteS32Little(success ? 0 : 1);
  reply.WriteS32Little(stacksAdded);

  client->SendPacket(reply);

  if (success) {
    std::set<uint32_t> updates = {itemType};
    characterManager->SendMaterials(client, updates);
  }

  return true;
}
