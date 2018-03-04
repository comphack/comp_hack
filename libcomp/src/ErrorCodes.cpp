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

#include "ErrorCodes.h"

libcomp::String ErrorCodeString(ErrorCodes_t error)
{
    libcomp::String msg;

    switch(error)
    {
        case ErrorCodes_t::SUCCESS:
            msg = "No error.";
            break;
        case ErrorCodes_t::SYSTEM_ERROR:
            msg = "A system error has occurred.";
            break;
        case ErrorCodes_t::PROTOCOL_ERROR:
            msg = "A protocol error has occurred.";
            break;
        case ErrorCodes_t::PRAMETER_ERROR:
            msg = "A parameter error has occurred.";
            break;
        case ErrorCodes_t::UNSUPPORTED_FEATURE:
            msg = "Unsupported feature.";
            break;
        case ErrorCodes_t::BAD_USERNAME_PASSWORD:
            msg = "Incorrect username or password.";
            break;
        case ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN:
            msg = "Account is already logged in.";
            break;
        case ErrorCodes_t::NOT_ENOUGH_CP:
            msg = "Insufficient cash shop points.";
            break;
        case ErrorCodes_t::SERVER_DOWN:
            msg = "Server currently down.";
            break;
        case ErrorCodes_t::NOT_AUTHORIZED:
            msg = "Not authorized to perform action.";
            break;
        case ErrorCodes_t::NEED_CHARACTER_TICKET:
            msg = "Do not have character creation ticket.";
            break;
        case ErrorCodes_t::NO_EMPTY_CHARACTER_SLOTS:
            msg = "No empty character slots.";
            break;
        case ErrorCodes_t::ALREADY_DID_THAT:
            msg = "You have already done that.";
            break;
        case ErrorCodes_t::SERVER_FULL:
            msg = "Server is currently full.";
            break;
        case ErrorCodes_t::CAN_NOT_BE_USED_YET:
            msg = "Feature can't be used yet.";
            break;
        case ErrorCodes_t::TOO_MANY_CHARACTERS:
            msg = "You have too many characters.";
            break;
        case ErrorCodes_t::BAD_CHARACTER_NAME:
            msg = "Can't use that character name.";
            break;
        case ErrorCodes_t::SERVER_CROWDED:
            msg = "Server crowded.";
            break;
        case ErrorCodes_t::WRONG_CLIENT_VERSION:
            msg = "Client must be updated to login.";
            break;
        case ErrorCodes_t::ACCOUNT_DISABLED:
            msg = "Account has been disabled.";
            break;
        case ErrorCodes_t::MUST_REAUTHORIZE_ACCOUNT:
            msg = "You must re-cert.";
            break;
        case ErrorCodes_t::ACCOUNT_LOCKED_A:
        case ErrorCodes_t::ACCOUNT_LOCKED_B:
            msg = "Account locked by antitheft function.";
            break;
        case ErrorCodes_t::CONNECTION_TIMEOUT:
            msg = "Connection has timed out.";
            break;
        default:
            msg = "An unknown error has occurred.";
            break;
    }

    return msg;
}
