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

#ifndef LIBCOMP_SRC_ERRORCODES_H
#define LIBCOMP_SRC_ERRORCODES_H

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

#endif // LIBCOMP_SRC_ERRORCODES_H
