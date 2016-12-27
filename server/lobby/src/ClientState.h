/**
 * @file server/lobby/src/ClientState.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief State of a client connection.
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

#ifndef SERVER_LOBBY_SRC_CLIENTSTATE_H
#define SERVER_LOBBY_SRC_CLIENTSTATE_H

// objects Includes
#include <ClientStateObject.h>

namespace lobby
{

class ClientState : public objects::ClientStateObject
{
public:
    ClientState();
    virtual ~ClientState();
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_CLIENTSTATE_H
