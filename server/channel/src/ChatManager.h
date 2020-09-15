/**
 * @file server/channel/src/ChatManager.h
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

#ifndef SERVER_CHANNEL_SRC_CHATMANAGER_H
#define SERVER_CHANNEL_SRC_CHATMANAGER_H

#include "ChannelClientConnection.h"

// libcomp Includes
#include <PacketCodes.h>

namespace objects {
class Account;
class Character;
}  // namespace objects

namespace channel {

class ChannelServer;

/*
 * Visiblity for a chat message used internally to route messages. This level
 * is determined by the ChatType_t value.
 */
enum ChatVis_t : uint16_t {
  CHAT_VIS_SELF,
  CHAT_VIS_PARTY,
  CHAT_VIS_ZONE,
  CHAT_VIS_RANGE,
  CHAT_VIS_CLAN,
  CHAT_VIS_VERSUS,
  CHAT_VIS_TEAM,
};

/**
 * Manager to handle chat communication and processing of GM commands.
 */
class ChatManager {
 public:
  /**
   * Create a new ChatManager.
   * @param server Pointer back to the channel server this
   *  belongs to
   */
  ChatManager(const std::weak_ptr<ChannelServer>& server);

  /**
   * Clean up the ChatManager.
   */
  ~ChatManager();

  /**
   * Send a chat message to the specified chat channel.
   * @param client Pointer to the client that sent the message
   * @param chatChannel Channel to send the chat message to
   * @param message Message to send
   * @return true if the message was handled properly, else false
   */
  bool SendChatMessage(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      ChatType_t chatChannel, const libcomp::String& message);

  /**
   * Send a tell message to the specified character.
   * @param client Pointer to the client that sent the message
   * @param message Message to send
   * @param targetName Name of the player the message is sending to
   * @return true if the message was handled properly, else false
   */
  bool SendTellMessage(const std::shared_ptr<ChannelClientConnection>& client,
                       const libcomp::String& message,
                       const libcomp::String& targetName);

  /**
   * Send a chat message to the client's team.
   * @param client Pointer to the client that sent the message
   * @param message Message to send
   * @param teamID ID of the team the client belongs to. If it does not
   *  match, the message will not be sent.
   * @return true if the message was handled properly, else false
   */
  bool SendTeamChatMessage(
      const std::shared_ptr<ChannelClientConnection>& client,
      const libcomp::String& message, int32_t teamID);

  /**
   * Determine if a message being sent should be handled as a GMand.
   * @param client Pointer to the client that sent the message
   * @param message Message that may be a GMand
   * @return true if the message was a GMand, false if it should be sent
   *  as a message
   */
  bool HandleGMand(const std::shared_ptr<ChannelClientConnection>& client,
                   const libcomp::String& message);

  /**
   * Execute a GM command using the supplied arguments.
   * @param client Pointer to the client that sent the command
   * @param cmd Command to execute
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool ExecuteGMCommand(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const libcomp::String& cmd, const std::list<libcomp::String>& args);

 private:
  /**
   * GM command to add CP to a target account.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_AddCP(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to announce message to server .
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Announce(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to ban a player currently on the channel
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Ban(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update the client's Battle Points (BP).
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_BattlePoints(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update the client's bethel directly.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Bethel(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set the total casino coins the character has
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Coin(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to add a demon to a character's COMP.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Contract(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to view a world level or view/change a character level
   * counter
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Counter(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update the client's current cowrie.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Cowrie(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to crash the server (for testing).
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Crash(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to gain digitalize points.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_DigitalizePoints(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to apply a status effect to the client's character or
   * demon.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Effect(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set the tarot/soul effects on an equipped item.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Enchant(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to spawn an entity in the client's zone.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Enemy(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to start an event or return the current event name.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Event(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to extend a character's max expertise points available.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_ExpertiseExtend(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update a character's expertise to an explicit
   * rank (and class).
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_ExpertiseSet(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update the familiarity level of the client's
   * partner demon.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Familiarity(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to display or update a zone or instance flag.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Flag(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update the client's fusion gauge.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_FusionGauge(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to move one player to another player.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Goto(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update a specific player's Grade Points (GP).
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_GradePoints(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to display help for other commands.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Help(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set the client character's homepoint to their
   * current zone's first spawn in point encountered.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Homepoint(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to create a new zone instance and enter it.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Instance(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to add an item to a character's inventory.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Item(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to kick a player from the channel
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Kick(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to level up a character or demon.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_LevelUp(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to print the server license.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_License(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set a character's LNC alignment value.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_LNC(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set a character's obtained maps.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Map(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to get the number of players on each channel or to
   * determine if a specific character is currently online.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Online(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to reset a specific player's PvP penalties.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_PenaltyReset(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set a character's obtained plugins.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Plugin(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to get a character's position.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Position(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to add an item to a character's post.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Post(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set a specific quest to a specific phase,
   * ignoring prior requirements and state.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Quest(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to get reported players that have not been resolved
   * yet.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Reported(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to resolve reported players records.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Resolve(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to perform a demon reunion by changing its growth type.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Reunion(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to throw away an item in the client or target
   * character's inventory. Useful for destroying items that cannot
   * be stored or thrown away.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Scrap(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to have the client's character or partner demon
   * learn a skill.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Skill(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to have the client's character gain allocatable
   * skill points.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_SkillPoint(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to add a mod slot to an item equipped by the client's
   * character.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_SlotAdd(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to have the client's partner demon gain soul points.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_SoulPoints(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to refresh all the spawn groups in the client's zone.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Spawn(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update the client character or partner demon's
   * running speed.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Speed(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set the spirit fusion effects on an equipped item.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Spirit(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set the the client character's SupportDisplay flag.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Support(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set the default ticker message upon login
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled problerly, else false
   */
  bool GMCommand_TickerMessage(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to add an available title to the character
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled problerly, else false
   */
  bool GMCommand_Title(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to add a tokusei to the client's character or partner
   * demon, detached from any item, skill or status effect.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled problerly, else false
   */
  bool GMCommand_Tokusei(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to set a character's current valuables.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Valuable(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to print the server version.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Version(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to offset the world time (of this channel only) by
   * a specified number of seconds.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_WorldTime(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to update the client's team's ziotite.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Ziotite(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to zone to a new map.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_Zone(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * GM command to increase the XP of a character or demon.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments for the command
   * @return true if the command was handled properly, else false
   */
  bool GMCommand_XP(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      const std::list<libcomp::String>& args);

  /**
   * Determine if the client has appropriate user level necessary
   * to execute a GMand. If the client's level is too low, a message
   * will be returned informing them of the error.
   * @param client Pointer to the client that sent the command
   * @param requiredLevel User level required to execute the GMand
   * @return true if the client has the necessary user level
   */
  bool HaveUserLevel(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      uint32_t requiredLevel);

  /**
   * Get channel accessible character, account and client pointers based
   * upon the supplied character name. If the character's account is logged
   * in as a different character on the channel, the client will still be
   * returned unless otherwise specified.
   * @param targetName Name of the character to retrieve information from
   * @param currentOnly If true, the client will not be returned if the
   *  account is logged into the channel but not as the specified character
   * @param targetCharacter Output pointer to the retrieved character
   * @param targetAccount Output pointer to the retrieved character's account
   * @param targetClient Output pointer to the retrieved character's client
   * @return true if the character and account exist, false if they do not
   */
  bool GetTargetCharacterAccount(
      libcomp::String& targetName, bool currentOnly,
      std::shared_ptr<objects::Character>& targetCharacter,
      std::shared_ptr<objects::Account>& targetAccount,
      std::shared_ptr<channel::ChannelClientConnection>& targetClient);

  /**
   * Get the valid equip type index associated to the next argument in a list
   * by equip type name or index.
   * @param client Pointer to the client that sent the command
   * @param args List of arguments to read and update
   * @param gmandName Name of the gmand being executed
   * @param validTypes Equipment type indexes that are valid to return
   * @return Equipment index or -1 if invalid
   */
  int8_t GetEquipTypeArg(
      const std::shared_ptr<channel::ChannelClientConnection>& client,
      std::list<libcomp::String>& args, const libcomp::String& gmandName,
      const std::set<int8_t>& validTypes);

  /**
   * Get the next argument from the supplied argument list as a string.
   * @param outVal Output variable to return the string argument to
   * @param args List of arguments read and update
   * @param encoding Optional encoding to use when reading the string.
   *  Defaults to UTF8.
   * @return true if there was an argument in the list, else false
   */
  bool GetStringArg(libcomp::String& outVal, std::list<libcomp::String>& args,
                    libcomp::Convert::Encoding_t encoding =
                        libcomp::Convert::Encoding_t::ENCODING_UTF8) const;

  /**
   * Get the next argument from the supplied argument list as an integer
   * type.
   * @param outVal Output variable to return the integer argument to
   * @param args List of arguments to read and update
   * @return true if there was an argument in the list that was an
   *  integer, else false
   */
  template <typename T>
  bool GetIntegerArg(T& outVal, std::list<libcomp::String>& args) const {
    if (args.size() == 0) {
      return false;
    }

    bool ok = true;
    outVal = args.front().ToInteger<T>(&ok);
    if (ok) {
      args.pop_front();
    }

    return ok;
  }

  /**
   * Get next argument from the supplied argument list as a float/long type.
   * @param outVal Output variable to return the argument to
   * @param args List of arguments to read and update
   * @return true if there was an argument in the list that was an
   *  float/long, else false
   */
  template <typename T>
  bool GetDecimalArg(T& outVal, std::list<libcomp::String>& args) const {
    if (args.size() == 0) {
      return false;
    }

    bool ok = true;
    outVal = args.front().ToDecimal<T>(&ok);
    if (ok) {
      args.pop_front();
    }

    return ok;
  }

  /// Pointer to the channel server
  std::weak_ptr<ChannelServer> mServer;

  /// List of GM command parsers.
  std::unordered_map<libcomp::String,
                     std::function<bool(ChatManager&,
                                        const std::shared_ptr<
                                            channel::ChannelClientConnection>&,
                                        const std::list<libcomp::String>&)>>
      mGMands;
};

}  // namespace channel
#endif
