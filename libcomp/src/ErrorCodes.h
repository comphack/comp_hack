/**
 * @file libcomp/src/ErrorCodes.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Contains enums for internal and external packet codes.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_ERRORCODES_H
#define LIBCOMP_SRC_ERRORCODES_H

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <stdint.h>

/**
 * Error codes used by the game client.
 */
enum class ErrorCodes_t : int32_t
{
    SUCCESS = 0, //!< No error
    SYSTEM_ERROR = -1, //!< System error
    PROTOCOL_ERROR = -2, //!< Protocol error
    PRAMETER_ERROR = -3, //!< Parameter error
    UNSUPPORTED_FEATURE = -4, //!< Unsupported feature
    BAD_USERNAME_PASSWORD = -5, //!< Incorrect username or password
    ACCOUNT_STILL_LOGGED_IN = -6, //!< Account still logged in
    NOT_ENOUGH_CP = -7, //!< Insufficient cash shop points
    SERVER_DOWN = -8, //!< Server currently down
    NOT_AUTHORIZED = -9, //!< Not authorized to perform action
    NEED_CHARACTER_TICKET = -10, //!< Do not have character creation ticket
    NO_EMPTY_CHARACTER_SLOTS = -11, //!< No empty character slots
    ALREADY_DID_THAT = -12, //!< You have already done that
    SERVER_FULL = -13, //!< Server is currently full
    CAN_NOT_BE_USED_YET = -14, //!< Feature can't be used yet
    TOO_MANY_CHARACTERS = -15, //!< You have too many characters
    BAD_CHARACTER_NAME = -16, //!< Can't use that character name
    SERVER_CROWDED = -17, //!< Server crowded (with popup)
    WRONG_CLIENT_VERSION = -18, //!< Wrong client version (and any gap)
    ACCOUNT_DISABLED = -26, //!< Currently can't use this account
    MUST_REAUTHORIZE_ACCOUNT = -28, //!< To log in you must re-cert (pops up login window)
    ACCOUNT_LOCKED_A = -101, //!< Account locked by antitheft function
    ACCOUNT_LOCKED_B = -102, //!< Account locked by antitheft function
    CONNECTION_TIMEOUT = -103, //!< Connection has timed out
};

libcomp::String ErrorCodeString(ErrorCodes_t error);

/**
 * Error codes used for skill failure by the game client. Most will print a message
 * in the player's chat window but they can include action requests.
 */
enum class SkillErrorCodes_t : uint8_t
{
    GENERIC = 0,    //!< Generic error has occurred, no message
    GENERIC_USE = 2,    //!< Cannot be used
    GENERIC_COST = 3,   //!< Cannot be paid for
    ACTIVATION_FAILURE = 4,   //!< Cannot be activated
    COOLING_DOWN = 5,   //!< Cool down has not completed
    ACTION_RETRY = 6,   //!< No message, request that client pursue and retry
    TOO_FAR = 8,   //!< The skill was too far away when execution was attempted
    CONDITION_RESTRICT = 10,    //!< Skill is not in a useable state
    LOCATION_RESTRICT = 11, //!< Cannot be used in the current location
    ITEM_USE = 12,  //!< Item cannot have its skill used
    SILENT_FAIL = 13, //!< Skill cannot be used but don't print an error
    TALK_INVALID = 21,  //!< Target cannot be talked to
    TALK_LEVEL = 22,    //!< Target's level is too high and cannot be talked
    TALK_WONT_LISTEN = 23,  //!< Target refuses to listen to talk skills
    TALK_INVALID_STATE = 25,    //!< Demon cannot be talked to due to its current state
    SUMMON_INVALID = 26,    //!< Demon cannot be summoned
    SUMMON_LEVEL = 28,  //!< Demon's level is too high and cannot be summoned
    TARGET_INVALID = 35,    //!< Target invalid for skill
    LNC_DIFFERENCE = 36,    //!< LNC differs
    MOUNT_ITEM_MISSING = 37,    //!< Character does not have the right mount item
    MOUNT_ITEM_DURABILITY = 38, //!< Mount iem's durability is zero
    MOUNT_SUMMON_RESTRICT = 39, //!< Attempted to summon while on mount
    MOUNT_DEMON_INVALID = 40,   //!< Partner demon cannot act as mount
    MOUNT_TOO_FAR = 41, //!< Partner mount target is too far away
    MOUNT_DEMON_CONDITION = 42, //!< Partner mount target condition is not valid
    MOUNT_OTHER_SKILL_RESTRICT = 44,    //!< Attempted to use non-mount skill while on mount
    MOUNT_MOVE_RESTRICT = 45,   //!< Cannot move so mounting not allowed
    PARTNER_MISSING = 46,   //!< No partner demon summoned
    PARTNER_FAMILIARITY = 47,   //!< Partner demon familiarity too low
    PARTNER_DEAD = 50,  //!< Partner demon is dead
    PARTNER_FAMILIARITY_ITEM = 52,  //!< Partner demon familiarity too low for item
    PARTNER_TOO_FAR = 53,   //!< Partner demon is too far away
    DEVIL_FUSION_RESTRICT = 54, //!< Devil fusion cannot be used in current location
    MOOCH_PARTNER_MISSING = 55, //!< No partner demon summoned so mooch cannot be used
    MOOCH_PARTNER_FAMILIARITY = 56, //!< Partner demon familiarity too low for mooch
    MOOCH_PARTNER_DEAD = 59,    //!< Partner demon is dead so mooch cannot be used
    MOOCH_PARTNER_TOO_FAR = 60, //!< Partner demon is too far away for mooch
    INVENTORY_SPACE_PRESENT = 61,   //!< Inventory space needed to recieve demon present
    INVENTORY_SPACE = 63,   //!< Inventory space needed to receive item
    NOTHING_HAPPENED_NOW = 68,   //!< Nothing happened currently
    NOTHING_HAPPENED_HERE = 69,  //!< Nothing happened in the current place
    TIME_RESTRICT = 71, //!< Time invalid for use
    ZONE_INVALID = 72,  //!< Target zone is not valid
    PARTNER_INCOMPATIBLE = 75,  //!< Partner demon is incompatible
    RESTRICED_USE = 76, //!< Skill use restricted
};

/**
 * Error codes used for party actions by the game client.
 */
enum class PartyErrorCodes_t : uint16_t
{
    GENERIC_ERROR = 199,    //!< Generic system error
    SUCCESS = 200,  //!< No error
    INVALID_OR_OFFLINE = 201,   //!< Target character either does not exist or is offline
    IN_PARTY = 202, //!< Player is already in a party
    INVALID_PARTY = 203,    //!< Request for invalid party received
    NO_LEADER = 204,    //!< Party does not have a leader (not sure why this would happen)
    NO_PARTY = 205, //!< Player is not in a party
    INVALID_MEMBER = 206,   //!< Party member being requested is invalid
    PARTY_FULL = 207,   //!< Party is full and cannot be joined or invited to
    LEADER_REQUIRED = 208,  //!< Leader required update attempted by non-leader
    INVALID_TARGET  = 209,   //!< Target is invalid
};

/**
 * Error codes used for player interaction "entrust" actions by the game client.
 */
enum class EntrustErrorCodes_t : int32_t
{
    SUCCESS = 0,    //!< No error
    SYSTEM_ERROR = -1, //!< Generic system error
    INVALID_CHAR_STATE = -2,    //!< Character is not in a state to preform action
    TOO_FAR = -4,   //!< Too far from target
    NONTRADE_ITEMS = -5,    //!< Rewards contain non-trade items
    INVENTORY_SPACE_NEEDED = -6,    //!< More inventory space needed
    INVALID_DEMON_STATE = -8,   //!< Demon is not in a state to perform action
    INVALID_DEMON_TARGET = -9,  //!< Demon is not a valid target
};

/**
 * Error codes used for team actions by the game client.
 */
enum class TeamErrorCodes_t : int8_t
{
    SUCCESS = 0,    //!< No error
    GENERIC_ERROR = -1, //!< Unspecified error
    LEADER_REQUIRED = -3,   //!< Leader required update attempted by non-leader
    INVALID_TARGET  = -4,   //!< Target is invalid
    NO_TEAM = -5,   //!< No current team exists
    TEAM_OVER = -6, //!< Requested team no longer exists
    INVALID_TARGET_STATE = -7,  //!< Target is not in a valid team request state
    MATCH_ACTIVE = -8,  //!< A match is currently active and the operation is not allowed
    INVALID_TEAM = -9,  //!< Requested team is not valid
    TEAM_FULL = -10,    //!< Requested team is full
    AWAITING_ENTRY = -11,   //!< Request invalid due to match entry queue
    PENALTY_ACTIVE = -12,   //!< Request invalid due to too many PvP penalties
    MODE_REQUIREMENTS = -13,    //!< Mode requirements are not met
    MATCH_ACTIVE_REJECT = -14,  //!< Target is in an active match and the operation is not allowed
    AWAITING_ENTRY_REJECT = -15,    //!< Requested target invalid due to match entry queue
    PENALTY_ACTIVE_REJECT = -16,    //!< Requested target invalid due to too many PvP penalties
    MODE_REQUIREMENTS_REJECT = -17, //!< Requested target mode requirements are not met
    TARGET_IN_PARTY = -19,  //!< Target is in a party and cannot join a team
    TARGET_DIASPORA_INVALID = -20,  //!< Target cannot enter a disapora team right now
    TARGET_VALUABLE_MISSING = -21,  //!< Target is missing a valuable required to be in the team
    TARGET_COOLDOWN_20H = -22,  //!< Target team cooldown has not completed
    IN_PARTY = -24, //!< Party exists and team cannot be formed
    VALUABLE_MISSING = -25, //!< Valuable required is missing to form the team
    COOLDOWN_20H = -27, //!< Team cooldown has not completed
    ZONE_INVALID = -28, //!< Zone the requestor is in does not support the team type
};

#endif // LIBCOMP_SRC_ERRORCODES_H
