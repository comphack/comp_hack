/**
 * @file server/channel/src/ChatManager.cpp
 * @ingroup channel
 *
 * @author HikaruM
 *
 * @brief Manages Chat Messages and GM Commands
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

#include "ChatManager.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <Git.h>
#include <Log.h>
#include <PacketCodes.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <ActivatedAbility.h>
#include <ChannelConfig.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <CharacterProgress.h>
#include <Clan.h>
#include <CultureData.h>
#include <Event.h>
#include <EventCounter.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Expertise.h>
#include <InstanceAccess.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiDevilData.h>
#include <MiDevilLVUpRateData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiNPCBasicData.h>
#include <MiQuestData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpotData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <Party.h>
#include <PostItem.h>
#include <PvPData.h>
#include <ReportedPlayer.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <Team.h>

// Standard C Includes
#include <cmath>
#include <cstdlib>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "ClientState.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "MatchManager.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

ChatManager::ChatManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server) {
  mGMands["addcp"] = &ChatManager::GMCommand_AddCP;
  mGMands["announce"] = &ChatManager::GMCommand_Announce;
  mGMands["ban"] = &ChatManager::GMCommand_Ban;
  mGMands["bethel"] = &ChatManager::GMCommand_Bethel;
  mGMands["bp"] = &ChatManager::GMCommand_BattlePoints;
  mGMands["coin"] = &ChatManager::GMCommand_Coin;
  mGMands["contract"] = &ChatManager::GMCommand_Contract;
  mGMands["counter"] = &ChatManager::GMCommand_Counter;
  mGMands["cowrie"] = &ChatManager::GMCommand_Cowrie;
  mGMands["crash"] = &ChatManager::GMCommand_Crash;
  mGMands["dxp"] = &ChatManager::GMCommand_DigitalizePoints;
  mGMands["effect"] = &ChatManager::GMCommand_Effect;
  mGMands["enchant"] = &ChatManager::GMCommand_Enchant;
  mGMands["enemy"] = &ChatManager::GMCommand_Enemy;
  mGMands["event"] = &ChatManager::GMCommand_Event;
  mGMands["expertise"] = &ChatManager::GMCommand_ExpertiseSet;
  mGMands["expertisemax"] = &ChatManager::GMCommand_ExpertiseExtend;
  mGMands["familiarity"] = &ChatManager::GMCommand_Familiarity;
  mGMands["flag"] = &ChatManager::GMCommand_Flag;
  mGMands["fgauge"] = &ChatManager::GMCommand_FusionGauge;
  mGMands["goto"] = &ChatManager::GMCommand_Goto;
  mGMands["gp"] = &ChatManager::GMCommand_GradePoints;
  mGMands["help"] = &ChatManager::GMCommand_Help;
  mGMands["homepoint"] = &ChatManager::GMCommand_Homepoint;
  mGMands["instance"] = &ChatManager::GMCommand_Instance;
  mGMands["item"] = &ChatManager::GMCommand_Item;
  mGMands["kick"] = &ChatManager::GMCommand_Kick;
  mGMands["levelup"] = &ChatManager::GMCommand_LevelUp;
  mGMands["license"] = &ChatManager::GMCommand_License;
  mGMands["lnc"] = &ChatManager::GMCommand_LNC;
  mGMands["map"] = &ChatManager::GMCommand_Map;
  mGMands["online"] = &ChatManager::GMCommand_Online;
  mGMands["penalty"] = &ChatManager::GMCommand_PenaltyReset;
  mGMands["plugin"] = &ChatManager::GMCommand_Plugin;
  mGMands["pos"] = &ChatManager::GMCommand_Position;
  mGMands["post"] = &ChatManager::GMCommand_Post;
  mGMands["quest"] = &ChatManager::GMCommand_Quest;
  mGMands["reported"] = &ChatManager::GMCommand_Reported;
  mGMands["resolve"] = &ChatManager::GMCommand_Resolve;
  mGMands["reunion"] = &ChatManager::GMCommand_Reunion;
  mGMands["scrap"] = &ChatManager::GMCommand_Scrap;
  mGMands["skill"] = &ChatManager::GMCommand_Skill;
  mGMands["skillpoint"] = &ChatManager::GMCommand_SkillPoint;
  mGMands["slotadd"] = &ChatManager::GMCommand_SlotAdd;
  mGMands["sp"] = &ChatManager::GMCommand_SoulPoints;
  mGMands["spawn"] = &ChatManager::GMCommand_Spawn;
  mGMands["speed"] = &ChatManager::GMCommand_Speed;
  mGMands["spirit"] = &ChatManager::GMCommand_Spirit;
  mGMands["support"] = &ChatManager::GMCommand_Support;
  mGMands["tickermessage"] = &ChatManager::GMCommand_TickerMessage;
  mGMands["title"] = &ChatManager::GMCommand_Title;
  mGMands["tokusei"] = &ChatManager::GMCommand_Tokusei;
  mGMands["valuable"] = &ChatManager::GMCommand_Valuable;
  mGMands["version"] = &ChatManager::GMCommand_Version;
  mGMands["worldtime"] = &ChatManager::GMCommand_WorldTime;
  mGMands["xp"] = &ChatManager::GMCommand_XP;
  mGMands["ziotite"] = &ChatManager::GMCommand_Ziotite;
  mGMands["zone"] = &ChatManager::GMCommand_Zone;
}

ChatManager::~ChatManager() {}

bool ChatManager::SendChatMessage(
    const std::shared_ptr<ChannelClientConnection>& client,
    ChatType_t chatChannel, const libcomp::String& message) {
  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();
  if (message.IsEmpty()) {
    return false;
  }

  auto state = client->GetClientState();
  auto zone = state->GetZone();
  if (!zone) {
    return false;
  }

  auto character = state->GetCharacterState()->GetEntity();
  libcomp::String sentFrom = character->GetName();

  ChatVis_t visibility = ChatVis_t::CHAT_VIS_SELF;

  uint32_t relayID = 0;
  PacketRelayMode_t relayMode = PacketRelayMode_t::RELAY_FAILURE;
  switch (chatChannel) {
    case ChatType_t::CHAT_PARTY:
      visibility = ChatVis_t::CHAT_VIS_PARTY;

      LogChatManagerInfo([&]() {
        return libcomp::String("[Party]:  %1: %2\n").Arg(sentFrom).Arg(message);
      });

      if (state->GetPartyID()) {
        relayID = state->GetPartyID();
        relayMode = PacketRelayMode_t::RELAY_PARTY;
      } else {
        LogChatManagerError([&]() {
          return libcomp::String(
                     "Party chat attempted by character not in a party: %1\n")
              .Arg(sentFrom);
        });

        return false;
      }
      break;
    case ChatType_t::CHAT_CLAN:
      visibility = ChatVis_t::CHAT_VIS_CLAN;

      LogChatManagerInfo([&]() {
        auto clan = character->GetClan().Get();
        auto name = clan ? clan->GetName() : libcomp::String("<unknown clan>");

        return libcomp::String("[Clan]:  %1: %2: %3\n")
            .Arg(name)
            .Arg(sentFrom)
            .Arg(message);
      });

      if (state->GetClanID()) {
        relayID = (uint32_t)state->GetClanID();
        relayMode = PacketRelayMode_t::RELAY_CLAN;
      } else {
        LogChatManagerError([&]() {
          return libcomp::String(
                     "Clan chat attempted by character not in a clan: %1\n")
              .Arg(sentFrom);
        });

        return false;
      }
      break;
    case ChatType_t::CHAT_TEAM:
      visibility = ChatVis_t::CHAT_VIS_TEAM;
      relayID = (uint32_t)state->GetTeamID();

      LogChatManagerInfo([&]() {
        return libcomp::String("[Team]:  %1: %2\n").Arg(sentFrom).Arg(message);
      });

      if (zone->GetInstanceType() == InstanceType_t::DIASPORA) {
        // Send to the whole zone instead
        visibility = ChatVis_t::CHAT_VIS_ZONE;
      } else if (state->GetTeamID()) {
        relayMode = PacketRelayMode_t::RELAY_TEAM;
      } else {
        LogChatManagerError([&]() {
          return libcomp::String(
                     "Team chat attempted by character not in a team: %1\n")
              .Arg(sentFrom);
        });

        return false;
      }
      break;
    case ChatType_t::CHAT_VERSUS:
      visibility = ChatVis_t::CHAT_VIS_VERSUS;

      LogChatManagerInfo([&]() {
        return libcomp::String("[Versus]:  %1: %2\n")
            .Arg(sentFrom)
            .Arg(message);
      });

      break;
    case ChatType_t::CHAT_SHOUT:
      visibility = ChatVis_t::CHAT_VIS_ZONE;

      LogChatManagerInfo([&]() {
        auto zoneMapID = zone->GetDynamicMapID();
        auto zoneID = zone->GetInstance()
                          ? libcomp::String("%1(%2)").Arg(zoneMapID).Arg(
                                zone->GetInstanceID())
                          : libcomp::String("%1").Arg(zoneMapID);

        return libcomp::String("[Shout]:  %1: %2: %3\n")
            .Arg(zoneID)
            .Arg(sentFrom)
            .Arg(message);
      });

      break;
    case ChatType_t::CHAT_SAY:
      visibility = ChatVis_t::CHAT_VIS_RANGE;

      LogChatManagerInfo([&]() {
        auto zoneMapID = zone->GetDynamicMapID();
        auto zoneID = zone->GetInstance()
                          ? libcomp::String("%1(%2)").Arg(zoneMapID).Arg(
                                zone->GetInstanceID())
                          : libcomp::String("%1").Arg(zoneMapID);

        return libcomp::String("[Say]:  %1: %2: %3\n")
            .Arg(zoneID)
            .Arg(sentFrom)
            .Arg(message);
      });

      break;
    case ChatType_t::CHAT_SELF:
      visibility = ChatVis_t::CHAT_VIS_SELF;

      LogChatManagerInfo([&]() {
        return libcomp::String("[Self]:  %1: %2\n").Arg(sentFrom).Arg(message);
      });

      break;
    default:
      return false;
  }

  bool relay = relayMode != PacketRelayMode_t::RELAY_FAILURE;

  libcomp::Packet p;
  if (relay) {
    // Route the message through the world via packet relay
    p.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
    p.WriteS32Little(state->GetWorldCID());
    p.WriteU8((uint8_t)relayMode);
    p.WriteU32Little(relayID);
    p.WriteU8(1);  // Include self
  }

  if (chatChannel == ChatType_t::CHAT_CLAN) {
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_CHAT);
    p.WriteS32Little((int32_t)relayID);
    p.WriteString16Little(state->GetClientStringEncoding(), sentFrom, true);
    p.WriteString16Little(state->GetClientStringEncoding(), message, true);
  } else if (chatChannel == ChatType_t::CHAT_TEAM) {
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_CHAT);
    p.WriteS32Little((int32_t)relayID);
    p.WriteString16Little(state->GetClientStringEncoding(), sentFrom, true);
    p.WriteString16Little(state->GetClientStringEncoding(), message, true);
  } else {
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
    p.WriteU16Little((uint16_t)chatChannel);
    p.WriteString16Little(state->GetClientStringEncoding(), sentFrom, true);
    p.WriteString16Little(state->GetClientStringEncoding(), message, true);
  }

  switch (visibility) {
    case ChatVis_t::CHAT_VIS_SELF:
      client->SendPacket(p);
      break;
    case ChatVis_t::CHAT_VIS_ZONE:
      zoneManager->BroadcastPacket(client, p, true);
      break;
    case ChatVis_t::CHAT_VIS_RANGE:
      zoneManager->SendToRange(client, p, true);
      break;
    case ChatVis_t::CHAT_VIS_VERSUS:
      // Get all characters in the instance in the same faction
      {
        std::list<std::shared_ptr<ChannelClientConnection>> vsTeam;
        auto instance = zone ? zone->GetInstance() : nullptr;
        if (instance) {
          auto cState = state->GetCharacterState();
          for (auto c : instance->GetConnections()) {
            auto cState2 = c->GetClientState()->GetCharacterState();
            if (cState->SameFaction(cState2)) {
              vsTeam.push_back(c);
            }
          }
        } else {
          vsTeam.push_back(client);
        }

        ChannelClientConnection::BroadcastPacket(vsTeam, p);
      }
      break;
    case ChatVis_t::CHAT_VIS_PARTY:
    case ChatVis_t::CHAT_VIS_CLAN:
    case ChatVis_t::CHAT_VIS_TEAM:
      mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(
          p);
      break;
    default:
      return false;
  }

  return true;
}

bool ChatManager::SendTellMessage(
    const std::shared_ptr<ChannelClientConnection>& client,
    const libcomp::String& message, const libcomp::String& targetName) {
  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  LogChatManagerInfo([character, targetName, message]() {
    return libcomp::String("[Tell]:  %1 >> %2: %3\n")
        .Arg(character->GetName())
        .Arg(targetName)
        .Arg(message);
  });

  // Relay a packet by target name
  libcomp::Packet relay;
  relay.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
  relay.WriteS32Little(state->GetWorldCID());
  relay.WriteU8((uint8_t)PacketRelayMode_t::RELAY_CHARACTER);
  relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                            targetName, true);

  // Write the normal packet to relay
  relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
  relay.WriteU16Little((uint16_t)ChatType_t::CHAT_TELL);

  // Clients should be using the same encoding
  relay.WriteString16Little(state->GetClientStringEncoding(),
                            character->GetName(), true);
  relay.WriteString16Little(state->GetClientStringEncoding(), message, true);

  mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(
      relay);

  return true;
}

bool ChatManager::SendTeamChatMessage(
    const std::shared_ptr<ChannelClientConnection>& client,
    const libcomp::String& message, int32_t teamID) {
  auto state = client->GetClientState();
  if (state->GetTeamID() == teamID) {
    return SendChatMessage(client, ChatType_t::CHAT_TEAM, message);
  }

  return false;
}

bool ChatManager::HandleGMand(
    const std::shared_ptr<ChannelClientConnection>& client,
    const libcomp::String& message) {
  auto state = client->GetClientState();

  std::smatch match;
  std::string input = message.C();
  static const std::regex toFind("@([^\\s]+)(.*)");
  if (std::regex_match(input, match, toFind)) {
    if (state->GetUserLevel() == 0 && "@version" != message &&
        "@license" != message) {
      // Don't process the message but don't fail
      LogChatManagerDebug([&]() {
        return libcomp::String(
                   "Non-GM account attempted to execute a GM command: %1\n")
            .Arg(state->GetAccountUID().ToString());
      });
      return true;
    }

    libcomp::String sentFrom =
        state->GetCharacterState()->GetEntity()->GetName();

    LogChatManagerInfo([&]() {
      return libcomp::String("[GM] %1: %2\n").Arg(sentFrom).Arg(message);
    });

    libcomp::String command(match[1]);
    libcomp::String args(match.max_size() > 2 ? match[2].str() : "");

    command = command.ToLower();

    std::list<libcomp::String> argsList;
    if (!args.IsEmpty()) {
      argsList = args.Split(" ");
    }
    argsList.remove_if(
        [](const libcomp::String& value) { return value.IsEmpty(); });

    mServer.lock()->QueueWork(
        [](ChatManager* pChatManager,
           const std::shared_ptr<ChannelClientConnection>& cmdClient,
           const libcomp::String& cmd,
           const std::list<libcomp::String>& cmdArgs) {
          if (!pChatManager->ExecuteGMCommand(cmdClient, cmd, cmdArgs)) {
            LogChatManagerWarning([&]() {
              return libcomp::String("GM command could not be processed: %1\n")
                  .Arg(cmd);
            });
          }
        },
        this, client, command, argsList);

    return true;
  }

  return false;
}

bool ChatManager::ExecuteGMCommand(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const libcomp::String& cmd, const std::list<libcomp::String>& args) {
  auto it = mGMands.find(cmd);

  if (it != mGMands.end()) {
    return it->second(*this, client, args);
  }

  LogChatManagerWarning([&]() {
    return libcomp::String("Unknown GM command encountered: %1\n").Arg(cmd);
  });

  return false;
}

bool ChatManager::GMCommand_AddCP(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ADD_CP)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto targetClient = client;
  auto targetAccount =
      client->GetClientState()->GetAccountLogin()->GetAccount().Get();
  auto server = mServer.lock();

  int64_t amount = 0;
  if (!GetIntegerArg<int64_t>(amount, argsCopy) || amount <= 0) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("@addcp requires a positive amount be specified"));
  }

  libcomp::String name;
  if (GetStringArg(name, argsCopy)) {
    std::shared_ptr<objects::Character> targetCharacter;
    if (!GetTargetCharacterAccount(name, false, targetCharacter, targetAccount,
                                   targetClient)) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String(
              "Invalid character name supplied for add CP command: %1")
              .Arg(name));
    }
  }

  auto accountManager = server->GetAccountManager();
  if (accountManager->IncreaseCP(targetAccount, amount)) {
    if (targetClient) {
      if (targetClient != client) {
        // Notify the other player
        auto character =
            client->GetClientState()->GetCharacterState()->GetEntity();
        SendChatMessage(targetClient, ChatType_t::CHAT_SELF,
                        libcomp::String("You have received %1 CP from %2!\n")
                            .Arg(amount)
                            .Arg(character ? character->GetName() : "a GM"));
      } else {
        SendChatMessage(
            targetClient, ChatType_t::CHAT_SELF,
            libcomp::String("Your available CP has increased by %1!\n")
                .Arg(amount));
      }

      accountManager->SendCPBalance(targetClient);
    }
  } else {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Failed to add CP to target character account: %1\n")
            .Arg(name));
  }

  return true;
}

bool ChatManager::GMCommand_Announce(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ANNOUNCE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;
  int8_t color = 0;

  if (!GetIntegerArg<int8_t>(color, argsCopy) || argsCopy.size() < 1) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("@announce requires two arguments, <color> <message>"));
  }

  libcomp::String message = libcomp::String::Join(argsCopy, " ");
  auto server = mServer.lock();
  server->SendSystemMessage(client, message, color, true);

  // Print to the console too
  libcomp::Packet p;
  p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
  p.WriteU16Little((uint16_t)ChatType_t::CHAT_SELF);
  p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT, "",
                        true);  // Not used
  p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_DEFAULT, message,
                        true);

  server->GetManagerConnection()->BroadcastPacketToClients(p);

  return true;
}

bool ChatManager::GMCommand_Ban(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_BAN)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;
  libcomp::String bannedPlayer;

  if (!GetStringArg(bannedPlayer, argsCopy)) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("@ban requires at least one argument, <username>"));
  }

  int8_t kickLevel = 1;
  int8_t kickLevelArg = -1;

  GetIntegerArg(kickLevel, argsCopy);

  std::shared_ptr<objects::Character> target;
  std::shared_ptr<objects::Account> targetAccount;
  std::shared_ptr<channel::ChannelClientConnection> targetClient;
  if (!GetTargetCharacterAccount(bannedPlayer, false, target, targetAccount,
                                 targetClient) ||
      (targetAccount && !targetClient)) {
    kickLevel = kickLevelArg;
    if (kickLevelArg <= 0 || kickLevel > 3) {
      kickLevel = 1;
    }

    SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String(
            "Target character is not currently on the channel to kick. Banning "
            "and sending level %1 disconnect request to the world.")
            .Arg(kickLevel));
  }

  if (!targetAccount) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Supplied character name is invalid.");
  } else if (client->GetClientState()->GetUserLevel() <
             targetAccount->GetUserLevel()) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "Your user level is lower than the user you tried to ban.");
  }

  libcomp::String reason = libcomp::String::Join(argsCopy, " ");

  auto server = mServer.lock();
  targetAccount->SetEnabled(false);
  targetAccount->SetBanReason(reason);
  targetAccount->SetBanInitiator(
      client->GetClientState()->GetAccountLogin()->GetAccount()->GetUsername());
  targetAccount->Update(server->GetLobbyDatabase());

  if (targetClient) {
    targetClient->Close();
  } else {
    libcomp::Packet p;
    p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
    p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
    p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                          targetAccount->GetUsername());
    p.WriteS8(kickLevel);

    server->GetManagerConnection()->GetWorldConnection()->SendPacket(p);
  }

  // Now clean up rentals for all characters on this server which will
  // all be loaded already if part of this channel
  auto characters = objects::Character::LoadCharacterListByAccount(
      server->GetWorldDatabase(), targetAccount->GetUUID());
  for (auto c : characters) {
    auto cData = c->GetCultureData().Get();
    if (cData && cData->GetActive()) {
      server->GetCharacterManager()->CultureExpire(cData, true);

      SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Culture data expired for %1.").Arg(c->GetName()));
    }
  }

  auto bazaarData = objects::BazaarData::LoadBazaarDataByAccount(
      server->GetWorldDatabase(), targetAccount->GetUUID());
  if (bazaarData &&
      bazaarData->GetState() == objects::BazaarData::State_t::BAZAAR_ACTIVE) {
    auto zoneData =
        server->GetServerDataManager()->GetZoneData(bazaarData->GetZone(), 0);
    auto zone = zoneData ? server->GetZoneManager()->GetGlobalZone(
                               zoneData->GetID(), zoneData->GetDynamicMapID())
                         : nullptr;
    std::shared_ptr<BazaarState> bState;
    if (zone) {
      for (auto b : zone->GetBazaars()) {
        if (bazaarData == b->GetCurrentMarket(bazaarData->GetMarketID())) {
          bState = b;
          break;
        }
      }
    }

    if (bState) {
      bazaarData->SetState(objects::BazaarData::State_t::BAZAAR_INACTIVE);
      bState->SetCurrentMarket(bazaarData->GetMarketID(), nullptr);

      SendChatMessage(client, ChatType_t::CHAT_SELF,
                      libcomp::String("Bazaar expired in zone %1.")
                          .Arg(zone->GetDefinitionID()));

      server->GetZoneManager()->SendBazaarMarketData(zone, bState,
                                                     bazaarData->GetMarketID());

      server->GetWorldDatabase()->QueueUpdate(bazaarData,
                                              targetAccount->GetUUID());
    } else {
      SendChatMessage(client, ChatType_t::CHAT_SELF,
                      libcomp::String("Bazaar in zone %1 cannot be expired.")
                          .Arg(zone->GetID()));
    }
  }

  return true;
}

bool ChatManager::GMCommand_BattlePoints(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_BATTLE_POINTS)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int32_t points;

  if (!GetIntegerArg(points, argsCopy) || points < 0) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("@bp requires a positive amount be specified"));
  }

  auto server = mServer.lock();

  auto pvpData = server->GetMatchManager()->GetPvPData(client);
  if (!pvpData) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "BP could not be updated");
  }

  // Hack to bump both to the same value
  pvpData->SetBP(0);
  pvpData->SetBPTotal(0);

  server->GetCharacterManager()->UpdateBP(client, (int32_t)points, true);

  return true;
}

bool ChatManager::GMCommand_Bethel(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_BETHEL)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint8_t idx;
  int32_t points;

  if (!GetIntegerArg(idx, argsCopy) || !GetIntegerArg(points, argsCopy) ||
      idx >= 5 || points < 0) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("@bethel requires a valid type index and a positive "
                        "amount to be specified"));
  }

  auto state = client->GetClientState();
  auto character = state->GetCharacterState()->GetEntity();
  auto progress = character ? character->GetProgress().Get() : nullptr;
  if (progress) {
    auto server = mServer.lock();

    progress->SetBethel(idx, points);

    server->GetCharacterManager()->SendCowrieBethel(client);

    server->GetWorldDatabase()->QueueUpdate(progress);

    return true;
  }

  return false;
}

bool ChatManager::GMCommand_Coin(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_COIN)) {
    return true;
  }

  auto state = client->GetClientState();
  auto eState = state->GetEventState();
  if (eState && eState->GetGameSession()) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "Coins cannot be added while web-game session is active");
  }

  std::list<libcomp::String> argsCopy = args;

  int64_t amount;

  if (!GetIntegerArg(amount, argsCopy)) {
    return false;
  }

  if (!mServer.lock()->GetCharacterManager()->UpdateCoinTotal(client, amount,
                                                              false)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Coin total could not be updated");
  }

  return true;
}

bool ChatManager::GMCommand_Contract(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_CONTRACT)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto characterManager = server->GetCharacterManager();
  auto definitionManager = server->GetDefinitionManager();

  uint32_t demonID;

  if (!GetIntegerArg<uint32_t>(demonID, argsCopy)) {
    libcomp::String name;
    if (!GetStringArg(name, argsCopy)) {
      return false;
    }

    auto devilData = definitionManager->GetDevilData(name);
    if (devilData == nullptr) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Invalid demon name: %1").Arg(name));
    }
    demonID = devilData->GetBasic()->GetID();
  }

  auto devilData = definitionManager->GetDevilData(demonID);
  if (devilData == nullptr) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Invalid demon ID: %1").Arg(demonID));
  }

  auto demon =
      characterManager->ContractDemon(client, devilData, 0, MAX_FAMILIARITY);
  return demon != nullptr || SendChatMessage(client, ChatType_t::CHAT_SELF,
                                             "Demon could not be contracted");
}

bool ChatManager::GMCommand_Counter(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_COUNTER)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int32_t type = 0;
  if (!GetIntegerArg(type, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@counter requires a type ID");
  }

  if (argsCopy.size() > 0) {
    // View/edit character flag
    auto targetClient = client;
    auto targetAccount =
        client->GetClientState()->GetAccountLogin()->GetAccount().Get();

    libcomp::String name;
    if (GetStringArg(name, argsCopy)) {
      std::shared_ptr<objects::Character> targetCharacter;
      if (!GetTargetCharacterAccount(name, false, targetCharacter,
                                     targetAccount, targetClient)) {
        return SendChatMessage(
            client, ChatType_t::CHAT_SELF,
            libcomp::String(
                "Invalid character name supplied for counter command: %1")
                .Arg(name));
      }
    }

    if (argsCopy.size() > 0) {
      // Edit
      int32_t add = 0;
      if (!GetIntegerArg(add, argsCopy)) {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
                               "@counter requires a value to add");
      }

      mServer.lock()->GetCharacterManager()->UpdateEventCounter(targetClient,
                                                                type, add);
    } else {
      // View only
      auto state = targetClient->GetClientState();
      auto eCounter = state->GetEventCounters(type).Get();

      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Player: %1; Event type: %2; Counter: %3")
              .Arg(name)
              .Arg(type)
              .Arg(eCounter ? eCounter->GetCounter() : 0));
    }
  } else {
    // View world flag
    auto eCounter =
        mServer.lock()->GetChannelSyncManager()->GetWorldEventCounter(type);

    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           libcomp::String("Event type: %1; Counter: %2")
                               .Arg(type)
                               .Arg(eCounter ? eCounter->GetCounter() : 0));
  }

  return true;
}

bool ChatManager::GMCommand_Cowrie(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_COWRIE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int32_t points;
  if (!GetIntegerArg(points, argsCopy) || points < 0) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String(
            "@cowrie requires that a positive amount be specified"));
  }

  auto state = client->GetClientState();
  auto character = state->GetCharacterState()->GetEntity();
  auto progress = character ? character->GetProgress().Get() : nullptr;
  if (progress) {
    auto server = mServer.lock();

    progress->SetCowrie(points);

    server->GetCharacterManager()->SendCowrieBethel(client);

    server->GetWorldDatabase()->QueueUpdate(progress);

    return true;
  }

  return false;
}

bool ChatManager::GMCommand_Crash(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_CRASH)) {
    return true;
  }

  (void)client;
  (void)args;

  abort();
}

bool ChatManager::GMCommand_DigitalizePoints(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_DIGITALIZE_POINTS)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint8_t raceID;
  int32_t points;
  if (!GetIntegerArg(raceID, argsCopy) || !GetIntegerArg(points, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@dxp requires a demon race ID and digitalize XP");
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  if (!definitionManager->GetGuardianLevelData(raceID)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Invalid digitalize demon race ID");
  }

  if (points < 0) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Invalid digitalize XP supplied");
  }

  std::unordered_map<uint8_t, int32_t> pointMap;
  pointMap[raceID] = points;

  server->GetCharacterManager()->UpdateDigitalizePoints(client, pointMap, false,
                                                        false);

  return true;
}

bool ChatManager::GMCommand_Effect(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_EFFECT)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint32_t effectID;

  if (!GetIntegerArg<uint32_t>(effectID, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@effect requires an effect ID");
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto def = definitionManager->GetStatusData(effectID);

  if (!def) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Invalid effect ID supplied: %1").Arg(effectID));
  }

  // If the next arg starts with a '+' or '-', mark as an add instead of replace
  bool isAdd = false;

  if (argsCopy.size() > 0) {
    libcomp::String& next = argsCopy.front();
    if (!next.IsEmpty() && (next.C()[0] == '+' || next.C()[0] == '-')) {
      isAdd = true;
    }
  }

  int8_t stack;

  if (!GetIntegerArg<int8_t>(stack, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@effect requires a stack count");
  }

  auto state = client->GetClientState();

  libcomp::String target;
  bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
  auto eState =
      isDemon
          ? std::dynamic_pointer_cast<ActiveEntityState>(state->GetDemonState())
          : std::dynamic_pointer_cast<ActiveEntityState>(
                state->GetCharacterState());

  StatusEffectChanges effects;
  effects[effectID] = StatusEffectChange(effectID, stack, !isAdd);
  eState->AddStatusEffects(effects, definitionManager);

  server->GetCharacterManager()->RecalculateTokuseiAndStats(eState, client);

  return true;
}

bool ChatManager::GMCommand_Enchant(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ENCHANT)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  // All but BULLET types are (technically) valid
  const std::set<int8_t> validTypes = {
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_HEAD,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FACE,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NECK,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_ARMS,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FEET,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_COMP,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EARRING,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EXTRA,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BACK,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TALISMAN,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON,
  };

  int8_t equipType = GetEquipTypeArg(client, argsCopy, "enchant", validTypes);
  if (equipType == -1) {
    return true;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto item = character ? character->GetEquippedItems((size_t)equipType).Get()
                        : nullptr;

  if (!item) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "The specified item type is not equipped");
  }

  int16_t tarot = 0;
  int16_t soul = 0;

  if (!GetIntegerArg(tarot, argsCopy) || !GetIntegerArg(soul, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Missing tarot and soul effects");
  }

  // Default to current effect if not specified
  if (tarot == -1) {
    tarot = item->GetTarot();
  }

  if (soul == -1) {
    soul = item->GetSoul();
  }

  // No restrictions enforced here except validity checks
  if ((tarot && !definitionManager->GetEnchantData(tarot)) ||
      (soul && !definitionManager->GetEnchantData(soul))) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Invalid tarot or soul effect specified");
  }

  item->SetTarot(tarot);
  item->SetSoul(soul);

  auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
      libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
  if (itemBox) {
    server->GetCharacterManager()->SendItemBoxData(
        client, itemBox, {(uint16_t)item->GetBoxSlot()});
  }

  cState->RecalcEquipState(definitionManager);

  server->GetCharacterManager()->RecalculateTokuseiAndStats(cState, client);

  server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());

  return true;
}

bool ChatManager::GMCommand_Enemy(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ENEMY)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  // Valid params: enemy, enemy+AI, enemy+AI+x+y, enemy+AI+x+y+rot,
  // enemy+x+y, enemy+x+y+rot
  if (argsCopy.empty() || argsCopy.size() > 5) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@enemy requires one to five args");
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();
  auto zoneManager = server->GetZoneManager();

  uint32_t demonID;

  if (!GetIntegerArg<uint32_t>(demonID, argsCopy)) {
    libcomp::String name;

    if (!GetStringArg(name, argsCopy)) {
      return false;
    }

    auto devilData = definitionManager->GetDevilData(name);

    if (devilData == nullptr) {
      return false;
    }

    demonID = devilData->GetBasic()->GetID();
  }

  cState->RefreshCurrentPosition(ChannelServer::GetServerTime());

  float x = cState->GetCurrentX();
  float y = cState->GetCurrentY();
  float rot = cState->GetCurrentRotation();

  // All optional params past this point
  libcomp::String aiType = "";

  if (!argsCopy.empty()) {
    // Check for a number for X first
    bool xParam = GetDecimalArg<float>(x, argsCopy);

    if (!xParam) {
      // Assume a non-number is an AI script type
      GetStringArg(aiType, argsCopy);
      xParam = GetDecimalArg<float>(x, argsCopy);
    }

    // X/Y optional but Y must be set if X is
    if (xParam) {
      if (!GetDecimalArg<float>(y, argsCopy)) {
        return false;
      }

      // Rotation is optional
      if (!GetDecimalArg<float>(rot, argsCopy)) {
        rot = 0.f;
      }
    } else {
      x = cState->GetCurrentX();
    }
  }

  auto def = definitionManager->GetDevilData(demonID);
  auto zone = zoneManager->GetCurrentZone(client);

  if (nullptr == def) {
    return false;
  }

  return zoneManager->SpawnEnemy(zone, demonID, x, y, rot, aiType);
}

bool ChatManager::GMCommand_Event(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_EVENT)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  libcomp::String eventID;

  if (argsCopy.size() > 0 && !GetStringArg(eventID, argsCopy)) {
    return false;
  }

  if (eventID.IsEmpty()) {
    auto state = client->GetClientState();
    auto eState = state->GetEventState();
    auto current = eState->GetCurrent();
    auto event = current ? current->GetEvent() : nullptr;

    if (event) {
      eventID = event->GetID();
    }

    if (eventID.IsEmpty()) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             libcomp::String("No event is currently active"));
    } else {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("The active event is: %1").Arg(eventID));
    }
  } else {
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();
    auto event = serverDataManager->GetEventData(eventID);

    if (event) {
      auto state = client->GetClientState();
      auto cState = state->GetCharacterState();
      auto zone = state->GetZone();

      EventOptions options;
      if (argsCopy.size() > 0) {
        // Copy in additional arguments as strings to preserve spaces
        libcomp::String param;
        while (argsCopy.size() > 0 && GetStringArg(param, argsCopy)) {
          options.TransformScriptParams.push_back(param);
        }
      }

      if (server->GetEventManager()->HandleEvent(
              client, eventID, cState->GetEntityID(), zone, options)) {
        return SendChatMessage(
            client, ChatType_t::CHAT_SELF,
            libcomp::String("Event started: %1").Arg(eventID));
      } else {
        return SendChatMessage(
            client, ChatType_t::CHAT_SELF,
            libcomp::String("Event could not be started: %1").Arg(eventID));
      }
    } else {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Event does not exist: %1").Arg(eventID));
    }
  }
}

bool ChatManager::GMCommand_ExpertiseExtend(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_EXPERTISE_EXTEND)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int8_t val = 1;
  if ((argsCopy.size() != 0 && !GetIntegerArg<int8_t>(val, argsCopy)) ||
      val < -1) {
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  if (character) {
    int8_t newVal = 0;
    if (val == -1) {
      // No cap
      newVal = val;
    } else {
      newVal = (int8_t)((character->GetExpertiseExtension() + val) > 127
                            ? 127
                            : (character->GetExpertiseExtension() + val));
    }

    if (newVal != character->GetExpertiseExtension()) {
      auto server = mServer.lock();
      auto characterManager = server->GetCharacterManager();

      character->SetExpertiseExtension(newVal);

      characterManager->SendExpertiseExtension(client);
      server->GetWorldDatabase()->QueueUpdate(character,
                                              state->GetAccountUID());
    }

    return true;
  }

  return false;
}

bool ChatManager::GMCommand_ExpertiseSet(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_EXPERTISE_SET)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();

  uint8_t expertiseID;
  if (!GetIntegerArg(expertiseID, argsCopy)) {
    return false;
  }

  uint8_t rank;
  if (!GetIntegerArg(rank, argsCopy)) {
    return false;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto exp = character->GetExpertises((size_t)expertiseID).Get();

  int32_t adjust = (rank * 10000);
  if (exp) {
    adjust = adjust - exp->GetPoints();
  }

  std::list<std::pair<uint8_t, int32_t>> pointMap;
  pointMap.push_back(std::make_pair(expertiseID, adjust));

  server->GetCharacterManager()->UpdateExpertisePoints(client, pointMap, true);

  return true;
}

bool ChatManager::GMCommand_Familiarity(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_FAMILIARITY)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint16_t familiarity;

  if (!GetIntegerArg<uint16_t>(familiarity, argsCopy)) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->UpdateFamiliarity(client, familiarity);

  return true;
}

bool ChatManager::GMCommand_Flag(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_FLAG)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  libcomp::String source;

  if (argsCopy.size() > 0 && !GetStringArg(source, argsCopy)) {
    return false;
  }

  source = source.ToLower();

  bool isInstance = "inst" == source;

  if ("zone" != source && !isInstance) {
    return false;
  }

  auto cState = client->GetClientState()->GetCharacterState();

  if (!cState) {
    return false;
  }

  auto zone = cState->GetZone();

  if (!zone) {
    return false;
  }

  auto instance = zone->GetInstance();

  if (isInstance && !instance) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           libcomp::String("Zone is not an instance!"));
  }

  if (argsCopy.empty()) {
    // List flags.
    auto flags = isInstance ? instance->GetFlagStates() : zone->GetFlagStates();

    SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("CID, Key, Value"));

    for (auto it : flags) {
      auto cid = it.first;

      for (auto it2 : it.second) {
        auto key = it2.first;
        auto val = it2.second;

        SendChatMessage(
            client, ChatType_t::CHAT_SELF,
            libcomp::String("%1, %2, %3").Arg(cid).Arg(key).Arg(val));
      }
    }
  } else {
    int32_t cid, key;

    if (argsCopy.empty() || !GetIntegerArg<int32_t>(cid, argsCopy)) {
      return false;
    }

    if (argsCopy.empty() || !GetIntegerArg<int32_t>(key, argsCopy)) {
      return false;
    }

    bool get = argsCopy.empty();

    // Get or set flag.
    if (get) {
      int32_t value = isInstance ? instance->GetFlagStateValue(key, 0, cid)
                                 : zone->GetFlagStateValue(key, 0, cid);

      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             libcomp::String("Value: %1").Arg(value));
    } else  // set
    {
      int32_t value;

      if (argsCopy.empty() || !GetIntegerArg<int32_t>(value, argsCopy)) {
        return false;
      }

      if (isInstance) {
        instance->SetFlagState(key, value, cid);
      } else {
        zone->SetFlagState(key, value, cid);
      }

      return true;
    }
  }

  return true;
}

bool ChatManager::GMCommand_FusionGauge(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_FUSION_GAUGE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint32_t points;

  if (!GetIntegerArg(points, argsCopy)) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->UpdateFusionGauge(
      client, (int32_t)points, false);

  return true;
}

bool ChatManager::GMCommand_Goto(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_GOTO)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  bool toSelf = false;
  libcomp::String name;
  if (argsCopy.size() > 0 && !GetStringArg(name, argsCopy)) {
    return false;
  }

  if (name == "self") {
    toSelf = true;
    if (!GetStringArg(name, argsCopy)) {
      return false;
    }
  }

  std::shared_ptr<ChannelClientConnection> oClient;
  if (name.IsEmpty()) {
    oClient = client;
  } else {
    std::shared_ptr<objects::Character> oTarget;
    std::shared_ptr<objects::Account> oTargetAccount;
    GetTargetCharacterAccount(name, true, oTarget, oTargetAccount, oClient);
  }

  if (oClient) {
    auto zoneManager = mServer.lock()->GetZoneManager();

    auto cState = client->GetClientState()->GetCharacterState();
    auto oTargetState = oClient->GetClientState();
    auto oTargetCState = oTargetState->GetCharacterState();
    if (oTargetCState && cState->GetZone() == oTargetCState->GetZone()) {
      auto fromCState = toSelf ? oTargetCState : cState;
      auto fromDState = toSelf ? oTargetState->GetDemonState()
                               : client->GetClientState()->GetDemonState();
      auto toCState = toSelf ? cState : oTargetCState;

      float destX = toCState->GetCurrentX();
      float destY = toCState->GetCurrentY();

      zoneManager->Warp(client, fromCState, destX, destY,
                        toCState->GetDestinationRotation());
      if (fromDState->GetEntity()) {
        zoneManager->Warp(client, fromDState, destX, destY,
                          toCState->GetDestinationRotation());
      }

      return true;
    } else {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Player '%1' does not exist in the same zone")
              .Arg(name));
    }
  } else {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String(
            "Character '%1' is not currently logged into this channel")
            .Arg(name));
  }
}

bool ChatManager::GMCommand_GradePoints(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_GRADE_POINTS)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto targetClient = client;
  auto targetAccount =
      client->GetClientState()->GetAccountLogin()->GetAccount().Get();
  auto server = mServer.lock();

  int32_t gp = 0;
  if (!GetIntegerArg(gp, argsCopy) || gp < 0) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("@gp requires a positive amount be specified"));
  }

  libcomp::String name;
  if (GetStringArg(name, argsCopy)) {
    std::shared_ptr<objects::Character> targetCharacter;
    if (!GetTargetCharacterAccount(name, false, targetCharacter, targetAccount,
                                   targetClient)) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Invalid character name supplied for GP command: %1")
              .Arg(name));
    }
  }

  auto pvpData = server->GetMatchManager()->GetPvPData(targetClient);
  if (!pvpData) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "GP could not be updated");
  }

  if (gp >= 1000) {
    gp = gp - 1000;
    pvpData->SetRanked(true);
  } else {
    pvpData->SetRanked(false);
  }

  if (gp > 2000) {
    gp = 2000;
  }

  pvpData->SetGP(gp);

  server->GetCharacterManager()->SendPvPCharacterInfo(targetClient);

  server->GetWorldDatabase()->QueueUpdate(
      pvpData, targetClient->GetClientState()->GetAccountUID());

  return true;
}

bool ChatManager::GMCommand_Help(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  static std::map<std::string, std::vector<std::string>> usage = {
      {"addcp",
       {
           "@addcp AMOUNT [NAME]",
           "Gives AMOUNT of CP to the character NAME or to yourself",
           "if no NAME is specified.",
       }},
      {"announce",
       {
           "@announce COLOR MESSAGE...",
           "Announce a ticker MESSAGE with the specified COLOR.",
       }},
      {"ban",
       {"@ban NAME [1/2/3] [REASON...]",
        "Bans the account which owns the character NAME. If the",
        "account is not on the channel, remove them with options",
        "1 (request current channel, default), 2 (request any",
        "channel) or 3 (remove from world/lobby, USE AT OWN RISK).",
        "Upon success, all open postings belonging to the account",
        "related to the current channel will be immediately closed.",
        "Can be used repeatedly for multi-channel setups."}},
      {"bethel",
       {"@bethel INDEX AMOUNT",
        "Set the current character's bethel AMOUNT corresponding",
        "to the supplied INDEX (0-4)."}},
      {"bp",
       {"@bp POINTS", "Set the current character's Battle Point current",
        "amount and total accumulated points as well."}},
      {"coin",
       {"@coin AMOUNT", "Set the current character's casino coin AMOUNT."}},
      {"contract",
       {
           "@contract ID|NAME",
           "Adds demon given by it's ID or NAME to your COMP.",
       }},
      {"counter",
       {"@counter ID [NAME] [VALUE]",
        "View a world counter with a specific ID or view/add",
        "to the VALUE of the counter from character with NAME."}},
      {"cowrie",
       {"@cowrie AMOUNT", "Set the current character's cowrie AMOUNT."}},
      {"crash",
       {
           "@crash",
           "Causes the server to crash for testing the database.",
       }},
      {"dxp",
       {"@dxp RACEID POINTS",
        "Gain digitalize XP for the specified demon race ID."}},
      {"effect",
       {"@effect ID [+/-]STACK [DEMON]",
        "Add or sets the stack count for status effect with",
        "the given ID for the player or demon if DEMON is set", "to 'demon'."}},
      {"enchant",
       {
           "@enchant EQUIP TAROT SOUL",
           "Set the enchnantment TAROT and SOUL effects for the",
           "specified EQUIP type. Use 0 to remove.",
           "EQUIP types: HEAD (0), FACE (1), NECK (2), TOP (3),",
           "ARMS (4), BOTTOM (5), FEET (6), COMP (7), RING (8),",
           "EARRING (9), EXTRA (10), BACK (11), TALISMAN (12)",
           "or WEAPON (13)",
       }},
      {"enemy",
       {"@enemy ID|NAME [AI [X Y [ROT]]]",
        "Spawns the enemy with the given ID or NAME at the",
        "position of the character or at the position X, Y",
        "with the rotation ROT. If AI is specified that AI",
        "type is used instead of the default."}},
      {"event",
       {"@event [ID [PARAMS]]",
        "Starts an event specified by ID or returns the current",
        "event if not specified. If additional PARAMS are supplied",
        "and the event is a transform event, the params will be used",
        "to override the existing transform script params."}},
      {"expertise",
       {"@expertise ID RANK", "Sets the expertise by ID to a specified RANK."}},
      {"expertisemax",
       {"@expertisemax STACKS",
        "Adds STACKS * 1000 points to the expertise cap.",
        "Maximum expertise cap is 154,000."}},
      {"familiarity",
       {"@familiarity VALUE",
        "Updates the current partner's familiarity to the given",
        "VALUE which can be in the range [0-10000]."}},
      {"flag",
       {
           "@flag TYPE [CID KEY [VALUE]]",
           "List, get or set zone flags. TYPE must be 'zone' or 'inst'.",
           "CID may be 0 for no specific character. If VALUE is not",
           "given the key is printed out instead of set.",
       }},
      {"fgauge",
       {"@fgauge VALUE",
        "Updates the current character's fusion gauge to the given",
        "VALUE which can be in the range of [0-10000] times the",
        "number of fusion gauge stocks available."}},
      {"goto",
       {"@goto [SELF] NAME",
        "If SELF is set to 'self' the player is moved to the",
        "character with the given name. If not set, the character",
        "with the given name is moved to the player."}},
      {"gp",
       {"@gp VALUE [NAME]",
        "Set the Grade Points on the character NAME or to yourself",
        "if no NAME is specified. Ranked grades are 1000 points",
        "higher than the value displayed (ex: 1000 for ranked base)."}},
      {"help",
       {"@help [GMAND]",
        "Lists the GMands supported by the server or prints the",
        "description of the GMAND specified."}},
      {"homepoint",
       {
           "@homepoint",
           "Sets your current position as your homepoint.",
       }},
      {"instance",
       {
           "@instance ID [VARIANTID]",
           "Creates a dungeon instance given the specified ",
           "instance ID and optional variant ID. This IDs must be in",
           "the XML data.",
       }},
      {"item",
       {"@item ID|NAME [QTY]",
        "Adds the item given by the ID or NAME in the specified",
        "quantity to the player's inventory. The NAME may be",
        "'macca' or 'mag' instead."}},
      {"kick",
       {"@kick NAME [1/2/3]",
        "Kicks the character with the given NAME from the server.",
        "If their account is not on the channel, remove them with",
        "options 1 (request current channel, default), 2 (request",
        "any channel) or 3 (remove from world/lobby, USE AT OWN", "RISK)."}},
      {"levelup",
       {"@levelup LEVEL [DEMON]",
        "Levels up the player to the specified LEVEL or the",
        "player's current partner if DEMON is set to 'demon'."}},
      {"license",
       {
           "@license",
           "Prints license information for the running server.",
       }},
      {"lnc",
       {"@lnc VALUE", "Sets the player's LNC to the given VALUE. VALUE should",
        "be in the range [-10000, 10000]."}},
      {"map",
       {
           "@map ID",
           "Adds map for the player with the given ID.",
       }},
      {"online",
       {"@online [NAME]", "Print how many players are online or check if the",
        "character with a specific NAME is online."}},
      {"penalty",
       {"@penalty [NAME]",
        "Remove all PvP penalties on the character NAME or to",
        "yourself if no NAME is specified."}},
      {"plugin",
       {
           "@plugin ID",
           "Adds plugin for the player with the given ID.",
       }},
      {"pos",
       {"@pos [SPOTID|X Y]",
        "Prints out the X, Y position of the player or moves",
        "the player to the given SPOTID or X, Y position."}},
      {"post",
       {"@post ID [NAME]",
        "Adds the post item given by the ID to the character",
        "specified by NAME's post. If NAME is not specified",
        "the player's post is used."}},
      {"quest",
       {"@quest ID PHASE",
        "Sets the phase of the quest given by the ID to the phase",
        "PHASE. A phase of -1 is complete and -2 is a reset."}},
      {"reported",
       {"@reported [COUNT|PLAYERNAME]",
        "Get a set of unresolved reported player records of a",
        "specified COUNT or targeted at a specific PLAYERNAME."}},
      {"resolve",
       {"@resolve ENTRYUID",
        "Resolve a reported player record by UID. Records can",
        "be retrieved via @reported."}},
      {"reunion",
       {"@reunion TYPE [RANK]",
        "Perform reunion on your currently summoned demon setting",
        "the normal TYPE [1-12] and RANK or explicit growth TYPE",
        "by ID if no RANK specified."}},
      {"scrap",
       {"@scrap SLOT [NAME]",
        "Removes the item in slot SLOT from the character's",
        "inventory whose NAME is specified. The SLOT is a value",
        "in the range [1, 50] and if the NAME is not specified",
        "the player's inventory is used."}},
      {"skill",
       {"@skill ID DEMON",
        "Grants the skill with the specified ID to the player or",
        "the player's partner if DEMON is set to 'demon'."}},
      {"skillpoint",
       {"@skillpoint PTS",
        "Adds the specified number of skill points PTS to the",
        "available skill points for allocation."}},
      {"slotadd",
       {"@slotadd EQUIP", "Adds a slot to the specified EQUIP type.",
        "EQUIP types: TOP (3), BOTTOM (5) or WEAPON (13)"}},
      {"sp",
       {
           "@sp PTS",
           "Updates the player's partner to have PTS SP.",
       }},
      {"spawn",
       {
           "@spawn",
           "Spawn the max number of enemies in each spawn group",
           "in the current zone.",
       }},
      {"speed",
       {"@speed MULTIPLIER [DEMON]",
        "Multiplies the speed of the player by MULTIPLIER or",
        "the demon if DEMON is set to 'demon'."}},
      {"spirit",
       {
           "@spirit EQUIP BASIC SPECIAL [B1 B2 B3]",
           "Set the spirit fusion BASIC and SPECIAL effects for the",
           "specified EQUIP type. Effect values are equal to the item",
           "ID they are gained from. B1-B3 fusion bonuses from 0-50",
           "can also be specified but will otherwise be ignored.",
           "EQUIP types: HEAD (0), FACE (1), NECK (2), TOP (3),",
           "ARMS (4), BOTTOM (5), FEET (6), COMP (7), RING (8),",
           "EARRING (9), EXTRA (10), BACK (11), TALISMAN (12)",
           "or WEAPON (13)",
       }},
      {"support",
       {"@support VALUE",
        "Show or hide the player character's support display state",
        "based on VALUE 1 (on) or 0 (off)."}},
      {"tickermessage",
       {
           "@tickermessage MESSAGE...",
           "Sends the ticker message MESSAGE to all players.",
       }},
      {"title",
       {"@title ID", "Grants the player a new character title by ID."}},
      {"tokusei",
       {"@tokusei CLEAR|ID [STACK] [DEMON]",
        "Adds STACK of a tokusei given by the specified ID",
        "to the player or the player's demon if DEMON is set",
        "to 'demon'. STACK must be specified if CLEAR is not",
        "set to 'clear'. DEMON may not be specified if CLEAR", "is set."}},
      {"valuable",
       {"@valuable ID [REMOVE]",
        "Grants the player the valuable with the given ID. If",
        "REMOVE is set to 'remove' the valuable is removed."}},
      {"version",
       {
           "@version",
           "Prints version information for the running server.",
       }},
      {"worldtime",
       {"@worldtime OFFSET",
        "Adjusts the current channel's world clock time by",
        "a specified OFFSET (in seconds). 2 for 1 minute,"
        "120 for 1 hour and 1440 for 1 phase."}},
      {"xp",
       {"@xp PTS [DEMON]", "Grants the player PTS XP or the demon if DEMON is",
        "set to 'demon'."}},
      {"ziotite",
       {"@ziotite SMALL LARGE",
        "Add to the current team's SMALL and LARGE ziotite."}},
      {"zone",
       {
           "@zone ID",
           "Moves the player to the zone specified by ID.",
       }},
  };

  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_HELP)) {
    return true;
  }

  libcomp::String command;

  if (1 < args.size()) {
    command = "help";
  } else if (!args.empty()) {
    command = args.front().ToLower();
  }

  if (command.IsEmpty()) {
    for (auto cmd : usage) {
      SendChatMessage(client, ChatType_t::CHAT_SELF, cmd.second.front());
    }
  } else {
    auto cmd = usage.find(command.ToUtf8());

    if (usage.end() != cmd) {
      for (auto line : cmd->second) {
        SendChatMessage(client, ChatType_t::CHAT_SELF, line);
      }
    } else {
      SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("The command '%1' does not exist.").Arg(command));
    }
  }

  return true;
}

bool ChatManager::GMCommand_Homepoint(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_HOMEPOINT)) {
    return true;
  }

  (void)args;

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();

  auto zoneDef = cState->GetZone()->GetDefinition();
  auto spots = mServer.lock()->GetDefinitionManager()->GetSpotData(
      zoneDef->GetDynamicMapID());
  for (auto spotPair : spots) {
    // Take the first zone-in point found
    if (spotPair.second->GetType() ==
        objects::MiSpotData::Type_t::ZONE_IN_POINT) {
      character->SetHomepointZone(zoneDef->GetID());
      character->SetHomepointSpotID(spotPair.first);

      libcomp::Packet p;
      p.WritePacketCode(
          ChannelToClientPacketCode_t::PACKET_EVENT_HOMEPOINT_UPDATE);
      p.WriteS32Little((int32_t)zoneDef->GetID());
      p.WriteFloat(spotPair.second->GetCenterX());
      p.WriteFloat(spotPair.second->GetCenterY());

      client->SendPacket(p);

      mServer.lock()->GetWorldDatabase()->QueueUpdate(character,
                                                      state->GetAccountUID());

      return true;
    }
  }

  return SendChatMessage(client, ChatType_t::CHAT_SELF,
                         "No valid spot ID found for the current zone");
}

bool ChatManager::GMCommand_Instance(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_INSTANCE)) {
    return true;
  }

  auto state = client->GetClientState();
  auto currentZone = state->GetZone();
  auto team = state->GetTeam();
  if (currentZone && currentZone->GetInstance()) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "You must leave the current instance to enter another");
  }

  std::list<libcomp::String> argsCopy = args;

  uint32_t instanceID;

  if (!GetIntegerArg<uint32_t>(instanceID, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@instance requires a zone instance ID");
  }

  auto server = mServer.lock();
  auto serverDataManager = server->GetServerDataManager();
  auto zoneManager = server->GetZoneManager();

  auto instDef = serverDataManager->GetZoneInstanceData(instanceID);
  if (!instDef) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Invalid instance ID supplied: %1").Arg(instanceID));
  }

  // Disapora has special rules for how you can enter it
  bool diaspora = false;

  uint32_t variantID = 0;
  std::shared_ptr<objects::ServerZoneInstanceVariant> variantDef;
  if (argsCopy.size() > 0) {
    if (!GetIntegerArg<uint32_t>(variantID, argsCopy)) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "Variant ID must be an integer");
    } else {
      variantDef = serverDataManager->GetZoneInstanceVariantData(variantID);
      if (!variantDef) {
        return SendChatMessage(
            client, ChatType_t::CHAT_SELF,
            libcomp::String("Invalid variant ID supplied: %1").Arg(variantID));
      }

      if (variantDef->GetInstanceType() == InstanceType_t::DIASPORA) {
        diaspora = true;
        if (!team ||
            team->GetCategory() != objects::Team::Category_t::DIASPORA) {
          return SendChatMessage(
              client, ChatType_t::CHAT_SELF,
              "Diaspora variant cannot be entered without a team");
        }
      }
    }
  }

  std::set<int32_t> accessCIDs = {state->GetWorldCID()};
  if (diaspora) {
    for (auto memberCID : team->GetMemberIDs()) {
      accessCIDs.insert(memberCID);
    }
  } else {
    auto party = state->GetParty();
    if (party) {
      for (auto memberCID : party->GetMemberIDs()) {
        accessCIDs.insert(memberCID);
      }
    }
  }

  auto instAccess = std::make_shared<objects::InstanceAccess>();
  instAccess->SetAccessCIDs(accessCIDs);
  instAccess->SetDefinitionID(instanceID);
  instAccess->SetVariantID(variantID);

  uint8_t resultCode = zoneManager->CreateInstance(instAccess);
  if (!resultCode ||
      (!diaspora && !zoneManager->MoveToInstance(client, instAccess))) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Failed to add to instance: %1").Arg(instanceID));
  }

  return true;
}

bool ChatManager::GMCommand_Item(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ITEM)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  uint32_t itemID;

  if (!GetIntegerArg<uint32_t>(itemID, argsCopy)) {
    libcomp::String name;
    if (!GetStringArg(name, argsCopy)) {
      return false;
    }

    if (name.ToLower() == "macca") {
      itemID = SVR_CONST.ITEM_MACCA;
    } else if (name.ToLower() == "mag") {
      itemID = SVR_CONST.ITEM_MAGNETITE;
    } else {
      auto itemData = definitionManager->GetItemData(name);
      if (itemData == nullptr) {
        return false;
      }
      itemID = itemData->GetCommon()->GetID();
    }
  }

  uint32_t stackSize;

  if (!GetIntegerArg<uint32_t>(stackSize, argsCopy)) {
    stackSize = 1;
  }

  std::unordered_map<uint32_t, uint32_t> itemMap;
  itemMap[itemID] = stackSize;

  return server->GetCharacterManager()->AddRemoveItems(client, itemMap, true);
}

bool ChatManager::GMCommand_Kick(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_KICK)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;
  libcomp::String kickedPlayer;

  if (!GetStringArg(kickedPlayer, argsCopy) || argsCopy.size() > 1) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@kick requires one argument, <username>");
  }

  int8_t kickLevel = 1;

  std::shared_ptr<objects::Character> target;
  std::shared_ptr<objects::Account> targetAccount;
  std::shared_ptr<channel::ChannelClientConnection> targetClient;
  if (!GetTargetCharacterAccount(kickedPlayer, false, target, targetAccount,
                                 targetClient) ||
      (targetAccount && !targetClient)) {
    if (!GetIntegerArg(kickLevel, argsCopy) || kickLevel == 0 ||
        kickLevel > 3) {
      kickLevel = 1;
    }

    SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String(
            "Target character is not currently on the channel to kick. Sending "
            "level %1 disconnect request to the world.")
            .Arg(kickLevel));
  }

  if (!targetAccount) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Supplied character name is invalid.");
  } else if (client->GetClientState()->GetUserLevel() <
             targetAccount->GetUserLevel()) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "Your user level is lower than the user you tried to kick.");
  }

  if (targetClient != nullptr) {
    targetClient->Close();
  } else {
    libcomp::Packet p;
    p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
    p.WriteU32Little((uint32_t)LogoutPacketAction_t::LOGOUT_DISCONNECT);
    p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                          targetAccount->GetUsername());
    p.WriteS8(kickLevel);

    mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(p);
  }

  return true;
}

bool ChatManager::GMCommand_LevelUp(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_LEVEL_UP)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int8_t lvl;

  if (GetIntegerArg<int8_t>(lvl, argsCopy)) {
    if (lvl > 99 || lvl < 1) {
      return false;
    }
  } else {
    // Increase by 1
    lvl = -1;
  }

  auto state = client->GetClientState();

  libcomp::String target;
  bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
  int32_t entityID;
  int8_t currentLevel;

  if (isDemon) {
    auto dState = state->GetDemonState();

    if (!dState->GetEntity()) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "No demon summoned");
    }

    entityID = dState->GetEntityID();
    currentLevel = dState->GetLevel();
  } else {
    auto cState = state->GetCharacterState();
    entityID = cState->GetEntityID();
    currentLevel = cState->GetLevel();
  }

  if (lvl == -1 && lvl != 99) {
    lvl = static_cast<int8_t>(currentLevel + 1);
  } else if (currentLevel >= lvl) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->LevelUp(client, lvl, entityID);

  return true;
}

bool ChatManager::GMCommand_License(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  (void)args;

  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  libcomp::String("COMP_hack Server v%1.%2.%3 (%4)")
                      .Arg(VERSION_MAJOR)
                      .Arg(VERSION_MINOR)
                      .Arg(VERSION_PATCH)
                      .Arg(VERSION_CODENAME));
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  libcomp::String("Copyright (C) 2010-%1 COMP_hack Team")
                      .Arg(VERSION_YEAR));

  SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "This program is free software: you can redistribute it and/or modify");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "it under the terms of the GNU Affero General Public License as");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "published by the Free Software Foundation, either version 3 of the");
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  "License, or (at your option) any later version.");
  SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "This program is distributed in the hope that it will be useful,");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "but WITHOUT ANY WARRANTY; without even the implied warranty of");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  "GNU Affero General Public License for more details.");
  SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  "You should have received a copy of the GNU Affero General "
                  "Public License");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "along with this program.  If not, see <https://www.gnu.org/licenses/>.");
  SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "IMPORTANT: If you see this you have a right to the source code!");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "This includes all modifications ANYONE has made to it! This does");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "not include any content changes (just the code). Please let us");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "know if someone isn't respecting the license for this software.");
  SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  "You can get the mainline source from here:");
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  "https://github.com/comphack/comp_hack");
  SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      "Hopefully if someone changed the code for the server you are connected");
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  "to they added their URL here as well.");

  return true;
}

bool ChatManager::GMCommand_LNC(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_LNC)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int16_t lnc;

  if (!GetIntegerArg<int16_t>(lnc, argsCopy)) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->UpdateLNC(client, lnc);

  return true;
}

bool ChatManager::GMCommand_Map(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_MAP)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint16_t mapID;
  if (!GetIntegerArg<uint16_t>(mapID, argsCopy)) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->AddMap(client, mapID);

  return true;
}

bool ChatManager::GMCommand_Online(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ONLINE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();

  std::shared_ptr<objects::Character> targetCharacter;

  libcomp::String name;
  if (GetStringArg(name, argsCopy)) {
    // Get location of specific character
    targetCharacter = objects::Character::LoadCharacterByName(
        server->GetWorldDatabase(), name);
    if (!targetCharacter) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String(
              "Invalid character name supplied for online command: %1")
              .Arg(name));
    }

    auto login =
        server->GetAccountManager()->GetActiveLogin(targetCharacter->GetUUID());

    uint32_t zoneID = login ? login->GetZoneID() : 0;
    if (zoneID) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("%1 is currently in zone %2").Arg(name).Arg(zoneID));
    } else {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("%1 is not currently online").Arg(name));
    }
  } else {
    // Get number of current players
    int8_t channelID = (int8_t)server->GetChannelID();

    int32_t total = 0;
    int32_t channel = 0;
    for (auto& pair : server->GetAccountManager()->GetActiveLogins()) {
      total++;
      if (pair.second->GetChannelID() == channelID) {
        channel++;
      }
    }

    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Characters online: %1 (%2 on current channel)")
            .Arg(total)
            .Arg(channel));
  }

  return true;
}

bool ChatManager::GMCommand_PenaltyReset(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_PENALTY_RESET)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto state = client->GetClientState();

  auto targetClient = client;
  auto targetCharacter = state->GetCharacterState()->GetEntity();
  auto targetAccount = state->GetAccountLogin()->GetAccount().Get();

  libcomp::String name;
  if (GetStringArg(name, argsCopy)) {
    if (!GetTargetCharacterAccount(name, false, targetCharacter, targetAccount,
                                   targetClient)) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String(
              "Invalid character name supplied for penalty reset command: %1")
              .Arg(name));
    }
  }

  auto pvpData =
      targetCharacter ? targetCharacter->GetPvPData().Get() : nullptr;
  if (pvpData) {
    // If the character does not have PvPData already, there is nothing
    // to do
    auto server = mServer.lock();
    pvpData->SetPenaltyCount(0);

    server->GetCharacterManager()->SendPvPCharacterInfo(targetClient);

    server->GetWorldDatabase()->QueueUpdate(
        pvpData, targetClient->GetClientState()->GetAccountUID());
  }

  return true;
}

bool ChatManager::GMCommand_Plugin(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_PLUGIN)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint16_t pluginID;
  if (!GetIntegerArg<uint16_t>(pluginID, argsCopy)) {
    return false;
  }

  if (!mServer.lock()->GetCharacterManager()->AddPlugin(client, pluginID)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Invalid plugin ID supplied for @plugin command");
  }

  return true;
}

bool ChatManager::GMCommand_Position(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_POSITION)) {
    return true;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();

  std::list<libcomp::String> argsCopy = args;

  if (args.size() == 1) {
    // Go to spot
    uint32_t spotID = 0;
    if (!GetIntegerArg<uint32_t>(spotID, argsCopy)) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "Invalid args supplied for @pos command");
    }

    auto zone = cState->GetZone();
    auto def = zone ? zone->GetDefinition() : nullptr;

    float x, y, rot;
    if (def && zoneManager->GetSpotPosition(def->GetDynamicMapID(), spotID, x,
                                            y, rot)) {
      auto dState = state->GetDemonState();

      zoneManager->Warp(client, cState, x, y, rot);
      if (dState->GetEntity()) {
        zoneManager->Warp(client, dState, x, y, rot);
      }
    } else {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "Failed to find spot for @pos command");
    }

    return true;
  }
  if (args.size() == 2) {
    // Go to coordinates
    float destX, destY;

    if (!GetDecimalArg<float>(destX, argsCopy) ||
        !GetDecimalArg<float>(destY, argsCopy)) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "Invalid args supplied for @pos command");
    }

    auto dState = state->GetDemonState();

    zoneManager->Warp(client, cState, destX, destY,
                      cState->GetDestinationRotation());
    if (dState->GetEntity()) {
      zoneManager->Warp(client, dState, destX, destY,
                        dState->GetDestinationRotation());
    }

    return true;
  } else if (!args.empty()) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@pos supplied with too many args");
  }

  cState->RefreshCurrentPosition(server->GetServerTime());
  return SendChatMessage(client, ChatType_t::CHAT_SELF,
                         libcomp::String("Position: (%1, %2)")
                             .Arg(cState->GetCurrentX())
                             .Arg(cState->GetCurrentY()));
}

bool ChatManager::GMCommand_Post(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_POST)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto targetAccount = client->GetClientState()->GetAccountUID();
  auto server = mServer.lock();
  auto lobbyDB = server->GetLobbyDatabase();
  auto worldDB = server->GetWorldDatabase();
  auto definitionManager = server->GetDefinitionManager();

  uint32_t productID;

  if (!GetIntegerArg<uint32_t>(productID, argsCopy) ||
      !definitionManager->GetShopProductData(productID)) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Invalid shop product ID specified: %1\n")
            .Arg(productID));
  }

  libcomp::String name;

  if (GetStringArg(name, argsCopy)) {
    auto target = objects::Character::LoadCharacterByName(worldDB, name);
    targetAccount = target ? target->GetAccount() : NULLUUID;
  }

  if (targetAccount.IsNull()) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Invalid post target character specified: %1\n")
            .Arg(name));
  }

  auto postItems =
      objects::PostItem::LoadPostItemListByAccount(lobbyDB, targetAccount);
  if (postItems.size() >= MAX_POST_ITEM_COUNT) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "There is no more room in the Post!");
  }

  auto postItem = libcomp::PersistentObject::New<objects::PostItem>(true);
  postItem->SetType(productID);
  postItem->SetTimestamp((uint32_t)std::time(0));
  postItem->SetAccount(targetAccount);

  postItem->Insert(lobbyDB);

  return true;
}

bool ChatManager::GMCommand_Reported(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_REPORTED)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto lobbyDB = server->GetLobbyDatabase();
  auto worldDB = server->GetWorldDatabase();

  uint8_t count = 5;
  libcomp::String playerName;
  if (!GetIntegerArg(count, argsCopy)) {
    GetStringArg(playerName, argsCopy);
  }

  // Cap at 5 records
  if (count > 5) {
    count = 5;
  }

  std::list<std::shared_ptr<objects::ReportedPlayer>> reported;
  if (!playerName.IsEmpty()) {
    for (auto entry :
         objects::ReportedPlayer::LoadReportedPlayerListByPlayerName(
             worldDB, playerName)) {
      if (!entry->GetResolved()) {
        reported.push_back(entry);
      }
    }
  } else {
    reported = objects::ReportedPlayer::LoadReportedPlayerListByResolved(
        worldDB, false);
  }

  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      libcomp::String("%1 record%2 found%3")
          .Arg(reported.size())
          .Arg(reported.size() != 1 ? "s" : "")
          .Arg(reported.size() > (size_t)count ? " (limited to 5)" : ""));

  for (auto entry : reported) {
    SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Entry: %1").Arg(entry->GetUUID().ToString()));
    if (playerName.IsEmpty()) {
      SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Player: %1").Arg(entry->GetPlayerName()));
    }

    auto account = entry->GetReporter().Get(lobbyDB);

    SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Reported by: %1")
                        .Arg(account ? account->GetUsername() : "[Unknown]"));

    libcomp::String subject;
    switch (entry->GetSubject()) {
      case objects::ReportedPlayer::Subject_t::CHAT_HARASSMENT:
        subject = "Chat harassment";
        break;
      case objects::ReportedPlayer::Subject_t::ABUSIVE_ACTION:
        subject = "Abusive action";
        break;
      case objects::ReportedPlayer::Subject_t::PROGRESS_OBSTRUCTION:
        subject = "Progress obstruction";
        break;
      case objects::ReportedPlayer::Subject_t::CHEATING:
        subject = "Cheating";
        break;
      case objects::ReportedPlayer::Subject_t::ILLEGAL_TOOL_USE:
        subject = "Illegal tool use";
        break;
      case objects::ReportedPlayer::Subject_t::REAL_MONEY_TRADE:
        subject = "Real money trade";
        break;
      case objects::ReportedPlayer::Subject_t::MISC_HARASSMENT:
        subject = "Misc harassment";
        break;
      case objects::ReportedPlayer::Subject_t::UNSPECIFIED:
      default:
        subject = "Unspecified";
        break;
    }

    SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Reason: %1").Arg(subject));

    if (!entry->GetLocation().IsEmpty()) {
      SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Location: %1").Arg(entry->GetLocation()));
    }

    SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Comment: %1").Arg(entry->GetComment()));
  }

  return true;
}

bool ChatManager::GMCommand_Resolve(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_RESOLVE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto worldDB = server->GetWorldDatabase();

  bool failed = false;

  libcomp::String uidStr;
  libobjgen::UUID uuid;
  if (!GetStringArg(uidStr, argsCopy)) {
    failed = true;
  } else {
    uuid = libobjgen::UUID(uidStr.C());
    failed = uuid.IsNull();
  }

  if (!failed) {
    auto reported =
        libcomp::PersistentObject::LoadObjectByUUID<objects::ReportedPlayer>(
            worldDB, uuid);
    if (reported) {
      if (!reported->GetResolved()) {
        auto state = client->GetClientState();

        reported->SetResolved(true);
        reported->SetResolveTime((uint32_t)std::time(0));
        reported->SetResolver(state->GetAccountUID());

        failed = !reported->Update(worldDB);
      }
    } else {
      failed = true;
    }
  }

  if (failed) {
    SendChatMessage(client, ChatType_t::CHAT_SELF, "Resolve failed");
  }

  return true;
}

bool ChatManager::GMCommand_Reunion(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_REUNION)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint8_t type;
  int8_t rank = -1;

  if (!GetIntegerArg(type, argsCopy) ||
      (argsCopy.size() > 0 && !GetIntegerArg(rank, argsCopy))) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF, "Invalid parameters");
  }

  auto state = client->GetClientState();
  auto dState = state->GetDemonState();
  auto demon = dState->GetEntity();
  if (!demon) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Demon must be summoned");
  }

  int64_t demonID = state->GetObjectID(demon->GetUUID());

  auto server = mServer.lock();
  if (rank >= 0) {
    if (type < 1 || type > 12) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          "Invalid type specified with rank. Should be between 1 and 12");
    }

    // Determine actual growth type
    bool found = false;
    for (auto& pair :
         server->GetDefinitionManager()->GetAllDevilLVUpRateData()) {
      auto lvlUpRate = pair.second;
      if ((uint8_t)lvlUpRate->GetGroupID() == type &&
          (lvlUpRate->GetSubID() == rank ||
           (rank > 9 && lvlUpRate->GetSubID() == 9))) {
        type = (uint8_t)pair.first;
        found = true;
        break;
      }
    }

    if (!found) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "Invalid type/rank combination specified");
    }
  }

  if (!mServer.lock()->GetCharacterManager()->ReunionDemon(
          client, demonID, type, 0, false, true, rank)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF, "Reunion failed");
  }

  return true;
}

bool ChatManager::GMCommand_Quest(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_QUEST)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  uint32_t questID = 0;
  bool validQuestID = GetIntegerArg<uint32_t>(questID, argsCopy);
  auto questData =
      validQuestID ? definitionManager->GetQuestData(questID) : nullptr;
  if (!questData) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Invalid quest ID specified: %1").Arg(questID));
  }

  int8_t phase;
  if (!GetIntegerArg<int8_t>(phase, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "No phase specified for @quest command");
  } else if (phase < -2 || (int8_t)questData->GetPhaseCount() < phase) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String("Invalid phase '%1' supplied for quest: %2")
            .Arg(phase)
            .Arg(questID));
  }

  server->GetEventManager()->UpdateQuest(client, (int16_t)questID, phase, true);

  return true;
}

bool ChatManager::GMCommand_Scrap(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SCRAP)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint8_t slotNum = 0;
  if (GetIntegerArg<uint8_t>(slotNum, argsCopy)) {
    if (slotNum == 0 || slotNum > 50) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "@scrap slot numbers must be between 1 and 50");
    }
  } else {
    slotNum = 50;
  }

  auto targetClient = client;

  libcomp::String name;
  if (GetStringArg(name, argsCopy)) {
    targetClient = nullptr;

    auto zoneManager = mServer.lock()->GetZoneManager();
    for (auto zConnection : zoneManager->GetZoneConnections(client, true)) {
      auto zChar =
          zConnection->GetClientState()->GetCharacterState()->GetEntity();

      if (zChar && zChar->GetName() == name) {
        targetClient = zConnection;
        break;
      }
    }

    if (!targetClient) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String(
              "Invalid character name supplied for the current zone: %1")
              .Arg(name));
    }
  }

  if (targetClient) {
    auto cState = targetClient->GetClientState()->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character ? character->GetItemBoxes(0).Get() : nullptr;
    auto item =
        inventory ? inventory->GetItems((size_t)(slotNum - 1)).Get() : nullptr;
    if (item) {
      uint16_t stackSize = item->GetStackSize();

      std::list<std::shared_ptr<objects::Item>> insertItems;
      std::unordered_map<std::shared_ptr<objects::Item>, uint16_t>
          stackAdjustItems;
      stackAdjustItems[item] = 0;

      if (mServer.lock()->GetCharacterManager()->UpdateItems(
              targetClient, false, insertItems, stackAdjustItems)) {
        return SendChatMessage(
            client, ChatType_t::CHAT_SELF,
            libcomp::String("Item %1 (x%2) in slot %3 scrapped")
                .Arg(item->GetType())
                .Arg(stackSize)
                .Arg(slotNum));
      }
    } else {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("No item in slot %1").Arg(slotNum));
    }
  }

  return SendChatMessage(client, ChatType_t::CHAT_SELF, "Could not scrap item");
}

bool ChatManager::GMCommand_Skill(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SKILL)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto state = client->GetClientState();
  auto definitionManager = server->GetDefinitionManager();

  uint32_t skillID;

  if (!GetIntegerArg<uint32_t>(skillID, argsCopy)) {
    return false;
  }

  auto skill = definitionManager->GetSkillData(skillID);

  if (skill == nullptr) {
    return false;
  }

  libcomp::String target;
  bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
  auto entityID = isDemon ? state->GetDemonState()->GetEntityID()
                          : state->GetCharacterState()->GetEntityID();

  return mServer.lock()->GetCharacterManager()->LearnSkill(client, entityID,
                                                           skillID);
}

bool ChatManager::GMCommand_SkillPoint(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SKILL_POINT)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int32_t pointCount;
  if (!GetIntegerArg<int32_t>(pointCount, argsCopy) || pointCount < 0) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->UpdateSkillPoints(client, pointCount);

  return true;
}

bool ChatManager::GMCommand_SlotAdd(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SLOT_ADD)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  const std::set<int8_t> validTypes = {
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON};

  int8_t equipType = GetEquipTypeArg(client, argsCopy, "slotadd", validTypes);
  if (equipType == -1) {
    return true;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto item = character ? character->GetEquippedItems((size_t)equipType).Get()
                        : nullptr;

  if (!item) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "The specified item type is not equipped");
  }

  size_t openSlot = 0;
  for (uint32_t modSlot : item->GetModSlots()) {
    if (modSlot == 0) {
      break;
    }

    openSlot++;
  }

  if (openSlot < item->ModSlotsCount()) {
    item->SetModSlots(openSlot, MOD_SLOT_NULL_EFFECT);

    auto server = mServer.lock();

    auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
        libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
    if (itemBox) {
      server->GetCharacterManager()->SendItemBoxData(
          client, itemBox, {(uint16_t)item->GetBoxSlot()});
    }

    server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());
  }

  return true;
}

bool ChatManager::GMCommand_SoulPoints(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SOUL_POINTS)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int32_t pointCount;
  if (!GetIntegerArg<int32_t>(pointCount, argsCopy) || pointCount < 0) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->UpdateSoulPoints(client, pointCount,
                                                          true);

  return true;
}

bool ChatManager::GMCommand_Spawn(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SPAWN)) {
    return true;
  }

  (void)args;

  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();
  auto zone = zoneManager->GetCurrentZone(client);

  zoneManager->UpdateSpawnGroups(zone, true);
  return true;
}

bool ChatManager::GMCommand_Speed(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SPEED)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto state = client->GetClientState();

  float scaling = 1.f;
  GetDecimalArg<float>(scaling, argsCopy);

  libcomp::String target;
  bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
  auto entity =
      isDemon
          ? std::dynamic_pointer_cast<ActiveEntityState>(state->GetDemonState())
          : std::dynamic_pointer_cast<ActiveEntityState>(
                state->GetCharacterState());

  if (entity) {
    int16_t defaultSpeed = entity->GetCombatRunSpeed();
    int16_t speed = (int16_t)ceil((float)defaultSpeed * scaling);
    int16_t maxSpeed = (int16_t)ceil((double)defaultSpeed * 1.5);

    if (speed > maxSpeed) {
      maxSpeed = speed;
    }

    entity->SetSpeedBoost((int16_t)(speed - defaultSpeed));

    server->GetCharacterManager()->SendMovementSpeed(client, entity, false);
  }

  return true;
}

bool ChatManager::GMCommand_Spirit(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SPIRIT)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  // All but BULLET types are valid
  const std::set<int8_t> validTypes = {
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_HEAD,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FACE,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NECK,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_ARMS,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FEET,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_COMP,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EARRING,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EXTRA,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BACK,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TALISMAN,
      (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON,
  };

  int8_t equipType = GetEquipTypeArg(client, argsCopy, "spirit", validTypes);
  if (equipType == -1) {
    return true;
  }

  auto server = mServer.lock();
  auto definitionManager = server->GetDefinitionManager();

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto character = cState->GetEntity();
  auto item = character ? character->GetEquippedItems((size_t)equipType).Get()
                        : nullptr;

  if (!item) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "The specified item type is not equipped");
  }

  uint32_t basicEffect = 0;
  uint32_t specialEffect = 0;

  int8_t bonuses[] = {-1, -1, -1};

  if (!GetIntegerArg(basicEffect, argsCopy) ||
      !GetIntegerArg(specialEffect, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Missing basic and special effects");
  }

  // Default to current effects or self if not specified
  bool basicDefault = false;
  if (basicEffect == 0) {
    basicEffect =
        item->GetBasicEffect() ? item->GetBasicEffect() : item->GetType();
    basicDefault = true;
  }

  bool specialDefault = false;
  if (specialEffect == 0) {
    specialEffect =
        item->GetSpecialEffect() ? item->GetSpecialEffect() : item->GetType();
    specialDefault = true;
  }

  for (size_t i = 0; i < 3; i++) {
    if (argsCopy.size() > 0 && (!GetIntegerArg(bonuses[i], argsCopy) ||
                                bonuses[i] < -1 || bonuses[i] > 50)) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Invalid bonus %1 value supplied").Arg(i));
    }
  }

  auto basicDef = definitionManager->GetItemData(basicEffect);
  auto specialDef = definitionManager->GetItemData(specialEffect);
  if (!basicDef || !specialDef ||
      (int8_t)basicDef->GetBasic()->GetEquipType() != equipType ||
      (int8_t)specialDef->GetBasic()->GetEquipType() != equipType) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "Invalid basic or special effect");
  }

  // Do not apply other restrictions, go nuts

  // If clearing all, remove all effects
  if (basicDefault && specialDefault && !bonuses[0] && !bonuses[1] &&
      !bonuses[2]) {
    item->SetBasicEffect(0);
    item->SetSpecialEffect(0);
  } else {
    item->SetBasicEffect(basicEffect);
    item->SetSpecialEffect(specialEffect);
  }

  for (size_t i = 0; i < 3; i++) {
    if (bonuses[i] >= 0) {
      item->SetFuseBonuses(i, bonuses[i]);
    }
  }

  auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
      libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
  if (itemBox) {
    server->GetCharacterManager()->SendItemBoxData(
        client, itemBox, {(uint16_t)item->GetBoxSlot()});
  }

  cState->RecalcEquipState(definitionManager);

  server->GetCharacterManager()->RecalculateTokuseiAndStats(cState, client);

  server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());

  return true;
}

bool ChatManager::GMCommand_Support(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_SUPPORT)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  auto server = mServer.lock();
  auto character = client->GetClientState()->GetCharacterState()->GetEntity();

  uint8_t val;
  if (!GetIntegerArg(val, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "No value supplied for @support command");
  }

  character->SetSupportDisplay(val != 0);

  return SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      libcomp::String("Support display %1. Changes will be visible the next "
                      "time character data is sent to any player.")
          .Arg(val != 0 ? "enabled" : "disabled"));
}

bool ChatManager::GMCommand_TickerMessage(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_TICKER_MESSAGE)) {
    return true;
  }

  auto server = mServer.lock();
  auto conf =
      std::dynamic_pointer_cast<objects::ChannelConfig>(server->GetConfig());
  std::list<libcomp::String> argsCopy = args;
  int8_t mode = 0;

  if (!GetIntegerArg(mode, argsCopy) || argsCopy.size() < 1) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "Syntax invalid, try @tickermessage <mode> <message>");
  }

  libcomp::String message = libcomp::String::Join(argsCopy, " ");

  if (mode == 1) {
    server->SendSystemMessage(client, message, 0, true);
  }

  conf->SetSystemMessage(message);

  return true;
}

bool ChatManager::GMCommand_Title(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_TITLE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int16_t titleID;
  if (!GetIntegerArg<int16_t>(titleID, argsCopy)) {
    return false;
  }

  mServer.lock()->GetCharacterManager()->AddTitle(client, titleID);

  return true;
}

bool ChatManager::GMCommand_Tokusei(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_TOKUSEI)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  bool isClear = false;
  if (argsCopy.size() > 0 && argsCopy.front() == "clear") {
    isClear = true;
  }

  auto server = mServer.lock();

  int32_t tokuseiID;
  if (!isClear) {
    if (!GetIntegerArg<int32_t>(tokuseiID, argsCopy)) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "@tokusei requires a tokusei ID or \"clear\"");
    }

    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetTokuseiData(tokuseiID);

    if (!def) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Invalid tokusei ID supplied: %1").Arg(tokuseiID));
    }
  }

  uint16_t count = 1;
  if (!isClear && !GetIntegerArg<uint16_t>(count, argsCopy)) {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@tokusei requires an effect count if not clearing");
  }

  auto state = client->GetClientState();

  libcomp::String target;
  bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
  auto eState =
      isDemon
          ? std::dynamic_pointer_cast<ActiveEntityState>(state->GetDemonState())
          : std::dynamic_pointer_cast<ActiveEntityState>(
                state->GetCharacterState());

  if (isClear) {
    eState->ClearAdditionalTokusei();
  } else {
    eState->SetAdditionalTokusei(tokuseiID, count);
  }

  server->GetTokuseiManager()->Recalculate(state->GetCharacterState(), true);

  return true;
}

bool ChatManager::GMCommand_Valuable(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_VALUABLE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint16_t valuableID;
  if (!GetIntegerArg<uint16_t>(valuableID, argsCopy)) {
    return false;
  }

  bool remove = argsCopy.size() > 0 && argsCopy.front() == "remove";
  if (!mServer.lock()->GetCharacterManager()->AddRemoveValuable(
          client, valuableID, remove)) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "Invalid valuable ID supplied for @valuable command");
  }

  return true;
}

bool ChatManager::GMCommand_Version(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  (void)args;

  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  libcomp::String("COMP_hack Server v%1.%2.%3 (%4)")
                      .Arg(VERSION_MAJOR)
                      .Arg(VERSION_MINOR)
                      .Arg(VERSION_PATCH)
                      .Arg(VERSION_CODENAME));

#if 1 == HAVE_GIT
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      libcomp::String("%1 on branch %2").Arg(szGitCommittish).Arg(szGitBranch));
  SendChatMessage(
      client, ChatType_t::CHAT_SELF,
      libcomp::String("Commit by %1 on %2").Arg(szGitAuthor).Arg(szGitDate));
  SendChatMessage(client, ChatType_t::CHAT_SELF, szGitDescription);
  SendChatMessage(client, ChatType_t::CHAT_SELF,
                  libcomp::String("URL: %1").Arg(szGitRemoteURL));
#endif

  return true;
}

bool ChatManager::GMCommand_WorldTime(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_WORLD_TIME)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  uint32_t offset;
  if (!GetIntegerArg<uint32_t>(offset, argsCopy)) {
    return false;
  }

  auto server = mServer.lock();

  server->SetTimeOffset(offset);

  auto clock = server->GetWorldClockTime();

  libcomp::Packet notify;
  notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WORLD_TIME);
  notify.WriteS8(clock.MoonPhase);
  notify.WriteS8(clock.Hour);
  notify.WriteS8(clock.Min);

  server->GetManagerConnection()->BroadcastPacketToClients(notify);

  return true;
}

bool ChatManager::GMCommand_Ziotite(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ZIOTITE)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int32_t sZiotite;
  int8_t lZiotite;
  if (!GetIntegerArg(sZiotite, argsCopy) ||
      !GetIntegerArg(lZiotite, argsCopy)) {
    return SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        "@ziotite requires both small and large ziotite values");
  }

  auto state = client->GetClientState();
  auto team = state->GetTeam();
  if (team) {
    // Snap to 0 if going under
    if (sZiotite + team->GetSmallZiotite() < 0) {
      sZiotite = -team->GetSmallZiotite();
    }

    if (lZiotite + team->GetLargeZiotite() < 0) {
      lZiotite = (int8_t)(-team->GetLargeZiotite());
    }

    mServer.lock()->GetMatchManager()->UpdateZiotite(team, sZiotite, lZiotite,
                                                     state->GetWorldCID());

    return true;
  } else {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "@ziotite failed. You are not currently in a team");
  }
}

bool ChatManager::GMCommand_Zone(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_ZONE)) {
    return true;
  }

  auto state = client->GetClientState();
  auto cState = state->GetCharacterState();
  auto server = mServer.lock();
  auto zoneManager = server->GetZoneManager();
  uint32_t zoneID = 0;
  uint32_t dynamicMapID = 0;
  float xCoord = 0.f;
  float yCoord = 0.f;
  float rotation = 0.f;
  std::list<libcomp::String> argsCopy = args;

  if (args.empty()) {
    auto zone = cState->GetZone();
    auto zoneData = zone ? zone->GetDefinition() : nullptr;
    auto zoneDef =
        zoneData
            ? server->GetDefinitionManager()->GetZoneData(zoneData->GetID())
            : nullptr;

    if (zoneDef) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             libcomp::String("You are in zone %1 (%2)")
                                 .Arg(zoneData->GetID())
                                 .Arg(zoneDef->GetBasic()->GetName()));
    } else if (zoneData) {
      return SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("You are in zone %1").Arg(zoneData->GetID()));
    } else {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "You are (somehow) in the twilight zone");
    }
  } else if (4 >= args.size()) {
    bool getDynamicMapID = args.size() == 2 || args.size() == 4;

    // Parse the zone ID and dynamic map ID.
    bool parseOk = GetIntegerArg<uint32_t>(zoneID, argsCopy);
    if (parseOk && getDynamicMapID) {
      parseOk = GetIntegerArg<uint32_t>(dynamicMapID, argsCopy);
    }

    std::shared_ptr<objects::ServerZone> zoneData;

    // If the zone ID argument is right, look for the zone.
    if (parseOk) {
      zoneData =
          server->GetServerDataManager()->GetZoneData(zoneID, dynamicMapID);
    }

    // If the ID did not parse or the zone does not exist, stop here.
    if (!parseOk || !zoneData) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "ERROR: INVALID ZONE ID.  Please enter a proper "
                             "zoneID and try again.");
    }

    if (argsCopy.size() < 2) {
      xCoord = zoneData->GetStartingX();
      yCoord = zoneData->GetStartingY();
      rotation = zoneData->GetStartingRotation();
    } else if (!GetDecimalArg<float>(xCoord, argsCopy) ||
               !GetDecimalArg<float>(yCoord, argsCopy)) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             "ERROR: One of the inputs is not a number.  "
                             "Please re-enter the command with proper inputs.");
    }

    if (!zoneManager->EnterZone(client, zoneID, dynamicMapID, xCoord, yCoord,
                                rotation, true)) {
      return SendChatMessage(client, ChatType_t::CHAT_SELF,
                             libcomp::String("Failed to enter zone: %1 (%2)")
                                 .Arg(zoneID)
                                 .Arg(dynamicMapID));
    }

    return true;
  } else {
    return SendChatMessage(client, ChatType_t::CHAT_SELF,
                           "USAGE: @zone [ID [X Y]]");
  }
}

bool ChatManager::GMCommand_XP(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args) {
  if (!HaveUserLevel(client, SVR_CONST.GM_CMD_LVL_XP)) {
    return true;
  }

  std::list<libcomp::String> argsCopy = args;

  int64_t xp;

  if (!GetIntegerArg<int64_t>(xp, argsCopy)) {
    return false;
  }

  auto state = client->GetClientState();

  libcomp::String target;
  bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
  auto entityID = isDemon ? state->GetDemonState()->GetEntityID()
                          : state->GetCharacterState()->GetEntityID();

  mServer.lock()->GetCharacterManager()->UpdateExperience(client, xp, entityID);

  return true;
}

bool ChatManager::HaveUserLevel(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    uint32_t requiredLevel) {
  int32_t currentLevel = client->GetClientState()->GetUserLevel();
  if (currentLevel < (int32_t)requiredLevel) {
    SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Requested GMand requires a user level of "
                                    "at least %1. Your level is only %2.")
                        .Arg(requiredLevel)
                        .Arg(currentLevel));
    return false;
  }

  return true;
}

bool ChatManager::GetTargetCharacterAccount(
    libcomp::String& targetName, bool currentOnly,
    std::shared_ptr<objects::Character>& targetCharacter,
    std::shared_ptr<objects::Account>& targetAccount,
    std::shared_ptr<channel::ChannelClientConnection>& targetClient) {
  auto server = mServer.lock();
  auto worldDB = server->GetWorldDatabase();

  targetCharacter =
      objects::Character::LoadCharacterByName(worldDB, targetName);
  if (targetCharacter) {
    targetAccount =
        libcomp::PersistentObject::LoadObjectByUUID<objects::Account>(
            server->GetLobbyDatabase(), targetCharacter->GetAccount());
  } else {
    targetAccount = nullptr;
  }

  targetClient = targetAccount
                     ? server->GetManagerConnection()->GetClientConnection(
                           targetAccount->GetUsername())
                     : nullptr;

  if (currentOnly && targetClient) {
    auto state = targetClient->GetClientState();
    auto cState = state ? state->GetCharacterState() : nullptr;
    if (!cState || cState->GetEntity() != targetCharacter) {
      targetClient = nullptr;
    }
  }

  return targetCharacter && targetAccount;
}

int8_t ChatManager::GetEquipTypeArg(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    std::list<libcomp::String>& args, const libcomp::String& gmandName,
    const std::set<int8_t>& validTypes) {
  // Listed in index order matching objects::MiItemBasicData::EquipType_t
  const libcomp::String txtVals[14] = {
      "head", "face", "neck",    "top",   "arms", "bottom",   "feet",
      "comp", "ring", "earring", "extra", "back", "talisman", "weapon"};

  int8_t result = -1;
  if (!GetIntegerArg(result, args)) {
    libcomp::String txt;
    if (!GetStringArg(txt, args)) {
      SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String("Missing equip type argument for command: @%1")
              .Arg(gmandName));
      return -1;
    }

    txt = txt.ToLower();
    for (int8_t i = 0; i < 14; i++) {
      if (txtVals[(size_t)i] == txt) {
        result = i;
        break;
      }
    }

    if (result == -1) {
      SendChatMessage(
          client, ChatType_t::CHAT_SELF,
          libcomp::String(
              "Invalid equip type argument '%1' supplied for command: @%2")
              .Arg(txt)
              .Arg(gmandName));
      return -1;
    }
  }

  if (validTypes.find(result) == validTypes.end()) {
    SendChatMessage(
        client, ChatType_t::CHAT_SELF,
        libcomp::String(
            "Unsupported equip type argument supplied for command: @%1")
            .Arg(gmandName));
    return -1;
  }

  return result;
}

bool ChatManager::GetStringArg(libcomp::String& outVal,
                               std::list<libcomp::String>& args,
                               libcomp::Convert::Encoding_t encoding) const {
  if (args.size() == 0) {
    return false;
  }

  outVal = args.front().Replace("\\\"", "\"");
  args.pop_front();

  // Rebuild the full string if it starts with a double quote,
  // end if another unescaped double quote is read
  if (outVal.At(0) == '"') {
    while (outVal.At(outVal.Length() - 1) != '"' &&
           (outVal.Length() < 2 || outVal.At(outVal.Length() - 2) != '\\') &&
           !args.empty()) {
      outVal += " " + args.front().Replace("\\\"", "\"");
      args.pop_front();
    }

    outVal = outVal.Mid(1, outVal.Length() - 2);
  }

  if (encoding != libcomp::Convert::Encoding_t::ENCODING_UTF8) {
    auto convertedBytes = libcomp::Convert::ToEncoding(encoding, outVal, false);
    outVal = libcomp::String(
        std::string(convertedBytes.begin(), convertedBytes.end()));
  }

  return true;
}
