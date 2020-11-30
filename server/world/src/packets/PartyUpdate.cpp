/**
 * @file server/world/src/packets/PartyUpdate.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all party focused actions between the world
 *  and the channels.
 *
 * This file is part of the World Server (world).
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
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// world Includes
#include "CharacterManager.h"
#include "WorldServer.h"

using namespace world;

void PartyInvite(std::shared_ptr<WorldServer> server,
                 std::shared_ptr<libcomp::TcpConnection> requestConnection,
                 std::shared_ptr<objects::PartyCharacter> member,
                 const libcomp::String targetName) {
  auto characterManager = server->GetCharacterManager();
  auto cLogin = characterManager->GetCharacterLogin(member->GetName());
  auto targetLogin = characterManager->GetCharacterLogin(targetName);

  uint16_t responseCode = (uint16_t)PartyErrorCodes_t::INVALID_OR_OFFLINE;
  if (cLogin && targetLogin && targetLogin->GetChannelID() >= 0) {
    auto party = cLogin->GetPartyID()
                     ? characterManager->GetParty(cLogin->GetPartyID())
                     : nullptr;
    if (party && party->GetLeaderCID() != cLogin->GetWorldCID()) {
      responseCode = (uint16_t)PartyErrorCodes_t::LEADER_REQUIRED;
    } else if (party && party->MemberIDsCount() >= MAX_PARTY_SIZE) {
      responseCode = (uint16_t)PartyErrorCodes_t::PARTY_FULL;
    } else if (targetLogin->GetPartyID() != 0) {
      responseCode = (uint16_t)PartyErrorCodes_t::IN_PARTY;
    } else {
      characterManager->AddToParty(member, 0);
      auto channel =
          server->GetChannelConnectionByID(targetLogin->GetChannelID());
      if (channel) {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, targetLogin->GetWorldCID());
        relay.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_PARTY_INVITED);
        relay.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_DEFAULT, member->GetName(),
            true);
        relay.WriteU32Little(cLogin->GetPartyID());

        channel->SendPacket(relay);

        responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
      }
    }
  }

  libcomp::Packet relay;
  WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
  relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_INVITE);
  relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
                            targetName, true);
  relay.WriteU16Little(responseCode);

  requestConnection->SendPacket(relay);
}

void PartyCancel(std::shared_ptr<WorldServer> server,
                 const libcomp::String sourceName,
                 const libcomp::String targetName, uint32_t partyID) {
  auto characterManager = server->GetCharacterManager();
  auto targetLogin = characterManager->GetCharacterLogin(targetName);

  if (targetLogin && targetLogin->GetChannelID() >= 0 &&
      partyID == targetLogin->GetPartyID()) {
    auto channel =
        server->GetChannelConnectionByID(targetLogin->GetChannelID());
    if (channel) {
      libcomp::Packet relay;
      WorldServer::GetRelayPacket(relay, targetLogin->GetWorldCID());
      relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_CANCEL);
      relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
                                sourceName, true);

      channel->SendPacket(relay);
    }
  }
}

void PartyDropRule(std::shared_ptr<WorldServer> server,
                   std::shared_ptr<libcomp::TcpConnection> requestConnection,
                   std::shared_ptr<objects::CharacterLogin> cLogin,
                   uint8_t rule) {
  auto characterManager = server->GetCharacterManager();
  auto party = characterManager->GetParty(cLogin->GetPartyID());

  uint16_t responseCode = (uint16_t)PartyErrorCodes_t::GENERIC_ERROR;
  if (party && party->SetDropRule((objects::Party::DropRule_t)rule)) {
    responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
  }

  libcomp::Packet relay;
  WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
  relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DROP_RULE);
  relay.WriteU16Little(responseCode);

  requestConnection->QueuePacket(relay);

  if (responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS) {
    server->GetCharacterManager()->SendPartyInfo(party->GetID());

    relay.Clear();
    auto cidOffset = WorldServer::GetRelayPacket(relay);
    relay.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_PARTY_DROP_RULE_SET);
    relay.WriteU8(rule);

    server->GetCharacterManager()->SendToRelatedCharacters(
        relay, cLogin->GetWorldCID(), cidOffset, RELATED_PARTY, true);
  }

  requestConnection->FlushOutgoing();
}

void PartyRecruitJoin(std::shared_ptr<WorldServer> server,
                      std::shared_ptr<libcomp::TcpConnection> requestConnection,
                      std::shared_ptr<objects::PartyCharacter> member,
                      const libcomp::String targetName) {
  auto characterManager = server->GetCharacterManager();
  auto cLogin = characterManager->GetCharacterLogin(member->GetName());
  auto targetLogin = characterManager->GetCharacterLogin(targetName);

  uint16_t responseCode = (uint16_t)PartyErrorCodes_t::INVALID_OR_OFFLINE;
  if (cLogin && targetLogin && targetLogin->GetChannelID() >= 0) {
    auto party = targetLogin->GetPartyID()
                     ? characterManager->GetParty(targetLogin->GetPartyID())
                     : nullptr;
    if (party && party->MemberIDsCount() >= MAX_PARTY_SIZE) {
      responseCode = (uint16_t)PartyErrorCodes_t::PARTY_FULL;
    } else if (cLogin->GetPartyID() != 0) {
      responseCode = (uint16_t)PartyErrorCodes_t::IN_PARTY;
    } else if (party && targetLogin->GetWorldCID() != party->GetLeaderCID()) {
      responseCode = (uint16_t)PartyErrorCodes_t::LEADER_REQUIRED;
    } else {
      characterManager->AddToParty(member, 0);
      auto channel =
          server->GetChannelConnectionByID(targetLogin->GetChannelID());
      if (channel) {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, targetLogin->GetWorldCID());
        relay.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_PARTY_RECRUIT_REPLIED);
        relay.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_DEFAULT, member->GetName(),
            true);
        relay.WriteU32Little(party ? party->GetID() : 0);

        channel->SendPacket(relay);

        responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
      }
    }
  }

  libcomp::Packet relay;
  WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
  relay.WritePacketCode(
      ChannelToClientPacketCode_t::PACKET_PARTY_RECRUIT_REPLY);
  relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
                            targetName, true);
  relay.WriteU16Little(responseCode);

  requestConnection->SendPacket(relay);
}

bool Parsers::PartyUpdate::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() < 5) {
    LogPartyErrorMsg("Invalid packet data sent to PartyUpdate\n");

    return false;
  }

  uint8_t mode = p.ReadU8();

  auto server =
      std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
  switch ((InternalPacketAction_t)mode) {
    case InternalPacketAction_t::PACKET_ACTION_YN_REQUEST:
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES: {
      bool request = (InternalPacketAction_t)mode ==
                     InternalPacketAction_t::PACKET_ACTION_YN_REQUEST;
      bool isRecruit = p.ReadU8() == 1;

      auto member = std::make_shared<objects::PartyCharacter>();
      if (!member->LoadPacket(p, false)) {
        LogPartyError([&]() {
          return libcomp::String(
                     "Party member data failed to load for command %1\n")
              .Arg(mode);
        });

        return false;
      }

      if (p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little()))) {
        LogPartyError([&]() {
          return libcomp::String(
                     "Missing target name parameter for command %1\n")
              .Arg(mode);
        });

        return false;
      }

      libcomp::String targetName = p.ReadString16Little(
          libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

      if (request) {
        if (!isRecruit) {
          PartyInvite(server, connection, member, targetName);
        } else {
          PartyRecruitJoin(server, connection, member, targetName);
        }
      } else {
        // Party invite accept
        if (p.Left() != 4) {
          LogPartyErrorMsg(
              "Missing party ID parameter for party invite accept command\n");

          return false;
        }

        uint32_t partyID = p.ReadU32Little();
        if (!isRecruit) {
          server->GetCharacterManager()->PartyJoin(member, targetName, partyID,
                                                   connection);
        } else {
          server->GetCharacterManager()->PartyRecruit(member, targetName,
                                                      partyID, connection);
        }
      }

      return true;
    } break;
    default:
      // Check for later party based requests
      break;
  }

  int32_t cid = p.ReadS32Little();

  auto cLogin = server->GetCharacterManager()->GetCharacterLogin(cid);
  if (!cLogin) {
    LogPartyError([&]() {
      return libcomp::String("Invalid world CID sent to PartyUpdate: %1\n")
          .Arg(cid);
    });

    return false;
  }

  switch ((InternalPacketAction_t)mode) {
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO: {
      if (p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little()))) {
        LogPartyErrorMsg(
            "Missing source name parameter for party invite cancel command\n");

        return false;
      }

      libcomp::String sourceName = p.ReadString16Little(
          libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

      if (p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little()))) {
        LogPartyErrorMsg(
            "Missing target name parameter for party invite cancel command\n");

        return false;
      }

      libcomp::String targetName = p.ReadString16Little(
          libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

      if (p.Left() != 4) {
        LogPartyError([&]() {
          return libcomp::String("Missing party ID parameter for command %1\n")
              .Arg(mode);
        });

        return false;
      }

      uint32_t partyID = p.ReadU32Little();
      PartyCancel(server, sourceName, targetName, partyID);
    } break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE:
      server->GetCharacterManager()->PartyLeave(cLogin, connection);
      break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_DISBAND:
      server->GetCharacterManager()->PartyDisband(
          cLogin->GetPartyID(), cLogin->GetWorldCID(), connection);
      break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE: {
      if (p.Left() != 4) {
        LogPartyErrorMsg(
            "Missing leader CID parameter for party leader update command\n");

        return false;
      }

      int32_t targetCID = p.ReadS32Little();
      server->GetCharacterManager()->PartyLeaderUpdate(
          cLogin->GetPartyID(), cLogin->GetWorldCID(), connection, targetCID);
    } break;
    case InternalPacketAction_t::PACKET_ACTION_PARTY_DROP_RULE: {
      if (p.Left() != 1) {
        LogPartyErrorMsg(
            "Missing rule parameter for party drop rule command\n");

        return false;
      }

      uint8_t rule = p.ReadU8();
      PartyDropRule(server, connection, cLogin, rule);
    } break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_KICK: {
      if (p.Left() != 4) {
        LogPartyErrorMsg(
            "Missing target CID parameter for party kick command\n");

        return false;
      }

      int32_t targetCID = p.ReadS32Little();
      server->GetCharacterManager()->PartyKick(cLogin, targetCID);
    } break;
    default:
      break;
  }

  return true;
}
