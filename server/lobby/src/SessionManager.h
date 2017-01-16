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
     * Generate a new SID for an account.
     * @param sid Which SID to generate.
     * @param username Username of the account.
     * @return Generated SID for the account.
     */
    libcomp::String GenerateSID(uint8_t sid, const libcomp::String& username);

    /**
     * Generate a new SID set for an account.
     * @param username Username of the account.
     * @return Generated SIDs for the account.
     */
    std::pair<libcomp::String, libcomp::String> GenerateSIDs(
        const libcomp::String& username);

    /**
     * Check an SID for an account.
     * @param sid Which SID to check.
     * @param username Username of the account.
     * @param value Value to check against the SID.
     * @param otherSID Value of the other SID.
     * @return If the value matches what was recorded.
     */
    bool CheckSID(uint8_t sid, const libcomp::String& username,
        const libcomp::String& value, libcomp::String& otherSID);

private:
    /// Lock for access to the session map.
    std::mutex mSessionLock;

    /// Map of accounts with associated session IDs.
    std::unordered_map<libcomp::String,
        std::pair<libcomp::String, libcomp::String>> mSessionMap;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_SESSIONMANAGER_H
