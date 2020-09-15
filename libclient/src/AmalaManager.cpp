/**
 * @file libcomp/src/AmalaManager.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manages the custom amala network packets.
 *
 * This file is part of the COMP_hack Client Library (libclient).
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

#include "AmalaManager.h"

// libclient Includes
#include "LogicWorker.h"

// libcomp Includes
#include <Crypto.h>
#include <EnumUtils.h>
#include <Log.h>
#include <PacketCodes.h>

// packets Includes
#include <PacketChannelAmalaAccountDumpHeader.h>
#include <PacketChannelAmalaAccountDumpPart.h>

// logic messages
#include <MessageAccountDump.h>

// Standard C++ Includes
#include <fstream>

using namespace logic;

using libcomp::Message::MessageClientType;
using libcomp::Message::MessageType;

AmalaManager::AmalaManager(
    LogicWorker *pLogicWorker,
    const std::weak_ptr<libcomp::MessageQueue<libcomp::Message::Message *>>
        &messageQueue)
    : libcomp::Manager(),
      mLogicWorker(pLogicWorker),
      mMessageQueue(messageQueue),
      mAccountDumpSize(0),
      mAccountDumpPartCount(0) {
  (void)pLogicWorker;
}

AmalaManager::~AmalaManager() {}

std::list<libcomp::Message::MessageType> AmalaManager::GetSupportedTypes()
    const {
  return {
      MessageType::MESSAGE_TYPE_PACKET,
      MessageType::MESSAGE_TYPE_CLIENT,
  };
}

bool AmalaManager::ProcessMessage(const libcomp::Message::Message *pMessage) {
  switch (to_underlying(pMessage->GetType())) {
    case to_underlying(MessageType::MESSAGE_TYPE_PACKET):
      return ProcessPacketMessage((const libcomp::Message::Packet *)pMessage);
    case to_underlying(MessageType::MESSAGE_TYPE_CLIENT):
      return ProcessClientMessage(
          (const libcomp::Message::MessageClient *)pMessage);
    default:
      break;
  }

  return false;
}

bool AmalaManager::ProcessPacketMessage(
    const libcomp::Message::Packet *pMessage) {
  libcomp::ReadOnlyPacket p(pMessage->GetPacket());

  switch (pMessage->GetCommandCode()) {
    case to_underlying(
        ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_HEADER):
      return HandlePacketChannelAmalaAccountDumpHeader(p);
    case to_underlying(
        ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_PART):
      return HandlePacketChannelAmalaAccountDumpPart(p);
    default:
      break;
  }

  return false;
}

bool AmalaManager::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage) {
  switch (to_underlying(pMessage->GetMessageClientType())) {
    case to_underlying(MessageClientType::REQUEST_ACCOUNT_DUMP): {
      auto pAccountDump =
          reinterpret_cast<const MessageAccountDump *>(pMessage);
      mAccountDumpPath = pAccountDump->GetPath();

      // Request the character dump.
      mLogicWorker->SendBlankPacket(to_underlying(
          ClientToChannelPacketCode_t::PACKET_AMALA_REQ_ACCOUNT_DUMP));

      return true;
    }
    default:
      break;
  }

  return false;
}

bool AmalaManager::HandlePacketChannelAmalaAccountDumpHeader(
    libcomp::ReadOnlyPacket &p) {
  auto obj = std::make_shared<packets::PacketChannelAmalaAccountDumpHeader>();

  if (!obj->LoadPacket(p, false) || p.Left()) {
    return false;
  }

  mAccountDump.clear();

  mAccountDumpSHA1 = obj->GetSHA1();
  mAccountDumpName = obj->GetAccountName();
  mAccountDumpSize = obj->GetDumpSize();
  mAccountDumpPartCount = obj->GetPartCount();

  return true;
}

bool AmalaManager::HandlePacketChannelAmalaAccountDumpPart(
    libcomp::ReadOnlyPacket &p) {
  auto obj = std::make_shared<packets::PacketChannelAmalaAccountDumpPart>();

  if (!obj->LoadPacket(p, false) || p.Left()) {
    return false;
  }

  // Add the part data.
  auto partData = obj->GetPartData();
  mAccountDump.insert(mAccountDump.end(), partData.begin(), partData.end());

  if (mAccountDumpPartCount == obj->GetPartNumber()) {
    // Check the size and SHA1 of the dump.
    if (mAccountDumpSize == (uint32_t)mAccountDump.size() &&
        libcomp::Crypto::SHA1(mAccountDump) == mAccountDumpSHA1) {
      std::ofstream out;

      out.open(mAccountDumpPath.C(),
               std::ofstream::out | std::ofstream::binary);
      out.write(&mAccountDump[0], (std::streamsize)mAccountDump.size());

      mLogicWorker->SendToGame(new MessageAccountDumpStatus(out.good()));
    } else {
      mLogicWorker->SendToGame(new MessageAccountDumpStatus(false));
    }
  }

  return true;
}
