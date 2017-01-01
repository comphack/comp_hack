/**
 * @file libcomp/src/ManagerPacket.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manager to handle packets.
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

#ifndef LIBCOMP_SRC_MANAGERPACKET_H
#define LIBCOMP_SRC_MANAGERPACKET_H

// libcomp Includes
#include <BaseServer.h>
#include <Manager.h>

// Standard C++11 Includes
#include <stdint.h>
#include <memory>
#include <unordered_map>

namespace libcomp
{

typedef uint16_t CommandCode_t;

class PacketParser;

/**
 * Manager dedicated to handling messages of type @ref libcomp::Message::Packet.
 */
class ManagerPacket : public libcomp::Manager
{
public:
    /**
     * Create a new manager.
     * @param server Pointer to the server that uses this manager
     */
    ManagerPacket(std::weak_ptr<libcomp::BaseServer> server);

    /**
     * Cleanup the manager.
     */
    virtual ~ManagerPacket();

    /**
     * Adds a packet parser of the specified type to this manager
     * to handle a specific command code.
     * @param commandCode Packet command code to handle
     * @return true if its a valid type and the command code is not
     *  already being handled, false otherwise
     */
    template <class T> bool AddParser(CommandCode_t commandCode)
    {
        if(mPacketParsers.find(commandCode) == mPacketParsers.end() &&
            std::is_base_of<PacketParser, T>::value)
        {
            mPacketParsers[commandCode] = std::dynamic_pointer_cast<PacketParser>(
                std::shared_ptr<T>(new T()));
            return true;
        }

        return false;
    }

    virtual std::list<libcomp::Message::MessageType> GetSupportedTypes() const;
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage);

    /**
     * Get the server that uses this manager.
     * @return Pointer to the server that uses this manager
     */
    std::shared_ptr<libcomp::BaseServer> GetServer();

protected:
    /// Static list containing the packet message type to return via
    /// @ref ManagerPacket::GetSupportedTypes
    static std::list<libcomp::Message::MessageType> sSupportedTypes;

    /// Packet parser map by command code used to process messages
    std::unordered_map<CommandCode_t,
        std::shared_ptr<PacketParser>> mPacketParsers;

    /// Pointer to the server that uses this manager
    std::weak_ptr<libcomp::BaseServer> mServer;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_MANAGERPACKET_H
