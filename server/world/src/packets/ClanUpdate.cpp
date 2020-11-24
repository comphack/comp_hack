/**
 * @file server/world/src/packets/ClanUpdate.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all clan focused actions between the world
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <Clan.h>
#include <ClanMember.h>

// world Includes
#include "CharacterManager.h"
#include "WorldServer.h"

using namespace world;

void ClanForm(std::shared_ptr<WorldServer> server,
              std::shared_ptr<libcomp::TcpConnection> requestConnection,
              std::shared_ptr<objects::CharacterLogin> cLogin,
              const libcomp::String clanName, uint32_t baseZoneID,
              int8_t activationID) {
  libcomp::Packet response;
  response.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
  response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
  response.WriteU16Little(1);  // CID Count
  response.WriteS32Little(cLogin->GetWorldCID());

  int32_t newClanID = 0;
  auto db = server->GetWorldDatabase();
  auto characterManager = server->GetCharacterManager();
  auto existing = objects::Clan::LoadClanByName(db, clanName);
  if (!existing) {
    // Reload the character so the clan can be set
    auto character =
        libcomp::PersistentObject::LoadObjectByUUID<objects::Character>(
            db, cLogin->GetCharacter().GetUUID(), true);

    // Make the clan and add the character
    auto clan = libcomp::PersistentObject::New<objects::Clan>(true);
    clan->SetName(clanName);
    clan->SetBaseZoneID(baseZoneID);

    auto clanMaster = libcomp::PersistentObject::New<objects::ClanMember>(true);
    clanMaster->SetClan(clan->GetUUID());
    clanMaster->SetMemberType(objects::ClanMember::MemberType_t::MASTER);
    clanMaster->SetCharacter(character->GetUUID());

    clan->AppendMembers(clanMaster);

    character->SetClan(clan);

    auto dbChanges = libcomp::DatabaseChangeSet::Create();
    dbChanges->Insert(clan);
    dbChanges->Insert(clanMaster);
    dbChanges->Update(character);

    if (db->ProcessChangeSet(dbChanges)) {
      auto clanInfo = characterManager->GetClan(clan->GetUUID());
      if (clanInfo) {
        newClanID = clanInfo->GetID();
        cLogin->SetClanID(newClanID);
        response.WriteS32Little(newClanID);
      }
    } else {
      character->SetClan(NULLUUID);
    }
  }

  if (newClanID == 0) {
    // Failure
    response.WriteS32Little(0);
  } else {
    characterManager->RecalculateClanLevel(newClanID, false);
  }

  response.WriteS8(activationID);

  requestConnection->SendPacket(response);

  if (newClanID != 0) {
    // Send the base clan info and let the client request the rest like normal
    std::list<int32_t> cids = {cLogin->GetWorldCID()};
    characterManager->SendClanInfo(newClanID, 0x0F, cids);

    characterManager->SendClanDetails(cLogin, requestConnection);
  }
}

void ClanInvite(std::shared_ptr<WorldServer> server,
                std::shared_ptr<libcomp::TcpConnection> requestConnection,
                int32_t clanID, std::shared_ptr<objects::CharacterLogin> cLogin,
                const libcomp::String targetName) {
  auto characterManager = server->GetCharacterManager();
  auto targetLogin = characterManager->GetCharacterLogin(targetName);
  auto clanInfo = characterManager->GetClan(clanID);

  const int8_t ERR_NO_RESPONSE = -2;
  const int8_t ERR_OFFLINE = -6;
  const int8_t ERR_IN_CLAN = -7;
  // const int8_t ERR_INVALID_MAP = -11;

  int8_t responseCode = ERR_NO_RESPONSE;
  if (clanInfo) {
    if (cLogin && targetLogin && targetLogin->GetChannelID() >= 0) {
      if (targetLogin->GetClanID() != 0) {
        responseCode = ERR_IN_CLAN;
      } else {
        auto channel =
            server->GetChannelConnectionByID(targetLogin->GetChannelID());
        if (channel) {
          libcomp::Packet relay;
          WorldServer::GetRelayPacket(relay, targetLogin->GetWorldCID());
          relay.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_CLAN_INVITED);
          relay.WriteS32Little(cLogin->GetWorldCID());
          relay.WriteString16Little(
              libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
              clanInfo->GetClan()->GetName(), true);
          relay.WriteString16Little(
              libcomp::Convert::Encoding_t::ENCODING_DEFAULT,
              cLogin->GetCharacter()->GetName(), true);
          relay.WriteS32Little(clanID);

          channel->SendPacket(relay);

          responseCode = 0;  // Success
        } else {
          responseCode = ERR_OFFLINE;
        }
      }
    } else {
      responseCode = ERR_OFFLINE;
    }
  }

  libcomp::Packet relay;
  WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
  relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_INVITE);
  relay.WriteS32Little(clanID);
  relay.WriteS8(responseCode);

  requestConnection->SendPacket(relay);
}

bool Parsers::ClanUpdate::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (p.Size() < 5) {
    LogClanErrorMsg("Invalid packet data sent to ClanUpdate\n");

    return false;
  }

  uint8_t mode = p.ReadU8();
  int32_t cid = p.ReadS32Little();

  auto server =
      std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
  auto characterManager = server->GetCharacterManager();
  auto cLogin = characterManager->GetCharacterLogin(cid);
  if (!cLogin) {
    LogClanError([&]() {
      return libcomp::String("Invalid world CID sent to ClanUpdate: %1\n")
          .Arg(cid);
    });
    return false;
  }

  switch ((InternalPacketAction_t)mode) {
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LIST: {
      uint8_t infoLevel = p.ReadU8();

      std::list<int32_t> memberIDs;
      if (infoLevel == 1) {
        // Requesting members, not the clan
        uint16_t cidCount = p.ReadU16Little();
        for (uint16_t i = 0; i < cidCount; i++) {
          memberIDs.push_back(p.ReadS32Little());
        }
      }
      characterManager->SendClanDetails(cLogin, connection, memberIDs);
    } break;
    case InternalPacketAction_t::PACKET_ACTION_ADD: {
      if (p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little()))) {
        LogClanErrorMsg(
            "Missing clan name parameter for clan formation command\n");

        return false;
      }

      libcomp::String clanName = p.ReadString16Little(
          libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

      if (p.Left() != 5) {
        LogClanErrorMsg(
            "Missing base zone ID or activation ID parameters for clan "
            "formation command\n");

        return false;
      }

      uint32_t baseZoneID = p.ReadU32Little();
      int8_t activationID = p.ReadS8();

      ClanForm(server, connection, cLogin, clanName, baseZoneID, activationID);
    } break;
    case InternalPacketAction_t::PACKET_ACTION_UPDATE: {
      if (p.Left() < 5) {
        LogClanErrorMsg(
            "Missing clan ID or update flag parameters for clan update "
            "command\n");

        return false;
      }

      int32_t clanID = p.ReadS32Little();
      uint8_t updateFlags = p.ReadU8();

      auto clanInfo = characterManager->GetClan(clanID);
      auto member = clanInfo
                        ? clanInfo->GetMemberMap(cLogin->GetWorldCID()).Get()
                        : nullptr;
      if (member) {
        auto worldDB = server->GetWorldDatabase();
        if (updateFlags &
            (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_MESSAGE) {
          libcomp::String message = p.ReadString16Little(
              libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

          member->SetClanMessage(message);
        }

        member->Update(worldDB);

        characterManager->SendClanMemberInfo(cLogin, updateFlags);
      }
    } break;
    case InternalPacketAction_t::PACKET_ACTION_YN_REQUEST:
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES: {
      int32_t clanID = p.ReadS32Little();

      if (p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little()))) {
        LogClanError([&]() {
          return libcomp::String(
                     "Missing target name parameter for command %1\n")
              .Arg(mode);
        });

        return false;
      }

      libcomp::String targetName = p.ReadString16Little(
          libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

      // Clan invite accept
      if (clanID == 0 && !targetName.IsEmpty()) {
        // Only the target name is known, get the clan ID
        auto targetLogin = characterManager->GetCharacterLogin(targetName);
        clanID = targetLogin ? targetLogin->GetClanID() : 0;
      }

      if (clanID) {
        if ((InternalPacketAction_t)mode ==
            InternalPacketAction_t::PACKET_ACTION_YN_REQUEST) {
          // Clan invite
          ClanInvite(server, connection, clanID, cLogin, targetName);
        } else {
          characterManager->ClanJoin(cLogin, clanID);
        }
      }

      return true;
    } break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE: {
      if (p.Left() < 4) {
        LogClanErrorMsg("Missing clan ID parameter for clan disband command\n");

        return false;
      }

      int32_t clanID = p.ReadS32Little();
      characterManager->ClanLeave(cLogin, clanID, connection);
    } break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_DISBAND: {
      if (p.Left() < 4) {
        LogClanErrorMsg("Missing clan ID parameter for clan disband command\n");

        return false;
      }

      int32_t clanID = p.ReadS32Little();
      characterManager->ClanDisband(clanID, cLogin->GetWorldCID(), connection);
    } break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE: {
      if (p.Left() < 8) {
        LogClanErrorMsg(
            "Missing leader CID parameter for clan leader update command\n");

        return false;
      }

      int32_t clanID = p.ReadS32Little();
      int32_t targetCID = p.ReadS32Little();

      if (p.Left() < 1) {
        LogClanErrorMsg(
            "Missing update type parameter for clan leader update command\n");

        return false;
      }

      uint8_t updateType = p.ReadU8();

      auto clanInfo = characterManager->GetClan(clanID);
      auto targetLogin = characterManager->GetCharacterLogin(targetCID);
      auto targetMember =
          targetLogin ? clanInfo->GetMemberMap(targetCID).Get() : nullptr;

      bool success = targetMember != nullptr;
      switch ((objects::ClanMember::MemberType_t)updateType) {
        case objects::ClanMember::MemberType_t::MASTER: {
          libcomp::Packet relay;
          WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
          relay.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_CLAN_MASTER_UPDATE);
          relay.WriteS32Little(clanInfo->GetID());
          relay.WriteS8(0);  // Response code doesn't seem to matter

          connection->SendPacket(relay);

          if (!success) {
            break;
          }

          auto sourceMember =
              clanInfo->GetMemberMap(cLogin->GetWorldCID()).Get();
          if (!sourceMember || sourceMember->GetMemberType() !=
                                   objects::ClanMember::MemberType_t::MASTER) {
            LogClanErrorMsg(
                "Non-master clan member attempted to reassign the clan master "
                "role\n");

            break;
          }

          auto worldDB = server->GetWorldDatabase();
          sourceMember->SetMemberType(
              objects::ClanMember::MemberType_t::SUB_MASTER);
          targetMember->SetMemberType(
              objects::ClanMember::MemberType_t::MASTER);
          sourceMember->Update(worldDB);
          targetMember->Update(worldDB);

          auto clanLogins = characterManager->GetRelatedCharacterLogins(
              targetLogin, RELATED_CLAN);
          clanLogins.push_back(targetLogin);

          relay.Clear();
          auto cidOffset = WorldServer::GetRelayPacket(relay);
          relay.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_CLAN_MASTER_UPDATED);
          relay.WriteS32Little(clanInfo->GetID());
          relay.WriteS32Little(targetCID);

          characterManager->SendToCharacters(relay, clanLogins, cidOffset);

          relay.Clear();
          cidOffset = WorldServer::GetRelayPacket(relay);
          relay.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_CLAN_SUB_MASTER_UPDATED);
          relay.WriteS32Little(clanInfo->GetID());
          relay.WriteS8((int8_t)objects::ClanMember::MemberType_t::SUB_MASTER);
          relay.WriteS32Little(cLogin->GetWorldCID());

          characterManager->SendToCharacters(relay, clanLogins, cidOffset);
        } break;
        case objects::ClanMember::MemberType_t::SUB_MASTER: {
          libcomp::Packet relay;
          WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
          relay.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_CLAN_SUB_MASTER_UPDATE);
          relay.WriteS32Little(clanInfo->GetID());
          relay.WriteS8(0);  // Response code doesn't seem to matter

          connection->SendPacket(relay);

          if (!success) {
            break;
          }

          auto sourceMember =
              clanInfo->GetMemberMap(cLogin->GetWorldCID()).Get();
          if (!sourceMember || sourceMember->GetMemberType() ==
                                   objects::ClanMember::MemberType_t::NORMAL) {
            LogClanErrorMsg(
                "Non-sub-master level clan member attempted to adjust a clan "
                "sub-master role\n");

            break;
          } else if (targetMember->GetMemberType() ==
                     objects::ClanMember::MemberType_t::MASTER) {
            LogClanErrorMsg(
                "Attempted to set the clan master to a sub-master\n");

            break;
          }

          auto worldDB = server->GetWorldDatabase();
          auto newRole = (targetMember->GetMemberType() ==
                          objects::ClanMember::MemberType_t::NORMAL)
                             ? objects::ClanMember::MemberType_t::SUB_MASTER
                             : objects::ClanMember::MemberType_t::NORMAL;
          targetMember->SetMemberType(newRole);
          targetMember->Update(worldDB);

          relay.Clear();
          auto cidOffset = WorldServer::GetRelayPacket(relay);
          relay.WritePacketCode(
              ChannelToClientPacketCode_t::PACKET_CLAN_SUB_MASTER_UPDATED);
          relay.WriteS32Little(clanInfo->GetID());
          relay.WriteS8((int8_t)newRole);
          relay.WriteS32Little(targetCID);

          characterManager->SendToRelatedCharacters(relay, targetCID, cidOffset,
                                                    RELATED_CLAN, true);
        } break;
        default:
          LogClanError([&]() {
            return libcomp::String(
                       "Invalid update type for clan leader update command "
                       "encountered: %1\n")
                .Arg(updateType);
          });

          return false;
      }
    } break;
    case InternalPacketAction_t::PACKET_ACTION_CLAN_EMBLEM_UPDATE: {
      if (p.Left() < 4) {
        LogClanErrorMsg(
            "Missing clan ID parameter for clan emblem update command\n");

        return false;
      }

      int32_t clanID = p.ReadS32Little();

      libcomp::Packet relay;
      WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
      relay.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_CLAN_EMBLEM_UPDATE);
      relay.WriteS32Little(clanID);
      relay.WriteS8(0);  // Response code doesn't seem to matter

      connection->SendPacket(relay);

      if (p.Left() < 8) {
        LogClanErrorMsg(
            "Missing emblem definition parameters for clan emblem update "
            "command\n");

        return false;
      }

      uint8_t base = p.ReadU8();
      uint8_t symbol = p.ReadU8();
      uint8_t r1 = p.ReadU8();
      uint8_t g1 = p.ReadU8();
      uint8_t b1 = p.ReadU8();
      uint8_t r2 = p.ReadU8();
      uint8_t g2 = p.ReadU8();
      uint8_t b2 = p.ReadU8();

      auto clanInfo = characterManager->GetClan(clanID);
      if (clanInfo) {
        auto worldDB = server->GetWorldDatabase();
        auto clan = clanInfo->GetClan().Get();
        clan->SetEmblemBase(base);
        clan->SetEmblemSymbol(symbol);
        clan->SetEmblemColorR1(r1);
        clan->SetEmblemColorG1(g1);
        clan->SetEmblemColorB1(b1);
        clan->SetEmblemColorR2(r2);
        clan->SetEmblemColorG2(g2);
        clan->SetEmblemColorB2(b2);

        clan->Update(worldDB);
        characterManager->SendClanInfo(clanID, 0x02);
      }
    } break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_KICK: {
      if (p.Left() < 8) {
        LogClanErrorMsg(
            "Missing clan ID or target CID parameter for clan kick command\n");

        return false;
      }

      int32_t clanID = p.ReadS32Little();
      int32_t targetCID = p.ReadS32Little();
      characterManager->ClanKick(cLogin, clanID, targetCID, connection);
    } break;
    default:
      break;
  }

  return true;
}
