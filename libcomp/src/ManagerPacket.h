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

class ManagerPacket : public libcomp::Manager
{
public:
    ManagerPacket(const std::shared_ptr<libcomp::BaseServer>& server);
    virtual ~ManagerPacket();

    /**
     * @brief Addes a packet parser to this manager.
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

protected:
    static std::list<libcomp::Message::MessageType> sSupportedTypes;

    std::unordered_map<CommandCode_t,
        std::shared_ptr<PacketParser>> mPacketParsers;

    std::shared_ptr<libcomp::BaseServer> mServer;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_MANAGERPACKET_H
