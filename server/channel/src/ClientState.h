/**
 * @file server/channel/src/ClientState.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a client connection.
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

#ifndef SERVER_CHANNEL_SRC_CLIENTSTATE_H
#define SERVER_CHANNEL_SRC_CLIENTSTATE_H

// channel Includes
#include "CharacterState.h"

// objects Includes
#include <ClientStateObject.h>

namespace channel
{

/**
 * Contains the state of a game client currently connected to the
 * channel.
 */
class ClientState : public objects::ClientStateObject
{
public:
    /**
     * Create a new client state.
     */
    ClientState();

    /**
     * Clean up the client state.
     */
    virtual ~ClientState();

    /**
     * Get the string encoding to use for this client.
     * @return Encoding to use for strings
     */
    libcomp::Convert::Encoding_t GetClientStringEncoding();

    /**
     * Get the state of the character associated to the client.
     * @return Pointer to the CharacterState
     */
    std::shared_ptr<CharacterState> GetCharacterState();

    /**
     * Check if the client state has everything needed to start
     * being used.
     * @return true if the state is ready to use, otherwise false
     */
    bool Ready();

private:
    /// State of the character associated to the client
    std::shared_ptr<CharacterState> mCharacterState;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CLIENTSTATE_H
