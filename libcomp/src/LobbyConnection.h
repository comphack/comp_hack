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

/**
 * Represents a dedicated connection type for a lobby server in charge
 * of game client authentication and communication prior to connecting
 * to a world channel server.
 */
class LobbyConnection : public libcomp::EncryptedConnection
{
public:
    /**
     * Connection mode used to specify normal communications or
     * special actions both servers understand.
     */
    enum class ConnectionMode_t
    {
        MODE_NORMAL,    //!< Normal communication
        MODE_PING,      //!< Servers should send ping/pong messages
        MODE_WORLD_UP,  //!< A world is communicating that wants
                        //!< to connect to the lobby
    };

    /**
     * Create a new lobby connection.
     * @param io_service ASIO service to manage this connection.
     * @param mode What mode should the connection act in?
     */
    LobbyConnection(asio::io_service& io_service,
        ConnectionMode_t mode = ConnectionMode_t::MODE_NORMAL);

    /**
     * Create a new lobby connection.
     * @param socket Socket provided by the server for the new client.
     * @param pDiffieHellman Asymmetric encryption information.
     */
    LobbyConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);

    /**
     * Cleanup the connection object.
     */
    virtual ~LobbyConnection();

    virtual void ConnectionSuccess();

protected:
    virtual bool ParseExtensionConnection(libcomp::Packet& packet);

    /**
     * Just parse the extension.
     */
    void ParseExtension(libcomp::Packet& packet);

    /// The connection's connection mode.
    ConnectionMode_t mMode;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_LOBBYCONNECTION_H
