/**
 * @file server/channel/src/packets/game/SkillExecuteInstant.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to execute a skill instantaneously.
 *  If the skill supplied skill is not the correct type, it will
 *  activate normally.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Item.h>

// channel Includes
#include "ChannelServer.h"
#include "SkillManager.h"

using namespace channel;

bool Parsers::SkillExecuteInstant::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() < 20) {
    return false;
  }

  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto state = client->GetClientState();
  auto skillManager = server->GetSkillManager();

  int32_t sourceEntityID = p.ReadS32Little();
  uint32_t skillID = p.ReadU32Little();
  int32_t targetEntityID = p.ReadS32Little();

  uint32_t targetType = p.ReadU32Little();
  if (targetType != ACTIVATION_NOTARGET && p.Left() == 0) {
    LogSkillManagerError([&]() {
      return libcomp::String(
                 "Invalid skill target type sent from client for instant "
                 "execution request: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    return false;
  }

  auto source = state->GetEntityState(sourceEntityID, false);
  if (!source) {
    LogSkillManagerError([&]() {
      return libcomp::String(
                 "Invalid skill source sent from client for instant execution "
                 "request: %1\n")
          .Arg(state->GetAccountUID().ToString());
    });

    client->Close();
    return true;
  } else if (!source->Ready(true)) {
    // Entity is not currently active, send generic failure
    skillManager->SendFailure(source, skillID, client);
    return true;
  }

  int64_t targetObjectID = -1;
  switch (targetType) {
    case ACTIVATION_NOTARGET:
      // Nothing special to do
      break;
    case ACTIVATION_OBJECT: {
      targetObjectID = p.ReadS64Little();

      // Can be an item when not a use skill
      auto item = std::dynamic_pointer_cast<objects::Item>(
          libcomp::PersistentObject::GetObjectByUUID(
              state->GetObjectUUID(targetObjectID)));
      if (item && !skillManager->ValidateActivationItem(source, item)) {
        skillManager->SendFailure(source, skillID, client,
                                  (uint8_t)SkillErrorCodes_t::GENERIC);
        return true;
      }
    } break;
    case ACTIVATION_ITEM: {
      targetObjectID = p.ReadS64Little();

      auto item = std::dynamic_pointer_cast<objects::Item>(
          libcomp::PersistentObject::GetObjectByUUID(
              state->GetObjectUUID(targetObjectID)));
      if (!skillManager->ValidateActivationItem(source, item)) {
        skillManager->SendFailure(source, skillID, client,
                                  (uint8_t)SkillErrorCodes_t::ITEM_USE);
        return true;
      }
    } break;
    case ACTIVATION_TARGET:
      targetObjectID = (int64_t)p.ReadS32Little();
      break;
    default: {
      LogSkillManagerError([&]() {
        return libcomp::String(
                   "Unknown skill target type encountered for instant skill "
                   "execution request: %1\n")
            .Arg(targetType);
      });

      skillManager->SendFailure(source, skillID, client);
      return true;
    } break;
  }

  skillManager->ActivateSkill(source, skillID, targetObjectID,
                              (int64_t)targetEntityID, (uint8_t)targetType);

  return true;
}
