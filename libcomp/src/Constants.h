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

/// Maximum length of a chat message.
#define MAX_MESSAGE_LENGTH (80)

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

// Chat Radius for Say Chat.  ToDo: Define Proper Range.
#define CHAT_RADIUS_SAY (92)

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

/// Experience required to proceed from the indexed level to the next
const unsigned long long LEVEL_XP_REQUIREMENTS[] = {
    0ULL,               // 0->1
    40ULL,              // 1->2
    180ULL,             // 2->3
    480ULL,             // 3->4
    1100ULL,            // 4->5
    2400ULL,            // 5->6
    4120ULL,            // 6->7
    6220ULL,            // 7->8
    9850ULL,            // 8->9
    14690ULL,           // 9->10
    20080ULL,           // 10->11
    25580ULL,           // 11->12
    33180ULL,           // 12->13
    41830ULL,           // 13->14
    50750ULL,           // 14->15
    63040ULL,           // 15->16
    79130ULL,           // 16->17
    99520ULL,           // 17->18
    129780ULL,          // 18->19
    159920ULL,          // 19->20
    189800ULL,          // 20->21
    222600ULL,          // 21->22
    272800ULL,          // 22->23
    354200ULL,          // 23->24
    470400ULL,          // 24->25
    625000ULL,          // 25->26
    821600ULL,          // 26->27
    1063800ULL,         // 27->28
    1355200ULL,         // 28->29
    1699400ULL,         // 29->30
    840000ULL,          // 30->31
    899000ULL,          // 31->32
    1024000ULL,         // 32->33
    1221000ULL,         // 33->34
    1496000ULL,         // 34->35
    1855000ULL,         // 35->36
    2304000ULL,         // 36->37
    2849000ULL,         // 37->38
    3496000ULL,         // 38->39
    4251000ULL,         // 39->40
    2160000ULL,         // 40->41
    2255000ULL,         // 41->42
    2436000ULL,         // 42->43
    2709000ULL,         // 43->44
    3080000ULL,         // 44->45
    3452000ULL,         // 45->46
    4127000ULL,         // 46->47
    5072000ULL,         // 47->48
    6241000ULL,         // 48->49
    7640000ULL,         // 49->50
    4115000ULL,         // 50->51
    4401000ULL,         // 51->52
    4803000ULL,         // 52->53
    5353000ULL,         // 53->54
    6015000ULL,         // 54->55
    6892000ULL,         // 55->56
    7900000ULL,         // 56->57
    9308000ULL,         // 57->58
    11220000ULL,        // 58->59
    14057000ULL,        // 59->60
    8122000ULL,         // 60->61
    8538000ULL,         // 61->62
    9247000ULL,         // 62->63
    10101000ULL,        // 63->64
    11203000ULL,        // 64->65
    12400000ULL,        // 65->66
    14382000ULL,        // 66->67
    17194000ULL,        // 67->68
    20444000ULL,        // 68->69
    25600000ULL,        // 69->70
    21400314ULL,        // 70->71
    23239696ULL,        // 71->72
    24691100ULL,        // 72->73
    27213000ULL,        // 73->74
    31415926ULL,        // 74->75
    37564000ULL,        // 75->76
    46490000ULL,        // 76->77
    55500000ULL,        // 77->78
    66600000ULL,        // 78->79
    78783200ULL,        // 79->80
    76300000ULL,        // 80->81
    78364000ULL,        // 81->82
    81310000ULL,        // 82->83
    85100000ULL,        // 83->84
    89290000ULL,        // 84->85
    97400000ULL,        // 85->86
    110050000ULL,       // 86->87
    162000000ULL,       // 87->98
    264000000ULL,       // 88->89
    354000000ULL,       // 89->90
    696409989ULL,       // 90->91
    1392819977ULL,      // 91->92
    2089229966ULL,      // 92->93
    2100000000ULL,      // 93->94
    2110000000ULL,      // 94->95
    10477689898ULL,     // 95->96
    41910759592ULL,     // 96->97
    125732278776ULL,    // 97->98
    565795254492ULL,    // 98->99
};

} // namespace libcomp

#endif // LIBCOMP_SRC_CONSTANTS_H
