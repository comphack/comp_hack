/**
 * @file server/channel/src/CharacterManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages characters on the channel.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_CHARACTERMANAGER_H
#define SERVER_CHANNEL_SRC_CHARACTERMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"

namespace channel
{

class ChannelServer;

/**
 * Manager to handle Character focused actions.
 */
class CharacterManager
{
public:
    /**
     * Create a new CharacterManager.
     */
    CharacterManager();

    /**
     * Clean up the CharacterManager.
     */
    ~CharacterManager();

    /**
     * Send updated character data to the game client.
     * @param client Pointer to the client connection
     */
    void SendCharacterData(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Tell the game client to show a character.
     * @param client Pointer to the client connection
     */
    void ShowCharacter(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Send updated data about a demon in the COMP to the game client.
     * @param client Pointer to the client connection
     * @param box Demon container ID, should always be zero
     * @param slot Slot of the demon to send data for
     * @param id ID of the demon to send data for
     */
    void SendCOMPDemonData(const std::shared_ptr<
        ChannelClientConnection>& client,
        int8_t box, int8_t slot, int64_t id);

    /**
     * Send a character's status icon to the game clients.
     * @param client Pointer to the client connection containing
     *  the character
     */
    void SendStatusIcon(const std::shared_ptr<
        channel::ChannelClientConnection>& client);
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHARACTERMANAGER_H
