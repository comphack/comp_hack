/**
 * @file server/lobby/src/ManagerPacket.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to handle lobby packets.
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

#ifndef LIBCOMP_SRC_MANAGERPACKET_H
#define LIBCOMP_SRC_MANAGERPACKET_H

// libcomp Includes
#include "BaseServer.h"
#include "Manager.h"

// Standard C++11 Includes
#include <stdint.h>
#include <memory>
#include <unordered_map>

namespace lobby
{

enum class PacketManagerMode
{
    MANAGER_INTERNAL,
    MANAGER_CLIENT_FACING,
};

typedef uint16_t CommandCode_t;

class PacketParser;

class ManagerPacket : public libcomp::Manager
{
public:
    ManagerPacket(PacketManagerMode mode, std::shared_ptr<libcomp::BaseServer> server);
    virtual ~ManagerPacket();

    /**
     * @brief Get the different types of messages handles by this manager.
     */
    virtual std::list<libcomp::Message::MessageType> GetSupportedTypes() const;

    /**
     * Process a message from the queue.
     */
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage);

    /**
    * Get the server this manager belongs to.
    */
    std::shared_ptr<libcomp::BaseServer> GetServer();

private:
    std::unordered_map<CommandCode_t,
        std::shared_ptr<PacketParser>> mPacketParsers;

    std::shared_ptr<libcomp::BaseServer> mServer;
};

} // namespace lobby

#endif // LIBCOMP_SRC_MANAGERPACKET_H
