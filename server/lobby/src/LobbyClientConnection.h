/**
 * @file server/lobby/src/LobbyClientConnection.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby client connection class.
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

#ifndef SERVER_LOBBY_SRC_LOBBYCLIENTCONNECTION_H
#define SERVER_LOBBY_SRC_LOBBYCLIENTCONNECTION_H

// lobby Includes
#include "ClientState.h"

// libcomp Includes
#include <LobbyConnection.h>
#include <ManagerPacket.h>

// object Includes
#include <LobbyConfig.h>

namespace lobby
{

class LobbyClientConnection : public libcomp::LobbyConnection
{
public:
    LobbyClientConnection(asio::ip::tcp::socket& socket, DH *pDiffieHellman);
    virtual ~LobbyClientConnection();

    ClientState* GetClientState() const;
    void SetClientState(const std::shared_ptr<ClientState>& state);

private:
    std::shared_ptr<ClientState> mClientState;
};

static inline ClientState* state(
    const std::shared_ptr<libcomp::TcpConnection>& connection)
{
    ClientState *pState = nullptr;

    auto conn = std::dynamic_pointer_cast<LobbyClientConnection>(connection);

    if(nullptr != conn)
    {
        pState = conn->GetClientState();
    }

    return pState;
}

static inline std::shared_ptr<objects::LobbyConfig> config(
    libcomp::ManagerPacket *pPacketManager)
{
    std::shared_ptr<objects::LobbyConfig> pConfig;

    if(nullptr != pPacketManager)
    {
        auto server = pPacketManager->GetServer();

        if(nullptr != server)
        {
            pConfig = std::dynamic_pointer_cast<objects::LobbyConfig>(
                server->GetConfig());
        }
    }

    return pConfig;
}

} // namespace lobby

#endif // SERVER_LOBBY_SRC_LOBBYCLIENTCONNECTION_H
