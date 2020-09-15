/**
 * @file server/lobby/src/LobbyClientConnection.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby client connection class.
 *
 * This file is part of the Lobby Server (lobby).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "LobbyClientConnection.h"

using namespace lobby;

LobbyClientConnection::LobbyClientConnection(
    asio::ip::tcp::socket& socket,
    const std::shared_ptr<libcomp::Crypto::DiffieHellman>& diffieHellman)
    : LobbyConnection(socket, diffieHellman) {}

LobbyClientConnection::~LobbyClientConnection() {}

ClientState* LobbyClientConnection::GetClientState() const {
  return mClientState.get();
}

void LobbyClientConnection::SetClientState(
    const std::shared_ptr<ClientState>& state) {
  mClientState = state;
}
