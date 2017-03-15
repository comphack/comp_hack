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
    PACKET_ACTIVATE_SKILL = 0x0030, //!< Request to activate a player or demon skill.
    PACKET_EXECUTE_SKILL = 0x0031, //!< Request to execute a skill that has finished charging.
    PACKET_CANCEL_SKILL = 0x0032, //!< Request to execute a skill that has finished charging.
    PACKET_ALLOCATE_SKILL_POINT = 0x0049, //!< Request to allocate a skill point for a character.
    PACKET_TOGGLE_EXPERTISE = 0x004F, //!< Request to enable or disable an expertise.
    PACKET_LEARN_SKILL = 0x0051, //!< Request for a character to learn a skill.
    PACKET_KEEP_ALIVE = 0x0056,  //!< Request/check to keep the connection alive.
    PACKET_FIX_OBJECT_POSITION = 0x0058,  //!< Request to fix a game object's position.
    PACKET_STATE = 0x005A,  //!< Request for the client's character state.
    PACKET_PARTNER_DEMON_DATA = 0x005B,  //!< Request for the client's active demon state.
    PACKET_COMP_LIST = 0x005C,  //!< COMP demon list request.
    PACKET_COMP_DEMON_DATA = 0x005E,  //!< COMP demon data request.
    PACKET_STOP_MOVEMENT = 0x006F,  //!< Request to stop the movement of an entity or object.
    PACKET_ITEM_BOX = 0x0074,  //!< Request for info about a specific item box.
    PACKET_ITEM_MOVE = 0x0076,  //!< Request to move an item in an item box.
    PACKET_ITEM_DROP = 0x0077,  //!< Request to throw away an item from an item box.
    PACKET_ITEM_STACK = 0x0078,  //!< Request to stack or split stacked items in an item box.
    PACKET_EQUIPMENT_LIST = 0x007B,  //!< Request for equipment information.
    PACKET_COMP_SLOT_UPDATE = 0x009A,  //!< Request to update a COMP slot.
    PACKET_DISMISS_DEMON = 0x009B,  //!< Request to dismiss a demon.
    PACKET_HOTBAR_DATA = 0x00A2,  //!< Request for data about a hotbar page.
    PACKET_HOTBAR_SAVE = 0x00A4,  //!< Request to save a hotbar page.
    PACKET_VALUABLE_LIST = 0x00B8,  //!< Request for a list of obtained valuables.
    PACKET_SYNC = 0x00F3,  //!< Request to retrieve the server time.
    PACKET_ROTATE = 0x00F8,  //!< Request to rotate an entity or object.
    PACKET_UNION_FLAG = 0x0100,  //!< Request to receive uniion information.
    PACKET_LOCK_DEMON = 0x0233,  //!< Request to lock or unlock a demon in the COMP.
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
    PACKET_NPC_DATA = 0x0014,  //!< Message containing data about an NPC in a zone.
    PACKET_OBJECT_NPC_DATA = 0x0015,  //!< Message containing data about an object NPC in a zone.
    PACKET_OTHER_CHARACTER_DATA = 0x0016,  //!< Message containing data about a different client's character.
    PACKET_OTHER_PARTNER_DATA = 0x0017,  //!< Message containing data about a different client's active demon.
    PACKET_REMOVE_OBJECT = 0x0018,  //!< Message to remove an entity or object that belongs to the client.
    PACKET_SHOW_ENTITY = 0x001A,  //!< Message to display a game entity.
    PACKET_REMOVE_ENTITY = 0x001B,  //!< Message to remove a game entity.
    PACKET_MOVE = 0x001D,  //!< Message containing entity or object movement information.
    PACKET_ZONE_CHANGE = 0x0023,  //!< Information about a character's zone.
    PACKET_CHAT = 0x0028, //!< Response from chat request.
    PACKET_SKILL_CHARGING = 0x0033, //!< Response from skill activation request to charge a skill.
    PACKET_SKILL_COMPLETED = 0x0034, //!< Response from skill activation request to complete a skill.
    PACKET_SKILL_EXECUTING = 0x0036, //!< Response from skill activation request to execute a skill.
    PACKET_SKILL_FAILED = 0x0037,  //!< Response from skill activation request that execution has failed.
    PACKET_SKILL_REPORTS = 0x0038,  //!< Response from skill activation reporting what has been updated.
    PACKET_XP_UPDATE = 0x0046, //!< Notifies the client of an entity's XP value.
    PACKET_CHARACTER_LEVEL_UP = 0x0047, //!< Notifies the client that a character has leveled up.
    PACKET_PARTNER_LEVEL_UP = 0x0048, //!< Notifies the client that a partner demon has leveled up.
    PACKET_ALLOCATE_SKILL_POINT = 0x004A, //!< Response from the request to allocate a skill point for a character.
    PACKET_EQUIPMENT_CHANGED = 0x004B, //!< Notifies the client that a character's equipment has changed.
    PACKET_EXPERTISE_POINT_UPDATE = 0x004D, //!< Notifies the client that a character's expertise points have been updated.
    PACKET_EXPERTISE_RANK_UP = 0x004E, //!< Notifies the client that a character's expertise has ranked up.
    PACKET_TOGGLE_EXPERTISE = 0x0050, //!< Notifies the client to enable or disable an expertise.
    PACKET_LEARN_SKILL = 0x0052, //!< Notifies the client that a character has learned a skill.
    PACKET_KEEP_ALIVE = 0x0057,  //!< Response to keep the client connection alive.
    PACKET_FIX_OBJECT_POSITION = 0x0059,  //!< Response to fix a game object's position.
    PACKET_COMP_LIST = 0x005D,  //!< COMP demon list response.
    PACKET_COMP_DEMON_DATA = 0x005F,  //!< COMP demon data response.
    PACKET_PARTNER_SUMMONED = 0x0060,  //!< Notifies the client that a partner demon has been summoned.
    PACKET_STOP_MOVEMENT = 0x0070,  //!< Message containing entity or object movement stopping information.
    PACKET_ITEM_BOX = 0x0075,  //!< Response for info about a specific item box.
    PACKET_ITEM_UPDATE = 0x007A,  //!< Message containing updated information about an item in a box.
    PACKET_EQUIPMENT_LIST = 0x007C,  //!< Response for equipment information.
    PACKET_COMP_SLOT_UPDATED = 0x0098,  //!< Message containing information that a COMP slot has been updated.
    PACKET_HOTBAR_DATA = 0x00A3,  //!< Response for data about a hotbar page.
    PACKET_HOTBAR_SAVE = 0x00A5,  //!< Response to save a hotbar page.
    PACKET_VALUABLE_LIST = 0x00B9,  //!< Response containing a list of obtained valuables.
    PACKET_SYNC = 0x00F4,  //!< Response containing the server time.
    PACKET_ROTATE = 0x00F9,    //!< Message containing entity or object rotation information.
    PACKET_UNION_FLAG = 0x0101,  //!< Message containing union information.
    PACKET_LNC_POINTS = 0x0126,    //!< Message containing a character's LNC alignment value.
    PACKET_STATUS_ICON = 0x0195,  //!< Message containing the icon to show for a character.
    PACKET_LOCK_DEMON = 0x0234,  //!< Response to lock a demon in the COMP.

    PACKET_CONFIRMATION = 0x1FFF,  //!< Generic confirmation response.
};

/**
 * Request or response packet code between two internal servers.
 */
enum class InternalPacketCode_t : uint16_t
{
    PACKET_GET_WORLD_INFO = 0x1001, //!< Request to detail the world server.
    PACKET_SET_WORLD_INFO = 0x1002,  //!< Request to update a non-world server's world information.
    PACKET_SET_CHANNEL_INFO = 0x1003,    //!< Request to update a non-channel server's channel information.
    PACKET_ACCOUNT_LOGIN = 0x1004,    //!< Pass login information between the servers.
    PACKET_ACCOUNT_LOGOUT = 0x1005,    //!< Pass logout information between the servers.
};

/**
 * Mode signifier that changes how a packet should be handled.
 */
enum class InternalPacketAction_t : uint8_t
{
    PACKET_ACTION_ADD = 1,  //!< Packet action is an addition.
    PACKET_ACTION_UPDATE,   //!< Packet action is an update.
    PACKET_ACTION_REMOVE,   //!< Pacekt action is a removal.
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
