/**
 * @file libcomp/src/LobbyConnection.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby connection class.
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

#ifndef LIBCOMP_SRC_LOBBYCONNECTION_H
#define LIBCOMP_SRC_LOBBYCONNECTION_H

// libcomp Includes
#include "EncryptedConnection.h"

namespace libcomp
{

class LobbyConnection : public libcomp::EncryptedConnection
{
public:
    enum class ConnectionMode_t
    {
        MODE_NORMAL,
        MODE_PING,
        MODE_WORLD_UP,
    };

    LobbyConnection(asio::io_service& io_service,
        ConnectionMode_t mode = ConnectionMode_t::MODE_NORMAL);
    LobbyConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);
    virtual ~LobbyConnection();

    virtual void ConnectionSuccess();

protected:
    virtual bool ParseExtensionConnection(libcomp::Packet& packet);

    void ParseExtension(libcomp::Packet& packet);

    ConnectionMode_t mMode;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_LOBBYCONNECTION_H
