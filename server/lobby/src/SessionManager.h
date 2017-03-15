/**
 * @file server/lobby/src/SessionManager.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to track session keys.
 *
 * This file is part of the Lobby Server (lobby).
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

#ifndef SERVER_LOBBY_SRC_SESSIONMANAGER_H
#define SERVER_LOBBY_SRC_SESSIONMANAGER_H

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <mutex>
#include <unordered_map>

namespace lobby
{

/**
 * Manages session IDs for accounts connected to the lobby.
 */
class SessionManager
{
public:
    /**
     * Generate a new SID set for an account.
     * @param username Username of the account.
     * @return Generated SIDs for the account.
     */
    std::pair<libcomp::String, libcomp::String> GenerateSIDs(
        const libcomp::String& username);

    /**
     * Check an SID for an account.
     * @param username Username of the account.
     * @param value Value to check against the SID.
     * @param newSID Value of the new SID.
     * @return If the value matches what was recorded and
     *  there is either no timeout or it has not passed.
     */
    bool CheckSID(const libcomp::String& username,
        const libcomp::String& value, libcomp::String& newSID);

    /**
     * Clear an account's session or set an expiration timeout.
     * @param username Username of the account.
     * @param timeout Optional parameter to invalidate the
     *  session after the specified time offset has passed.
     *  If a negative value is specified (or defaulted) the
     *  timeout will not be used.
     */
    void ExpireSession(const libcomp::String& username,
        int8_t timeout = -1);

    /**
     * Clear an account session's expiration timeout.
     * @param username Username of the account.
     */
    void RefreshSession(const libcomp::String& username);

private:
    /// Lock for access to the session map.
    std::mutex mSessionLock;

    /// Map of accounts with associated session IDs.
    std::unordered_map<libcomp::String,
        std::pair<libcomp::String, libcomp::String>> mSessionMap;

    /// Map of accounts with timeouts for session validation.
    std::unordered_map<libcomp::String, time_t> mTimeoutMap;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_SESSIONMANAGER_H
