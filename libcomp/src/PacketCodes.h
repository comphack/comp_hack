/**
 * @file libcomp/src/PacketCodes.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Contains enums for internal and external packet codes.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_PACKETCODES_H
#define LIBCOMP_SRC_PACKETCODES_H

// Standard C++11 Includes
#include <stdint.h>
#include <type_traits>

/**
 * Request packet code from the game client to the lobby server.
 */
enum class ClientToLobbyPacketCode_t : uint16_t
{
    PACKET_LOGIN = 0x0003,  //!< Login request.
    PACKET_AUTH = 0x0005,   //!< Authorization request.
    PACKET_START_GAME = 0x0007, //!< Start game request.
    PACKET_CHARACTER_LIST = 0x0009, //!< Character list request.
    PACKET_WORLD_LIST = 0x000B, //!< World list request.
    PACKET_CREATE_CHARACTER = 0x000D,   //!< Create character request.
    PACKET_DELETE_CHARACTER = 0x000F,   //!< Delete character request.
    PACKET_QUERY_PURCHASE_TICKET = 0x0011,  //!< Query purchase ticket request.
    PACKET_PURCHASE_TICKET = 0x0013,    //!< Purchase ticket request.
};

/**
 * Response packet code from the lobby server to the game client.
 */
enum class LobbyToClientPacketCode_t : uint16_t
{
    PACKET_LOGIN = 0x0004, //!< Login response.
    PACKET_AUTH = 0x0006,  //!< Authorization response.
    PACKET_START_GAME = 0x0008,    //!< Start game response.
    PACKET_CHARACTER_LIST = 0x000A,    //!< Character list response.
    PACKET_WORLD_LIST = 0x000C,    //!< World list response.
    PACKET_CREATE_CHARACTER = 0x000E,  //!< Create character response.
    PACKET_DELETE_CHARACTER = 0x0010,  //!< Delete character response.
    PACKET_QUERY_PURCHASE_TICKET = 0x0012, //!< Query purchase ticket response.
    PACKET_PURCHASE_TICKET = 0x0014,   //!< Purchase ticket response.
};

/**
 * Request packet code from the game client to the channel server.
 */
enum class ClientToChannelPacketCode_t : uint16_t
{
    PACKET_LOGIN = 0x0000,  //!< Login request.
    PACKET_AUTH = 0x0002,   //!< Authorization request.
    PACKET_SEND_DATA = 0x0004,  //!< Request for data from the server.
    PACKET_LOGOUT = 0x0005,  //!< Logout request.
    PACKET_MOVE = 0x001C,  //!< Request to move an entity or object.
    PACKET_POPULATE_ZONE = 0x0019,  //!< Request to populate a zone with game objects and entities.
    PACKET_CHAT = 0x0026, //!< Request to add a message to the chat or process a GM command.
    PACKET_TELL = 0x0027, //!< Request to send a chat message to a specific player in the world.
    PACKET_ACTIVATE_SKILL = 0x0030, //!< Request to activate a player or demon skill.
    PACKET_EXECUTE_SKILL = 0x0031, //!< Request to execute a skill that has finished charging.
    PACKET_CANCEL_SKILL = 0x0032, //!< Request to execute a skill that has finished charging.
    PACKET_ALLOCATE_SKILL_POINT = 0x0049, //!< Request to allocate a skill point for a character.
    PACKET_TOGGLE_EXPERTISE = 0x004F, //!< Request to enable or disable an expertise.
    PACKET_LEARN_SKILL = 0x0051, //!< Request for a character to learn a skill.
    PACKET_UPDATE_DEMON_SKILL = 0x0054, //!< Request for a demon's learned skill set to be updated.
    PACKET_KEEP_ALIVE = 0x0056,  //!< Request/check to keep the connection alive.
    PACKET_FIX_OBJECT_POSITION = 0x0058,  //!< Request to fix a game object's position.
    PACKET_STATE = 0x005A,  //!< Request for the client's character state.
    PACKET_PARTNER_DEMON_DATA = 0x005B,  //!< Request for the client's active demon state.
    PACKET_DEMON_BOX = 0x005C,  //!< Demon box list request.
    PACKET_DEMON_BOX_DATA = 0x005E,  //!< Demon box demon data request.
    PACKET_CHANNEL_LIST = 0x0063,  //!< Request for the list of channels connected to the world.
    PACKET_REVIVE_CHARACTER = 0x0067,  //!< Request to revive the client's character.
    PACKET_STOP_MOVEMENT = 0x006F,  //!< Request to stop the movement of an entity or object.
    PACKET_SPOT_TRIGGERED = 0x0071,  //!< A spot has been triggered by the client.
    PACKET_WORLD_TIME = 0x0072,  //!< Request for the server's current world time.
    PACKET_ITEM_BOX = 0x0074,  //!< Request for info about a specific item box.
    PACKET_ITEM_MOVE = 0x0076,  //!< Request to move an item in an item box.
    PACKET_ITEM_DROP = 0x0077,  //!< Request to throw away an item from an item box.
    PACKET_ITEM_STACK = 0x0078,  //!< Request to stack or split stacked items in an item box.
    PACKET_EQUIPMENT_LIST = 0x007B,  //!< Request for equipment information.
    PACKET_TRADE_REQUEST = 0x007D,  //!< Request to start a trade with another player.
    PACKET_TRADE_ACCEPT = 0x0080,  //!< Request to accept the trade request from another player.
    PACKET_TRADE_ADD_ITEM = 0x0082,  //!< Request to add an item to the current trade offer.
    PACKET_TRADE_LOCK = 0x0085,  //!< Request to lock the current trade for acceptance.
    PACKET_TRADE_FINISH = 0x0088,  //!< Request to finish the current trade.
    PACKET_TRADE_CANCEL = 0x008B,  //!< Request to cancel the current trade.
    PACKET_CASH_BALANCE = 0x0090,  //!< Request for the current account's total CP.
    PACKET_SHOP_DATA = 0x0092,  //!< Request for details about an NPC shop.
    PACKET_SHOP_BUY = 0x0094,  //!< Request to buy an item from a shop.
    PACKET_SHOP_SELL = 0x0096,  //!< Request to sell an item to a shop.
    PACKET_DEMON_BOX_MOVE = 0x009A,  //!< Request to move a demon in a box.
    PACKET_DISMISS_DEMON = 0x009B,  //!< Request to dismiss a demon.
    PACKET_POST_LIST = 0x009C,  //!< Request to list items in the player account's Post.
    PACKET_POST_ITEM = 0x009E,  //!< Request to move an item from the Post into the current character's inventory.
    PACKET_HOTBAR_DATA = 0x00A2,  //!< Request for data about a hotbar page.
    PACKET_HOTBAR_SAVE = 0x00A4,  //!< Request to save a hotbar page.
    PACKET_EVENT_RESPONSE = 0x00B7,  //!< Message that the player has responded to the current event.
    PACKET_VALUABLE_LIST = 0x00B8,  //!< Request for a list of obtained valuables.
    PACKET_OBJECT_INTERACTION = 0x00BA,  //!< An object has been clicked on.
    PACKET_FRIEND_INFO = 0x00BD,  //!< Request for the current player's own friend information.
    PACKET_FRIEND_REQUEST = 0x00C0,  //!< Request to ask a player to be added to the player's friend list.
    PACKET_FRIEND_ADD = 0x00C3,  //!< Request to add the player and the requestor to both friend lists.
    PACKET_FRIEND_REMOVE = 0x00C4,  //!< Request to remove the player and the target from both friend lists.
    PACKET_FRIEND_DATA = 0x00C6,  //!< Request to update friend list specific data for the player.
    PACKET_PARTY_INVITE = 0x00D2,  //!< Request to invite a player to join the player's party.
    PACKET_PARTY_JOIN = 0x00D5,  //!< Request to join a party from which an invite was received.
    PACKET_PARTY_CANCEL = 0x00D8,  //!< Request to cancel a party invite.
    PACKET_PARTY_LEAVE = 0x00DA,  //!< Request to leave the player's current party.
    PACKET_PARTY_DISBAND = 0x00DD,  //!< Request to disband the current party.
    PACKET_PARTY_LEADER_UPDATE = 0x00E0,  //!< Request to update the current party's designated leader.
    PACKET_PARTY_DROP_RULE = 0x00E3,  //!< Request to update the current party's drop rule setting.
    PACKET_PARTY_MEMBER_UPDATE = 0x00E6,  //!< Response signifying whether a party member update was received correctly.
    PACKET_PARTY_KICK = 0x00EB,  //!< Request to kick a player from the current party.
    PACKET_DEMON_FUSION = 0x00EF,  //!< Request to fuse two demons into a new demon.
    PACKET_SYNC = 0x00F3,  //!< Request to retrieve the server time.
    PACKET_ROTATE = 0x00F8,  //!< Request to rotate an entity or object.
    PACKET_UNION_FLAG = 0x0100,  //!< Request to receive union information.
    PACKET_ITEM_DEPO_LIST = 0x0102,  //!< Request to list the client account's item depositories.
    PACKET_DEPO_RENT = 0x0104,  //!< Request to rent a client account item or demon depository.
    PACKET_QUEST_ACTIVE_LIST = 0x010B,  //!< Request for the player's active quest list.
    PACKET_QUEST_COMPLETED_LIST = 0x010C,  //!< Request for the player's completed quest list.
    PACKET_CLAN_DISBAND = 0x0138,   //!< Request to disband the player character's clan.
    PACKET_CLAN_INVITE = 0x013B,    //!< Request to invite a character to the player character's clan.
    PACKET_CLAN_JOIN = 0x013E,  //!< Request to accept an invite to join a clan.
    PACKET_CLAN_CANCEL = 0x0140,    //!< Request to reject an invite to join a clan.
    PACKET_CLAN_KICK = 0x0142,  //!< Request to kick a character from the player character's clan.
    PACKET_CLAN_MASTER_UPDATE = 0x0145, //!< Request to update a clan's master member.
    PACKET_CLAN_SUB_MASTER_UPDATE = 0x0148, //!< Request to toggle a player's sub-master member type.
    PACKET_CLAN_LEAVE = 0x014B, //!< Request to leave the player character's clan.
    PACKET_CLAN_CHAT = 0x014E,  //!< Request to send a chat message to the current clan's channel.
    PACKET_CLAN_INFO = 0x0150,  //!< Request for the current player character's clan information.
    PACKET_CLAN_LIST = 0x0152,  //!< Request for member information from the current player character's clan.
    PACKET_CLAN_DATA = 0x0154,  //!< Request to update clan member information about the current player character.
    PACKET_CLAN_FORM = 0x0156,  //!< Request to form a new clan from a clan formation item.
    PACKET_SYNC_CHARACTER = 0x017E,  //!< Request to sync the player character's basic information.
    PACKET_MAP_FLAG = 0x0197,  //!< Request to receive map information.
    PACKET_DEMON_COMPENDIUM = 0x019B,  //!< Request for the Demon Compendium.
    PACKET_DUNGEON_RECORDS = 0x01C4,  //!< Request for the current player's dungeon challenge records.
    PACKET_CLAN_EMBLEM_UPDATE = 0x01E1, //!< Request to update the player character's clan's emblem.
    PACKET_DEMON_FAMILIARITY = 0x01E6,  //!< Request to sync the familiarity of every demon in the player's COMP.
    PACKET_MATERIAL_BOX = 0x0205,  //!< Request for info about the materials container.
    PACKET_ANALYZE = 0x0209,  //!< Request to analyze another player character.
    PACKET_FUSION_GAUGE = 0x0217,   //!< Request for the player's fusion gauge state.
    PACKET_TITLE_LIST = 0x021B,   //!< Request for the list of available titles.
    PACKET_PARTNER_DEMON_QUEST_LIST = 0x022D,   //!< Request for the player's partner demon quest list.
    PACKET_UNSUPPORTED_0232 = 0x0232,  //!< Unknown. Requested at start up. Contains character name and client side flags.
    PACKET_LOCK_DEMON = 0x0233,  //!< Request to lock or unlock a demon in the COMP.
    PACKET_PVP_CHARACTER_INFO = 0x024D,  //!< Request for PvP character information.
    PACKET_TEAM_INFO = 0x027B,  //!< Request for the current player's team information.
    PACKET_PARTNER_DEMON_QUEST_TEMP = 0x028F,  //!< Unknown. Request for partner demon quest info.
    PACKET_RECEIVED_PLAYER_DATA = 0x028C,  //!< Empty message sent after character/demon data requested have been received.
    PACKET_RECEIVED_LISTS = 0x028E,  //!< Empty message sent after setup lists requested have been received.
    PACKET_ITEM_DEPO_REMOTE = 0x0296,  //!< Request to open the remote item depos.
    PACKET_DEMON_DEPO_REMOTE = 0x02EF,  //!< Request to open the remote demon depo.
    PACKET_COMMON_SWITCH_INFO = 0x02F4,  //!< Unknown. Request for "common switch" information.
    PACKET_CASINO_COIN_TOTAL = 0x02FA,   //!< Request for the current character's casino coin total.
    PACKET_SEARCH_ENTRY_INFO = 0x03A3,  //!< Unknown. Request for the current player's search entries.
    PACKET_HOURAI_DATA = 0x03A5,  //!< Request for Club Hourai related information.
    PACKET_CULTURE_DATA = 0x03AC,  //!< Unknown. Request for culture information.
    PACKET_DEMON_DEPO_LIST = 0x03F5,  //!< Request to list the client account's demon depositories.
    PACKET_BLACKLIST = 0x0408,  //!< Request for the current player's blacklist.
    PACKET_DIGITALIZE_POINTS = 0x0414,  //!< Request for the current player's digitalize point information.
    PACKET_DIGITALIZE_ASSIST = 0x0418,  //!< Request for the current player's digitalize assist information.
};

/**
 * Response packet code from the channel server to the game client.
 */
enum class ChannelToClientPacketCode_t : uint16_t
{
    PACKET_LOGIN = 0x0001,  //!< Login response.
    PACKET_AUTH = 0x0003,  //!< Authorization response.
    PACKET_LOGOUT = 0x0009,  //!< Logout response.
    PACKET_CHARACTER_DATA = 0x000F,  //!< Message containing data about the client's character.
    PACKET_PARTNER_DATA = 0x0010,  //!< Message containing data about the client's active demon.
    PACKET_ENEMY = 0x0011,  //!< Message containing data about an enemy in a zone.
    PACKET_NPC_DATA = 0x0014,  //!< Message containing data about an NPC in a zone.
    PACKET_OBJECT_NPC_DATA = 0x0015,  //!< Message containing data about an object NPC in a zone.
    PACKET_OTHER_CHARACTER_DATA = 0x0016,  //!< Message containing data about a different client's character.
    PACKET_OTHER_PARTNER_DATA = 0x0017,  //!< Message containing data about a different client's active demon.
    PACKET_REMOVE_OBJECT = 0x0018,  //!< Message to remove an entity or object that belongs to the client.
    PACKET_SHOW_ENTITY = 0x001A,  //!< Message to display a game entity.
    PACKET_REMOVE_ENTITY = 0x001B,  //!< Message to remove a game entity.
    PACKET_MOVE = 0x001D,  //!< Message containing entity or object movement information.
    PACKET_WARP = 0x0022,  //!< Message continaing entity or object warping information.
    PACKET_ZONE_CHANGE = 0x0023,  //!< Information about a character's zone.
    PACKET_SYNC_TIME = 0x0025,  //!< Message containing the current server time.
    PACKET_CHAT = 0x0028, //!< Response from chat request.
    PACKET_SKILL_CHARGING = 0x0033, //!< Response from skill activation request to charge a skill.
    PACKET_SKILL_COMPLETED = 0x0034, //!< Response from skill activation request to complete a skill.
    PACKET_SKILL_EXECUTING = 0x0036, //!< Response from skill activation request to execute a skill.
    PACKET_SKILL_FAILED = 0x0037,  //!< Response from skill activation request that execution has failed.
    PACKET_SKILL_REPORTS = 0x0038,  //!< Response from skill activation reporting what has been updated.
    PACKET_SKILL_SWITCH = 0x0039,   //!< Notification that a switch skill has been enabled or disabled.
    PACKET_BATTLE_STARTED = 0x003B, //!< Notifies the client that an entity has entered into a battle.
    PACKET_BATTLE_STOPPED = 0x003C, //!< Notifies the client that an entity is no longer in battle.
    PACKET_ENEMY_ACTIVATED = 0x0041, //!< Notifies the client that an enemy has become activate.
    PACKET_DO_TDAMAGE = 0x0043, //!< Notifies the client that "time" damage has been dealt to an entity.
    PACKET_ADD_STATUS_EFFECT = 0x0044, //!< Notifies the client that an entity has gained a status effect.
    PACKET_REMOVE_STATUS_EFFECT = 0x0045, //!< Notifies the client that an entity has lost a status effect.
    PACKET_XP_UPDATE = 0x0046, //!< Notifies the client of an entity's XP value.
    PACKET_CHARACTER_LEVEL_UP = 0x0047, //!< Notifies the client that a character has leveled up.
    PACKET_PARTNER_LEVEL_UP = 0x0048, //!< Notifies the client that a partner demon has leveled up.
    PACKET_ALLOCATE_SKILL_POINT = 0x004A, //!< Response from the request to allocate a skill point for a character.
    PACKET_EQUIPMENT_CHANGED = 0x004B, //!< Notifies the client that a character's equipment has changed.
    PACKET_ENTITY_STATS = 0x004C,  //!< Notifies the client of a character or parter demon's calculated stats.
    PACKET_EXPERTISE_POINT_UPDATE = 0x004D, //!< Notifies the client that a character's expertise points have been updated.
    PACKET_EXPERTISE_RANK_UP = 0x004E, //!< Notifies the client that a character's expertise has ranked up.
    PACKET_TOGGLE_EXPERTISE = 0x0050, //!< Notifies the client to enable or disable an expertise.
    PACKET_LEARN_SKILL = 0x0052, //!< Notifies the client that a character has learned a skill.
    PACKET_UPDATE_DEMON_SKILL = 0x0055, //!< Notifies the client that the partner demon's learned skill set has been updated.
    PACKET_KEEP_ALIVE = 0x0057,  //!< Response to keep the client connection alive.
    PACKET_FIX_OBJECT_POSITION = 0x0059,  //!< Response to fix a game object's position.
    PACKET_DEMON_BOX = 0x005D,  //!< Demon box list response.
    PACKET_DEMON_BOX_DATA = 0x005F,  //!< Demon box demon data response.
    PACKET_PARTNER_SUMMONED = 0x0060,  //!< Notifies the client that a partner demon has been summoned.
    PACKET_POP_ENTITY_FOR_PRODUCTION = 0x0061,  //!< Sets up an entity that will spawn in via PACKET_SHOW_ENTITY.
    PACKET_CHANNEL_LIST = 0x0064,  //!< Message containing the list of channels connected to the world.
    PACKET_REVIVE_ENTITY = 0x006E,  //!< Notification that an entity has been revived.
    PACKET_STOP_MOVEMENT = 0x0070,  //!< Message containing entity or object movement stopping information.
    PACKET_WORLD_TIME = 0x0073,  //!< Response for the server's current world time.
    PACKET_ITEM_BOX = 0x0075,  //!< Response for info about a specific item box.
    PACKET_ITEM_UPDATE = 0x007A,  //!< Message containing updated information about an item in a box.
    PACKET_EQUIPMENT_LIST = 0x007C,  //!< Response for equipment information.
    PACKET_TRADE_REQUEST = 0x007E,  //!< Response for starting a trade with another player.
    PACKET_TRADE_REQUESTED = 0x007F,  //!< Notification to another player that a trade is being requested.
    PACKET_TRADE_ACCEPT = 0x0081,  //!< Response for accepting the trade request from another player.
    PACKET_TRADE_ADD_ITEM = 0x0083,  //!< Response for adding an item to the current trade offer.
    PACKET_TRADE_ADDED_ITEM = 0x0084,  //!< Notification than an item has been added to the current trade offer.
    PACKET_TRADE_LOCK = 0x0086,  //!< Response for locking the current trade for acceptance.
    PACKET_TRADE_LOCKED = 0x0087,  //!< Notification that the other player has locked their current trade offer.
    PACKET_TRADE_FINISH = 0x0089,  //!< Response for finishing the current trade.
    PACKET_TRADE_FINISHED = 0x008A,  //!< Notification that the current trade has finished.
    PACKET_TRADE_ENDED = 0x008C,  //!< Notification that the current trade has ended and wheter it was a success of failure.
    PACKET_CASH_BALANCE = 0x0091,  //!< Response containing the current account's total CP.
    PACKET_SHOP_DATA = 0x0093,  //!< Response containing details about an NPC shop.
    PACKET_SHOP_BUY = 0x0095,  //!< Response to the request to buy an item from a shop.
    PACKET_SHOP_SELL = 0x0097,  //!< Response to the request to sell an item to a shop.
    PACKET_DEMON_BOX_UPDATE = 0x0098,  //!< Message containing information that one or more demon box has been updated.
    PACKET_POST_LIST = 0x009D, //!< Response to the request to list items in the player account's Post.
    PACKET_POST_ITEM = 0x009F, //!< Response to the request to move an item from the Post into the current character's inventory.
    PACKET_HOTBAR_DATA = 0x00A3,  //!< Response for data about a hotbar page.
    PACKET_HOTBAR_SAVE = 0x00A5,  //!< Response to save a hotbar page.
    PACKET_EVENT_MESSAGE = 0x00A6,  //!< Request to the client to display an event message.
    PACKET_EVENT_NPC_MESSAGE = 0x00A7,  //!< Request to the client to display an NPC event message.
    PACKET_EVENT_PROMPT = 0x00AC,  //!< Request to the client to display choices to the player.
    PACKET_EVENT_PLAY_SCENE = 0x00AD,  //!< Request to the client to play an in-game cinematic.
    PACKET_EVENT_OPEN_MENU = 0x00AE,  //!< Request to the client to open a menu.
    PACKET_VALUABLE_LIST = 0x00B9,  //!< Response containing a list of obtained valuables.
    PACKET_EVENT_END = 0x00BB,  //!< Request to the client to end the current event.
    PACKET_NPC_STATE_CHANGE = 0x00BC,  //!< Notification that an object NPC's state has changed.
    PACKET_FRIEND_INFO_SELF = 0x00BE,  //!< Response containing the current player's own friend information.
    PACKET_FRIEND_INFO = 0x00BF,  //!< Message containing information about a friend on the friends list.
    PACKET_FRIEND_REQUEST = 0x00C1,    //!< Response from a player to be added to the player's friend list.
    PACKET_FRIEND_REQUESTED = 0x00C2,  //!< Notification that a friend request has been received.
    PACKET_FRIEND_ADD_REMOVE = 0x00C5,  //!< Response to the client stating that a friend was added or removed.
    PACKET_FRIEND_DATA = 0x00C7,  //!< Notification to update friend list specific data from a player.
    PACKET_PARTY_INVITE = 0x00D3,  //!< Response to the request to invite a player to join the player's party.
    PACKET_PARTY_INVITED = 0x00D4,  //!< Notification that a party invite has been received.
    PACKET_PARTY_JOIN = 0x00D6,  //!< Response to the request to join a party.
    PACKET_PARTY_MEMBER_INFO = 0x00D7,  //!< Message containing information about a party member.
    PACKET_PARTY_CANCEL = 0x00D9,  //!< Response to the request to cancel a party invite.
    PACKET_PARTY_LEAVE = 0x00DB,  //!< Response to the request to leave the player's current party.
    PACKET_PARTY_LEFT = 0x00DC,  //!< Notification that a player has left the current party.
    PACKET_PARTY_DISBAND = 0x00DE,  //!< Response to the request to disband the current party.
    PACKET_PARTY_DISBANDED = 0x00DF,  //!< Notification that the current party has been disbanded.
    PACKET_PARTY_LEADER_UPDATE = 0x00E1,  //!< Response to the request to update the current party's designated leader.
    PACKET_PARTY_LEADER_UPDATED = 0x00E2,  //!< Notification that the current party's designated leader has changed.
    PACKET_PARTY_DROP_RULE = 0x00E4,  //!< Response to the request to update the current party's drop rule setting.
    PACKET_PARTY_DROP_RULE_SET = 0x00E5,  //!< Notification that the current party's drop rule has been changed.
    PACKET_PARTY_MEMBER_UPDATE = 0x00E7,  //!< Notification containing a player character's info in the current party.
    PACKET_PARTY_MEMBER_PARTNER = 0x00E8,  //!< Notification containing a parter demon's info in the current party.
    PACKET_PARTY_MEMBER_ZONE = 0x00E9,  //!< Notification that a current party member has moved to a different zone.
    PACKET_PARTY_MEMBER_ICON = 0x00EA,  //!< Notification that a current party member's icon state has changed.
    PACKET_DEMON_FUSION = 0x00F0,  //!< Response containing the results of a two-way fusion.
    PACKET_PARTY_KICK = 0x00EC,  //!< Notification that a player has been kicked from the current party.
    PACKET_SYNC = 0x00F4,  //!< Response containing the server time.
    PACKET_ROTATE = 0x00F9,    //!< Message containing entity or object rotation information.
    PACKET_RUN_SPEED = 0x00FB,    //!< Message containing an entity's updated running speed.
    PACKET_UNION_FLAG = 0x0101,  //!< Message containing union information.
    PACKET_ITEM_DEPO_LIST = 0x0103,  //!< Response containing the list of the client account's item depositories.
    PACKET_DEPO_RENT = 0x0105,  //!< Response to renting a client account item or demon depository.
    PACKET_EVENT_HOMEPOINT_UPDATE = 0x0106,  //!< Message indicating that the player's homepoint has been updated.
    PACKET_QUEST_ACTIVE_LIST = 0x010D,  //!< Response containing the player's active quest list.
    PACKET_QUEST_COMPLETED_LIST = 0x010E,  //!< Response containing the player's completed quest list.
    PACKET_QUEST_PHASE_UPDATE = 0x010F,    //!< Notification that a quest's phase has been updated.
    PACKET_QUEST_KILL_COUNT_UPDATE = 0x0110,    //!< Notification that a quest's kill counts have been updated.
    PACKET_LNC_POINTS = 0x0126,    //!< Message containing a character's LNC alignment value.
    PACKET_EVENT_STAGE_EFFECT = 0x0127,  //!< Request to the client to render a stage effect.
    PACKET_CLAN_FORM = 0x0137,   //!< Response to the request to form a new clan from a clan formation item.
    PACKET_CLAN_DISBAND = 0x0139,  //!< Response to the request to disband the player character's clan.
    PACKET_CLAN_DISBANDED = 0x013A,    //!< Notification that the player character's clan has disbanded.
    PACKET_CLAN_INVITE = 0x013C,   //!< Response to the request to invite a character to the player character's clan.
    PACKET_CLAN_INVITED = 0x013D,  //!< Notification that the player character has been invited to join a clan.
    PACKET_CLAN_JOIN = 0x013F, //!< Response to the request to accept an invite to join a clan.
    PACKET_CLAN_CANCEL = 0x0141,   //!< Response to the request to reject an invite to join a clan.
    PACKET_CLAN_KICK = 0x0143, //!< Response to the request to kick a character from the player character's clan.
    PACKET_CLAN_KICKED = 0x0144,   //!< Notification that a character has been kicked from the player character's clan.
    PACKET_CLAN_MASTER_UPDATE = 0x0146,    //!< Response to the request to update a clan's master member.
    PACKET_CLAN_MASTER_UPDATED = 0x0147,   //!< Notification that the player character's clan's master has changed.
    PACKET_CLAN_SUB_MASTER_UPDATE = 0x0149,    //!< Response to the request to toggle a player's sub-master member type.
    PACKET_CLAN_SUB_MASTER_UPDATED = 0x014A,   //!< Notification that a clan member has had their sub-master role toggled.
    PACKET_CLAN_LEAVE = 0x014C,    //!< Response to the request to leave the player character's clan.
    PACKET_CLAN_LEFT = 0x014D, //!< Notification that a member of the player character's clan has left.
    PACKET_CLAN_CHAT = 0x014F, //!< Chat message sent from a member of the same clan.
    PACKET_CLAN_INFO = 0x0151,  //!< Response containing the current player's clan information.
    PACKET_CLAN_LIST = 0x0153, //!< Response to the request for member information from the current player character's clan.
    PACKET_CLAN_DATA = 0x0155, //!< Notification that a player character's clan's member info has updated
    PACKET_EVENT_DIRECTION = 0x015D,  //!< Request to the client to signify a direction to the player.
    PACKET_CLAN_NAME_UPDATED = 0x0169,  //!< Notification that a character's clan name has updated.
    PACKET_SYSTEM_MSG = 0x0171, //!< Message containing announcement ticker data. 
    PACKET_SYNC_CHARACTER = 0x017F,  //!< Response to the request to sync the player character's basic information.
    PACKET_BAZAAR_DATA = 0x0183,  //!< Message containing data about a bazaar in a zone.
    PACKET_SKILL_LIST_UPDATED = 0x0187, //!< Notification that a player's skill list has been updated.
    PACKET_STATUS_ICON = 0x0195,  //!< Message containing the icon to show for the client's character.
    PACKET_STATUS_ICON_OTHER = 0x0196,  //!< Message containing the icon to show for another player's character.
    PACKET_MAP_FLAG = 0x0198,  //!< Message containing map information.
    PACKET_DEMON_COMPENDIUM = 0x019C,  //!< Response containing the Demon Compendium.
    PACKET_DEMON_FAMILIARITY_UPDATE = 0x01A5,  //!< Notification that the current partner demon's familiarity has updated.
    PACKET_DUNGEON_CHALLENGES = 0x01C5,  //!< Response containing the current player's dungeon challenge records.
    PACKET_CLAN_EMBLEM_UPDATE = 0x01E2,    //!< Response to the request to update the player character's clan's emblem.
    PACKET_CLAN_EMBLEM_UPDATED = 0x01E3,   //!< Notification that a character's clan emblem has updated.
    PACKET_CLAN_LEVEL_UPDATED = 0x01E4,    //!< Notification that a character's clan level has updated.
    PACKET_CLAN_UPDATE = 0x01E5,   //!< Notification to a character that their visible clan info has been updated.
    PACKET_DEMON_FAMILIARITY = 0x01E7,  //!< Response to the request for the familiarity values of each demon in the COMP.
    PACKET_EVENT_EX_NPC_MESSAGE = 0x01E9,  //!< Request to the client to display an extended NPC event message.
    PACKET_EVENT_PLAY_SOUND_EFFECT = 0x01FB,  //!< Request to the client to play a sound effect as part of an event.
    PACKET_MATERIAL_BOX = 0x0206,  //!< Response containing info about the materials container.
    PACKET_EQUIPMENT_ANALYZE = 0x020A, //!< Message containing another player character's current equipment for "analyze".
    PACKET_OTHER_CHARACTER_EQUIPMENT_CHANGED = 0x020B, //!< Notifies the client that another character's equipment has changed.
    PACKET_EVENT_SPECIAL_DIRECTION = 0x0216,  //!< Request to the client to signify a special direction to the player.
    PACKET_FUSION_GAUGE = 0x0218,   //!< Response containing the player's fusion gauge state.
    PACKET_TITLE_LIST = 0x021C,   //!< Response containing the list of available titles.
    PACKET_PARTNER_DEMON_QUEST_LIST = 0x022E,   //!< Response containing the player's partner demon quest list.
    PACKET_LOCK_DEMON = 0x0234,  //!< Response to lock a demon in the COMP.
    PACKET_PVP_CHARACTER_INFO = 0x024E,  //!< Response containing PvP character information.
    PACKET_TEAM_INFO = 0x027C,  //!< Response containing the current player's team information.
    PACKET_EVENT_MULTITALK = 0x028D,  //!< Request to the client to start a multitalk event.
    PACKET_PARTNER_DEMON_QUEST_TEMP = 0x0290,  //!< Unknown. Response containing partner demon quest info.
    PACKET_EVENT_GET_ITEMS = 0x0291,  //!< Request to the client to inform the player that a items have been obtained.
    PACKET_EVENT_PLAY_BGM = 0x0294,   //!< Request to the client to play background music as part of an event.
    PACKET_EVENT_STOP_BGM = 0x0295,   //!< Request to the client to stop playing specific background music as part of an event.
    PACKET_ITEM_DEPO_REMOTE = 0x0297,  //!< Response to the request to open the remote item depos.
    PACKET_DEMON_DEPO_REMOTE = 0x02F0,  //!< Response to the request to open the remote demon depos.
    PACKET_COMMON_SWITCH_INFO = 0x02F5,  //!< Unknown. Response containing "common switch" information.
    PACKET_CASINO_COIN_TOTAL = 0x02FB,   //!< Message containing the current character's casino coin total.
    PACKET_SEARCH_ENTRY_INFO = 0x03A4,  //!< Unknown. Response containing the current player's search entries.
    PACKET_HOURAI_DATA = 0x03A6,  //!< Response containing Club Hourai related information.
    PACKET_CULTURE_DATA = 0x03AD,  //!< Unknown. Response containing culture information.
    PACKET_DEMON_DEPO_LIST = 0x03F6,  //!< Response to the request to open the demon depo.
    PACKET_BLACKLIST = 0x0409,  //!< Response containing the current player's blacklist.
    PACKET_DIGITALIZE_POINTS = 0x0415,  //!< Response containing the current player's digitalize point information.
    PACKET_DIGITALIZE_ASSIST = 0x0419,  //!< Response containing the current player's digitalize assist information.
};

/**
 * Client specified set of flags added to the log out packet to signify the expected action to take.
 */
enum class LogoutPacketAction_t : uint32_t
{
    LOGOUT_PREPARE = 10, //!< Prepare to log out, printing a 10 second countdown if not sent a different code shortly after.
    LOGOUT_CANCEL = 11, //!< Logout cancel confirmation.
    LOGOUT_DISCONNECT = 13, //!< Close the connection resulting in a generic disconnect message if sent not following a prepare.
    LOGOUT_CHANNEL_SWITCH = 14, //!< Log out of the current channel and move to the one specified in the packet.
};

/**
 * Request or response packet code between two internal servers.
 */
enum class InternalPacketCode_t : uint16_t
{
    PACKET_GET_WORLD_INFO = 0x1001, //!< Request to detail the world server.
    PACKET_SET_WORLD_INFO = 0x1002, //!< Request to update a non-world server's world information.
    PACKET_SET_CHANNEL_INFO = 0x1003,   //!< Request to update a server's channel information.
    PACKET_ACCOUNT_LOGIN = 0x1004,  //!< Pass login information between the servers.
    PACKET_ACCOUNT_LOGOUT = 0x1005, //!< Pass logout information between the servers.
    PACKET_RELAY = 0x1006,  //!< Relay a channel to client message from one player to another between servers.
    PACKET_CHARACTER_LOGIN = 0x1007,    //!< Pass character login information between the servers.
    PACKET_FRIENDS_UPDATE = 0x1008, //!< Pass friend information between the servers.
    PACKET_PARTY_UPDATE = 0x1009,   //!< Pass party information between the servers.
    PACKET_CLAN_UPDATE = 0x100A,    //!< Pass clan information between the servers.
};

/**
 * Mode signifier that changes how a packet should be handled.
 */
enum class InternalPacketAction_t : uint8_t
{
    PACKET_ACTION_ADD = 1,  //!< Indicates a contextual addition.
    PACKET_ACTION_UPDATE,   //!< Indicates a contextual update.
    PACKET_ACTION_REMOVE,   //!< Indicates a contextual removal.
    PACKET_ACTION_YN_REQUEST,   //!< Indicates a contextual Yes/No request.
    PACKET_ACTION_RESPONSE_YES, //!< Indicates a contextual "Yes" response.
    PACKET_ACTION_RESPONSE_NO,  //!< Indicates a contextual "No" response.

    PACKET_ACTION_GROUP_LIST,   //!< Indicates that a contextual group list is being requested or sent.
    PACKET_ACTION_GROUP_LEAVE,  //!< Indicates that a contextual group update consists of "leave" information.
    PACKET_ACTION_GROUP_DISBAND,    //!< Indicates that a contextual group update consists of "disband" information.
    PACKET_ACTION_GROUP_LEADER_UPDATE,  //!< Indicates that a contextual group update consists of "leader update" information.
    PACKET_ACTION_GROUP_KICK,   //!< Indicates that a contextual group update consists of "kick" information.

    PACKET_ACTION_CLAN_EMBLEM_UPDATE,  //!< Indicates that a clan update consists of "emblem update" information.
    PACKET_ACTION_PARTY_DROP_RULE,  //!< Indicates that a party update consists of "drop rule" information.
};

/**
 * Specifies how the message should be relayed. All options are valid when communicating with the world
 * server and only the failure or world CIDs options are used when the world relays the packet to a channel.
 */
enum class PacketRelayMode_t : uint8_t
{
    RELAY_FAILURE = 0,  //!< Response back to the world containing CIDs that were not in the channel to attempt a retry.
    RELAY_CHARACTER,  //!< Request to relay a message to a character by name.
    RELAY_CIDS,  //!< Request to relay a message to one or more characters by world CID.
    RELAY_PARTY,  //!< Request to relay a message to all current members of a party.
    RELAY_CLAN,  //!< Request to relay a message to all online clan members.
    RELAY_TEAM,  //!< Request to relay a message to all current team members.
};

/**
 * Flags used to indicate the type of information being sent in a PACKET_CHARACTER_LOGIN message.  Data
 * signified to have been added to the packet should be written from lowest to highest value listed here.
 */
enum class CharacterLoginStateFlag_t : uint8_t
{
    CHARLOGIN_STATUS = 0x01,  //!< Indicates that a player's active status is contained in a packet.
    CHARLOGIN_ZONE = 0x02,   //!< Indicates that a character's zone is contained in a packet.
    CHARLOGIN_CHANNEL = 0x04,  //!< Indicates that a character's channel is contained in a packet.
    CHARLOGIN_BASIC = 0x07,  //!< Indicates that basic information about a character is contained in a packet.
    CHARLOGIN_MESSAGE = 0x08,  //!< Indicates that a character's friend or clan message is contained in a packet.
    CHARLOGIN_FRIEND_UNKNOWN = 0x10,  //!< Unknown. Indicates that some friend information is being sent.
    CHARLOGIN_FRIEND_FLAGS = 0x1F,  //!< Indicates that one or many friend related pieces of data are contained in a packet.
    CHARLOGIN_PARTY_INFO = 0x20,  //!< Indicates that player character party information is contained in a packet.
    CHARLOGIN_PARTY_DEMON_INFO = 0x40,  //!< Indicates that partner demon party information is contained in a packet.
    CHARLOGIN_PARTY_ICON = 0x80,  //!< Indicates that party icon information is contained in a packet.
    CHARLOGIN_PARTY_FLAGS = 0xE2,  //!< Indicates that one or many party related pieces of data are contained in a packet.
};

/**
 * Casting utility to maintain an enum type when passed as a parameter.
 */
template<typename T>
constexpr typename std::underlying_type<T>::type to_underlying(T val)
{
    return static_cast<typename std::underlying_type<T>::type>(val);
}

#endif // LIBCOMP_SRC_PACKETCODES_H
