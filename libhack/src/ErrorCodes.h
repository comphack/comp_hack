/**
 * @file libhack/src/ErrorCodes.h
 * @ingroup libhack
 *
 * @author HACKfrost
 *
 * @brief Contains enums for internal and external packet codes.
 *
 * This file is part of the COMP_hack Library (libhack).
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

#ifndef LIBHACK_SRC_ERRORCODES_H
#define LIBHACK_SRC_ERRORCODES_H

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <stdint.h>

/**
 * Error codes used by the game client.
 */
enum class ErrorCodes_t : int32_t {
  /// No error
  SUCCESS = 0,
  /// System error
  SYSTEM_ERROR = -1,
  /// Protocol error
  PROTOCOL_ERROR = -2,
  /// Parameter error
  PRAMETER_ERROR = -3,
  /// Unsupported feature
  UNSUPPORTED_FEATURE = -4,
  /// Incorrect username or password
  BAD_USERNAME_PASSWORD = -5,
  /// Account still logged in
  ACCOUNT_STILL_LOGGED_IN = -6,
  /// Insufficient cash shop points
  NOT_ENOUGH_CP = -7,
  /// Server currently down
  SERVER_DOWN = -8,
  /// Not authorized to perform action
  NOT_AUTHORIZED = -9,
  /// Do not have character creation ticket
  NEED_CHARACTER_TICKET = -10,
  /// No empty character slots
  NO_EMPTY_CHARACTER_SLOTS = -11,
  /// You have already done that
  ALREADY_DID_THAT = -12,
  /// Server is currently full
  SERVER_FULL = -13,
  /// Feature can't be used yet
  CAN_NOT_BE_USED_YET = -14,
  /// You have too many characters
  TOO_MANY_CHARACTERS = -15,
  /// Can't use that character name
  BAD_CHARACTER_NAME = -16,
  /// Server crowded (with popup)
  SERVER_CROWDED = -17,
  /// Wrong client version (and any gap)
  WRONG_CLIENT_VERSION = -18,
  /// Currently can't use this account
  ACCOUNT_DISABLED = -26,
  /// To log in you must re-cert (pops up login window)
  MUST_REAUTHORIZE_ACCOUNT = -28,
  /// Account locked by antitheft function
  ACCOUNT_LOCKED_A = -101,
  /// Account locked by antitheft function
  ACCOUNT_LOCKED_B = -102,
  /// Connection has timed out
  CONNECTION_TIMEOUT = -103,
};

libcomp::String ErrorCodeString(ErrorCodes_t error);

/**
 * Error codes used for skill failure by the game client. Most will print a
 * message in the player's chat window but they can include action requests.
 */
enum class SkillErrorCodes_t : uint8_t {
  /// Generic error has occurred, no message
  GENERIC = 0,
  /// Cannot be used
  GENERIC_USE = 2,
  /// Cannot be paid for
  GENERIC_COST = 3,
  /// Cannot be activated
  ACTIVATION_FAILURE = 4,
  /// Cool down has not completed
  COOLING_DOWN = 5,
  /// No message, request that client pursue and retry
  ACTION_RETRY = 6,
  /// The skill was too far away when execution was attempted
  TOO_FAR = 8,
  /// Skill is not in a useable state
  CONDITION_RESTRICT = 10,
  /// Cannot be used in the current location
  LOCATION_RESTRICT = 11,
  /// Item cannot have its skill used
  ITEM_USE = 12,
  /// Skill cannot be used but don't print an error
  SILENT_FAIL = 13,
  /// Target cannot be talked to
  TALK_INVALID = 21,
  /// Target's level is too high and cannot be talked
  TALK_LEVEL = 22,
  /// Target refuses to listen to talk skills
  TALK_WONT_LISTEN = 23,
  /// Demon cannot be talked to due to its current state
  TALK_INVALID_STATE = 25,
  /// Demon cannot be summoned
  SUMMON_INVALID = 26,
  /// Demon's level is too high and cannot be summoned
  SUMMON_LEVEL = 28,
  /// Target invalid for skill
  TARGET_INVALID = 35,
  /// LNC differs
  LNC_DIFFERENCE = 36,
  /// Character does not have the right mount item
  MOUNT_ITEM_MISSING = 37,
  /// Mount iem's durability is zero
  MOUNT_ITEM_DURABILITY = 38,
  /// Attempted to summon while on mount
  MOUNT_SUMMON_RESTRICT = 39,
  /// Partner demon cannot act as mount
  MOUNT_DEMON_INVALID = 40,
  /// Partner mount target is too far away
  MOUNT_TOO_FAR = 41,
  /// Partner mount target condition is not valid
  MOUNT_DEMON_CONDITION = 42,
  /// Attempted to use non-mount skill while on mount
  MOUNT_OTHER_SKILL_RESTRICT = 44,
  /// Cannot move so mounting not allowed
  MOUNT_MOVE_RESTRICT = 45,
  /// No partner demon summoned
  PARTNER_MISSING = 46,
  /// Partner demon familiarity too low
  PARTNER_FAMILIARITY = 47,
  /// Partner demon is dead
  PARTNER_DEAD = 50,
  /// Partner demon familiarity too low for item
  PARTNER_FAMILIARITY_ITEM = 52,
  /// Partner demon is too far away
  PARTNER_TOO_FAR = 53,
  /// Devil fusion cannot be used in current location
  DEVIL_FUSION_RESTRICT = 54,
  /// No partner demon summoned so mooch cannot be used
  MOOCH_PARTNER_MISSING = 55,
  /// Partner demon familiarity too low for mooch
  MOOCH_PARTNER_FAMILIARITY = 56,
  /// Partner demon is dead so mooch cannot be used
  MOOCH_PARTNER_DEAD = 59,
  /// Partner demon is too far away for mooch
  MOOCH_PARTNER_TOO_FAR = 60,
  /// Inventory space needed to recieve demon present
  INVENTORY_SPACE_PRESENT = 61,
  /// Inventory space needed to receive item
  INVENTORY_SPACE = 63,
  /// Nothing happened currently
  NOTHING_HAPPENED_NOW = 68,
  /// Nothing happened in the current place
  NOTHING_HAPPENED_HERE = 69,
  /// Time invalid for use
  TIME_RESTRICT = 71,
  /// Target zone is not valid
  ZONE_INVALID = 72,
  /// Partner demon is incompatible
  PARTNER_INCOMPATIBLE = 75,
  /// Skill use restricted
  RESTRICED_USE = 76,
};

/**
 * Error codes used for party actions by the game client.
 */
enum class PartyErrorCodes_t : uint16_t {
  /// Generic system error
  GENERIC_ERROR = 199,
  /// No error
  SUCCESS = 200,
  /// Target character either does not exist or is offline
  INVALID_OR_OFFLINE = 201,
  /// Player is already in a party
  IN_PARTY = 202,
  /// Request for invalid party received
  INVALID_PARTY = 203,
  /// Party does not have a leader (not sure why this would happen)
  NO_LEADER = 204,
  /// Player is not in a party
  NO_PARTY = 205,
  /// Party member being requested is invalid
  INVALID_MEMBER = 206,
  /// Party is full and cannot be joined or invited to
  PARTY_FULL = 207,
  /// Leader required update attempted by non-leader
  LEADER_REQUIRED = 208,
  /// Target is invalid
  INVALID_TARGET = 209,
};

/**
 * Error codes used for player interaction "entrust" actions by the game client.
 */
enum class EntrustErrorCodes_t : int32_t {
  /// No error
  SUCCESS = 0,
  /// Generic system error
  SYSTEM_ERROR = -1,
  /// Character is not in a state to preform action
  INVALID_CHAR_STATE = -2,
  /// Too far from target
  TOO_FAR = -4,
  /// Rewards contain non-trade items
  NONTRADE_ITEMS = -5,
  /// More inventory space needed
  INVENTORY_SPACE_NEEDED = -6,
  /// Demon is not in a state to perform action
  INVALID_DEMON_STATE = -8,
  /// Demon is not a valid target
  INVALID_DEMON_TARGET = -9,
};

/**
 * Error codes used for team actions by the game client.
 */
enum class TeamErrorCodes_t : int8_t {
  /// No error
  SUCCESS = 0,
  /// Unspecified error
  GENERIC_ERROR = -1,
  /// Leader required update attempted by non-leader
  LEADER_REQUIRED = -3,
  /// Target is invalid
  INVALID_TARGET = -4,
  /// No current team exists
  NO_TEAM = -5,
  /// A different team already exists
  OTHER_TEAM = -6,
  /// Target is not in a valid team request state
  INVALID_TARGET_STATE = -7,
  /// A match is currently active and the operation is not allowed
  MATCH_ACTIVE = -8,
  /// Requested team is not valid
  INVALID_TEAM = -9,
  /// Requested team is full
  TEAM_FULL = -10,
  /// Request invalid due to match entry queue
  AWAITING_ENTRY = -11,
  /// Request invalid due to too many PvP penalties
  PENALTY_ACTIVE = -12,
  /// Mode requirements are not met
  MODE_REQUIREMENTS = -13,
  /// Target is in an active match and the operation is not allowed
  MATCH_ACTIVE_REJECT = -14,
  /// Requested target invalid due to match entry queue
  AWAITING_ENTRY_REJECT = -15,
  /// Requested target invalid due to too many PvP penalties
  PENALTY_ACTIVE_REJECT = -16,
  /// Requested target mode requirements are not met
  MODE_REQUIREMENTS_REJECT = -17,
  /// Target is in a party and cannot join a team
  TARGET_IN_PARTY = -19,
  /// Target cannot enter a disapora team right now
  TARGET_DIASPORA_INVALID = -20,
  /// Target is missing a valuable required to be in the team
  TARGET_VALUABLE_MISSING = -21,
  /// Target team cooldown has not completed
  TARGET_COOLDOWN_20H = -22,
  /// Party exists and team cannot be formed
  IN_PARTY = -24,
  /// Valuable required is missing to form the team
  VALUABLE_MISSING = -25,
  /// Team cooldown has not completed
  COOLDOWN_20H = -27,
  /// Zone the requestor is in does not support the team type
  ZONE_INVALID = -28,
};

#endif  // LIBHACK_SRC_ERRORCODES_H
