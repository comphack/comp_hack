/**
 * @file server/channel/src/packets/game/Warp.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to warp to a selected warp point.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <ActivatedAbility.h>
#include <CharacterProgress.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiConditionData.h>
#include <MiSkillBasicData.h>
#include <MiSkillData.h>
#include <MiWarpPointData.h>
#include <MiWarpRestriction.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "SkillManager.h"
#include "ZoneManager.h"

using namespace channel;

bool Parsers::Warp::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() != 9) {
    return false;
  }

  int32_t entityID = p.ReadS32Little();
  int8_t activationID = p.ReadS8();
  uint32_t warpPointID = p.ReadU32Little();

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto sourceState = state->GetEntityState(entityID);

  if (sourceState == nullptr) {
    LogGeneralError([&]() {
      return libcomp::String(
                 "Invalid entity ID received from a warp request: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    client->Close();
    return true;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto definitionManager = server->GetDefinitionManager();
  auto skillManager = server->GetSkillManager();
  auto zoneManager = server->GetZoneManager();
  auto characterManager = server->GetCharacterManager();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto inventory = character->GetItemBoxes(0).Get();
  auto progress = character->GetProgress().Get();

  auto activatedAbility = sourceState->GetSpecialActivations(activationID);
  if (!activatedAbility) {
    LogGeneralErrorMsg("Invalid activation ID encountered for Warp request\n");
  } else {
    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(
            state->GetObjectUUID(activatedAbility->GetActivationObjectID())));

    // Warp is valid if required conditions are met, and the item was consumed
    // or the skill wasn't an item skill
    auto warpDef = definitionManager->GetWarpPointData(warpPointID);
    bool warpConditionsMet = warpDef ? true : false;
    if (warpDef) {
      // Always 3 restrictions
      auto warpRestrictions = warpDef->GetWarpRestrictions();

      for (auto i = 0; i < warpDef->WarpRestrictionsCount(); ++i) {
        LogGeneralError([&]() {
          return libcomp::String(
                     "AllocateSkillPoint failed to process operational "
                     "changeset "
                     "when updating stats: %1\n")
              .Arg(state->GetAccountUID().ToString());
        });
        switch (warpRestrictions[i]->GetRestrictionType()) {
          case objects::MiWarpRestriction::RestrictionType_t::
              HAS_COMPLETED_QUEST: {
            size_t index;
            uint8_t shiftVal;
            characterManager->ConvertIDToMaskValues(
                (uint16_t)warpRestrictions[i]->GetRestrictionValue(), index,
                shiftVal);

            uint8_t indexVal = progress->GetCompletedQuests(index);

            if ((indexVal & shiftVal) == 0) {
              // Quest not completed
              warpConditionsMet = false;
            }
          } break;
          case objects::MiWarpRestriction::RestrictionType_t::
              HAS_ITEM_IN_INVENTORY: {
            if (!characterManager->GetExistingItemCount(
                    character,
                    (uint16_t)warpRestrictions[i]->GetRestrictionValue())) {
              warpConditionsMet = false;
            }
          } break;
          case objects::MiWarpRestriction::RestrictionType_t::HAS_VALUABLE: {
            if (!characterManager->HasValuable(
                    character,
                    (uint16_t)warpRestrictions[i]->GetRestrictionValue())) {
              warpConditionsMet = false;
            }
          } break;
          case objects::MiWarpRestriction::RestrictionType_t::NONE:
            break;
          default: {
            LogGeneralError([&]() {
              return libcomp::String(
                         "WarpPoint ID %1 has an invalid WarpRestriction type "
                         "that resolves to %2\n")
                  .Arg(warpDef->GetID())
                  .Arg(warpRestrictions[i]->GetRestrictionTypeValue());
            });

            warpConditionsMet = false;
          } break;
        }

        if (!warpConditionsMet) {
          break;
        }
      }
    }

    auto skillData = activatedAbility->GetSkillData();
    if (warpConditionsMet &&
        (item ||
         (skillData->GetBasic()->GetFamily() != SkillFamily_t::ITEM &&
          skillData->GetBasic()->GetFamily() != SkillFamily_t::DEMON_SOLO))) {
      uint32_t zoneID = warpDef->GetZoneID();

      float x = warpDef->GetX();
      float y = warpDef->GetY();
      float rot = warpDef->GetRotation();

      skillManager->ExecuteSkill(sourceState, activationID,
                                 activatedAbility->GetActivationObjectID());

      zoneManager->EnterZone(client, zoneID, 0, x, y, rot);
    } else {
      skillManager->CancelSkill(sourceState, activationID);
    }
  }

  return true;
}
