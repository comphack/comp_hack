/**
 * @file libcomp/src/InternalConnection.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Internal server connection class.
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

#ifndef LIBCOMP_SRC_INTERNALCONNECTION_H
#define LIBCOMP_SRC_INTERNALCONNECTION_H

// libcomp Includes
#include "EncryptedConnection.h"

namespace libcomp
{

/**
 * Represents a connection established between two internal servers.
 */
class InternalConnection : public libcomp::EncryptedConnection
{
public:
    /**
     * Create a new internal connection.
     * @param io_service ASIO service to manage this connection.
     */
    InternalConnection(asio::io_service& io_service);

    /**
     * Create a new internal connection.
     * @param socket Socket provided by the server for the new client.
     * @param pDiffieHellman Asymmetric encryption information.
     */
    InternalConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);

    /**
     * Cleanup the connection object.
     */
    virtual ~InternalConnection();
};

} // namespace libcomp

#endif // LIBCOMP_SRC_INTERNALCONNECTION_H
