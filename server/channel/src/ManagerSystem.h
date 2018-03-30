/**
 * @file server/channel/src/ManagerSystem.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager to handle system messages specific to the channel server.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_MANAGERSYSTEM_H
#define SERVER_CHANNEL_SRC_MANAGERSYSTEM_H

// libcomp Includes
#include <BaseServer.h>
#include <InternalConnection.h>
#include <Manager.h>

// Internal Protocol Objects
//#include <Protocol/Internal/x.h>

// Boost ASIO Includes
#include <asio.hpp>

// channel Includes
#include "ChannelClientConnection.h"

namespace channel
{

/**
 * Class to handle messages pertaining to connecting to
 * the world or game clients.
 */
class ManagerSystem : public libcomp::Manager
{
public:
    /**
     * Create a new manager.
     * @param server Pointer to the server that uses this manager
     */
    ManagerSystem(std::weak_ptr<libcomp::BaseServer> server);

    /**
     * Clean up the manager.
     */
    virtual ~ManagerSystem();

    /**
     * Get the different types of messages handled by this manager.
     * @return List of supported message types
     */
    virtual std::list<libcomp::Message::MessageType> GetSupportedTypes() const;

    /**
     * Process a message from the queue.
     * @param pMessage Message to be processed.
     * @return true on success, false on failure
     */
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage);
private:
    /// Static list of supported message types for the manager.
    static std::list<libcomp::Message::MessageType> sSupportedTypes;

    /// Pointer to the server that uses this manager.
    std::weak_ptr<libcomp::BaseServer> mServer;
};

} // namespace world

#endif // SERVER_CHANNEL_SRC_MANAGERCONNECTION_H
