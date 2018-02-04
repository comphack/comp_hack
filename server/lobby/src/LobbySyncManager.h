/**
 * @file server/lobby/src/LobbySyncManager.h
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief 
 *
 * This file is part of the Lobby Server (lobby).
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

#ifndef SERVER_LOBBY_SRC_LOBBYSYNCMANAGER_H
#define SERVER_LOBBY_SRC_LOBBYSYNCMANAGER_H

// libcomp Includes
#include <DataSyncManager.h>
#include <EnumMap.h>

// object Includes
#include <SearchEntry.h>

namespace objects
{
class Account;
}

namespace lobby
{

class LobbyServer;

/**
 * Lobby specific implementation of the DataSyncManager in charge of
 * performing server side update operations.
 */
class LobbySyncManager : public libcomp::DataSyncManager
{
public:
    /**
     * Create a new LobbySyncManager
     * @param server Pointer back to the channel server this belongs to.
     */
    LobbySyncManager(const std::weak_ptr<LobbyServer>& server);

    /**
     * Clean up the LobbySyncManager
     */
    virtual ~LobbySyncManager();

    /**
     * Initialize the LobbySyncManager after the server has been initialized.
     * @return false if any errors were encountered and the server should
     *  be shut down.
     */
    bool Initialize();

    /**
     * Server specific handler for explicit types of non-persistent records
     * being updated.
     * @param type Type name of the object being updated
     * @param obj Pointer to the record definition
     * @param isRemove true if the record is being removed, false if it is
     *  either an insert or an update
     * @param source Source server identifier
     * @return Response codes matching the internal DataSyncManager set
     */
    template<class T> int8_t Update(const libcomp::String& type,
        const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
        const libcomp::String& source);

private:
    /**
     * Sync the supplied.
     * @return false if any errors were encountered and the server should
     *  be shut down.
     */
    void SyncAccount(const std::shared_ptr<objects::Account>& account);

    /// Pointer to the lobby server.
    std::weak_ptr<LobbyServer> mServer;
};

} // namespace lobby

#endif // SERVER_WORLD_SRC_LOBBYSYNCMANAGER_H
