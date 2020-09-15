/**
 * @file server/channel/src/packets/amala/AccountDumpRequest.cpp
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Request from the client to dump the account information.
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
#include <Account.h>
#include <AccountLogin.h>
#include <Crypto.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"

using namespace channel;

#define PART_SIZE (1024)

void DumpAccount(AccountManager* accountManager,
                 const std::shared_ptr<ChannelClientConnection> client) {
  auto state = client->GetClientState();

  std::string dump = accountManager->DumpAccount(state).ToUtf8();

  // Send the account dump to the client.
  if (!dump.empty()) {
    std::vector<char> dumpData(dump.c_str(), dump.c_str() + dump.size());

    {
      auto accountName = state->GetAccountLogin()->GetAccount()->GetUsername();

      libcomp::Packet reply;
      reply.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_HEADER);
      reply.WriteU32Little((uint32_t)dump.size());
      reply.WriteU32Little(((uint32_t)dump.size() + PART_SIZE - 1) / PART_SIZE);
      reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                                libcomp::Crypto::SHA1(dumpData), true);
      reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                                accountName, true);

      client->SendPacket(reply);
    }

    const char* szNextPart = &dumpData[0];
    const char* szEnd = szNextPart + dump.size();

    uint32_t partNumber = 1;

    while (szNextPart < szEnd) {
      uint32_t partSize = (uint32_t)(szEnd - szNextPart);

      if (partSize > PART_SIZE) {
        partSize = PART_SIZE;
      }

      libcomp::Packet reply;
      reply.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_AMALA_ACCOUNT_DUMP_PART);
      reply.WriteU32Little(partNumber++);
      reply.WriteU32Little(partSize);
      reply.WriteArray(szNextPart, partSize);

      client->SendPacket(reply);

      szNextPart += partSize;
    }
  }
}

bool Parsers::AmalaAccountDumpRequest::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (0 != p.Size()) {
    return false;
  }

  auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
  auto server =
      std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());

  server->QueueWork(DumpAccount, server->GetAccountManager(), client);

  return true;
}
