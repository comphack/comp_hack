/**
 * @file server/lobby/src/World.h
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief World definition in regards to the lobby containing an active connection to the world server.
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

#ifndef SERVER_LOBBY_SRC_WORLD_H
#define SERVER_LOBBY_SRC_WORLD_H

// libcomp Includes
#include <ChannelDescription.h>
#include <InternalConnection.h>
#include <WorldDescription.h>

namespace lobby
{

class World
{
public:
    World(std::shared_ptr<libcomp::InternalConnection> connection);
    virtual ~World();

    /**
    * @brief Get all necessary descriptive information about this world
    */
    bool Initialize();

    std::shared_ptr<libcomp::InternalConnection> GetConnection() const;

    std::shared_ptr<objects::WorldDescription> GetWorldDescription() const;

    const std::list<std::shared_ptr<objects::ChannelDescription>> GetChannelDescriptions() const;

    std::shared_ptr<objects::ChannelDescription> GetChannelDescriptionByID(uint8_t id) const;

    bool RemoveChannelDescriptionByID(uint8_t id);

    void SetWorldDescription(const std::shared_ptr<objects::WorldDescription>& worldDescription);

    void SetChannelDescription(const std::shared_ptr<objects::ChannelDescription>& channelDescription);

private:
    std::shared_ptr<libcomp::InternalConnection> mConnection;

    std::shared_ptr<objects::WorldDescription> mWorldDescription;
    std::list<std::shared_ptr<objects::ChannelDescription>> mChannelDescriptions;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_WORLD_H
