/**
 * @file libcomp/src/Constants.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Constant values used throughout the applications.
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

#ifndef LIBCOMP_SRC_CONSTANTS_H
#define LIBCOMP_SRC_CONSTANTS_H

namespace libcomp
{

/// Number of bits in a Blowfish key.
#define BF_NET_KEY_BIT_SIZE (64)

/// Number of bytes in a Blowfish key.
#define BF_NET_KEY_BYTE_SIZE (BF_NET_KEY_BIT_SIZE / 8)

/// Number of bits in a Diffie-Hellman key.
#define DH_KEY_BIT_SIZE (1024)

/// Number of hex digits for a Diffie-Hellman key.
#define DH_KEY_HEX_SIZE (DH_KEY_BIT_SIZE / 4)

/// Number of bytes in the Diffie-Hellman share data.
#define DH_SHARED_DATA_SIZE (DH_KEY_BIT_SIZE / 8)

/// Base "g" for a Diffie-Hellman key exchange (int format).
#define DH_BASE_INT 2

/// Base "g" for a Diffie-Hellman key exchange (string format).
#define DH_BASE_STRING "2"

/// Size of the stack that is used to talk to a Squirrel VM.
#define SQUIRREL_STACK_SIZE (1024)

/// Maximum number of bytes in a packet.
#define MAX_PACKET_SIZE (16384)

/// Maximum number of calls to trace when generating the backtrace.
#define MAX_BACKTRACE_DEPTH (100)

/// Maximum number of clients that can be connected at a given time.
#define MAX_CLIENT_CONNECTIONS (4096)

/// Maximum supported character level.
#define MAX_LEVEL (99)

/// Number of expertise.
#define EXPERTISE_COUNT (38)

/// Time until the client is expected to timeout and disconnect (in seconds).
#define TIMEOUT_CLIENT (15)

/// Time until the server should close the socket (in seconds).
/// It is the assumed the client has reached @ref TIMEOUT_CLIENT and due to
/// network issues is not able to confirm socket closure on their end.
#define TIMEOUT_SOCKET (17)

/// Chat message is only visible by the person who sent it.
#define CHAT_VISIBILITY_SELF   (0)

/// Chat message is only visible to characters near the sender.
#define CHAT_VISIBILITY_RANGE  (1)

/// Chat message is only visible to characters in the same zone.
#define CHAT_VISIBILITY_ZONE   (2)

/// Chat message is only visible to characters in the same party.
#define CHAT_VISIBILITY_PARTY  (3)

/// Chat message is only visible to characters in the same clan.
#define CHAT_VISIBILITY_KLAN   (4)

/// Chat message is only visible to characters on the same PvP team.
#define CHAT_VISIBILITY_TEAM   (5)

/// Chat message is visible to all characters.
#define CHAT_VISIBILITY_GLOBAL (6)

/// Chat message is visible to all GMs.
#define CHAT_VISIBILITY_GMS    (7)

/// Number of G1 times stored.
#define G1_TIME_COUNT (18)

/// Number of friends that can be registered in the friend list.
#define FRIEND_COUNT (100)

/// Number of valuable mask bytes to send to the client.
#define VALUABLE_MASK_COUNT (64)

/// Max number of valuable mask bytes stored in the character data.
#define VALUABLE_MASK_MAX   (64)

/// Number of quest mask bytes to send to the client.
#define QUEST_MASK_COUNT (128)

/// Max number of quest mask bytes stored in the character data.
#define QUEST_MASK_MAX   (512)

/// Highest possible account ID (lower to preserve the PC registry IDs).
#define MAX_ACCOUNT_ID (0x07FFFFFF)

/// Maximum number of characters for an account.
#define MAX_CHARACTER (20)

/// How many item storage boxes are avaliable via the user interface.
#define ITEM_BOX_COUNT (10)

/// The skill activation contains no extra information.
#define ACTIVATION_NOTARGET (0)

/// The skill activation contains a target UID.
#define ACTIVATION_TARGET   (1)

/// The skill activation contains a demon UID.
#define ACTIVATION_DEMON    (2)

/// The skill activation contains an item UID.
#define ACTIVATION_ITEM     (3)

/// The maximum skill activation extra info type value.
#define ACTIVATION_MAX      (4)

/// Cost type for HP.
#define COST_TYPE_HP   (0)

/// Cost type for MP.
#define COST_TYPE_MP   (1)

/// Cost type for an item.
#define COST_TYPE_ITEM (2)

/// Maximum cost type value.
#define COST_TYPE_MAX  (3)

/// A fixed cost number.
#define COST_NUMREP_FIXED   (0)

/// A percentage cost number.
#define COST_NUMREP_PERCENT (1)

/// Maximum cost number representation value.
#define COST_NUMREP_MAX     (2)

/// Client has not attempted to login yet.
#define LOGIN_STATE_WAIT (0)

/// Client has sent a valid account name.
#define LOGIN_STATE_VALID_ACCT (1)

/// Client has authenticated.
#define LOGIN_STATE_AUTHENTICATED (2)

/// Client has sent a P0004_sendData packet.
#define LOGIN_STATE_GOT_SEND_DATA (3)

/// Character data has been loaded by the database thread.
#define LOGIN_STATE_HAVE_PCDATA (4)

/// Client sent a P0004_sendData packet and the character data has been loaded.
#define LOGIN_STATE_PENDING_REPLY (5)

/// Client has been notified of a successful login.
#define LOGIN_STATE_LOGGED_IN (6)

/// Client is attempting to log out.
#define LOGIN_STATE_PENDING_LOGOUT (7)

/// Known index values contained in the binary structure Correct tables
enum CorrectData : unsigned char
{
    CORRECT_STR = 0,
    CORRECT_MAGIC = 1,
    CORRECT_VIT = 2,
    CORRECT_INTEL = 3,
    CORRECT_SPEED = 4,
    CORRECT_LUCK = 5,
    CORRECT_MAXHP = 6,
    CORRECT_MAXMP = 7,
    CORRECT_HP_REGEN = 8,
    CORRECT_MP_REGEN = 9,

    CORRECT_MOVE_SPEED = 11,
    CORRECT_CLSR = 12,
    CORRECT_LNGR = 13,
    CORRECT_SPELL = 14,
    CORRECT_SUPPORT = 15,
    CORRECT_CRITICAL = 16,

    CORRECT_PDEF = 19,
    CORRECT_MDEF = 20,

    CORRECT_ACQUIRE_EXP = 80,
    CORRECT_ACQUIRE_MAG = 81,
    CORRECT_ACQUIRE_MACCA = 82,
    CORRECT_ACQUIRE_EXPERTISE = 83,
};

} // namespace libcomp

#endif // LIBCOMP_SRC_CONSTANTS_H
