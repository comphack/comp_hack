/**
 * @file libcomp/src/LobbyManager.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manages the active lobby client connection.
 *
 * This file is part of the COMP_hack Client Library (libclient).
 *
 * Copyright (C) 2012-2019 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCLIENT_SRC_LOBBYMANAGER_H
#define LIBCLIENT_SRC_LOBBYMANAGER_H

// libcomp Includes
#include <Manager.h>
#include <MessagePacket.h>
#include <MessageQueue.h>

namespace packets
{
    class PacketLobbyWorldList;
} // namespace packets

namespace logic
{
    class LogicWorker;

    /**
     * Worker for client<==>server interaction.
     */
    class LobbyManager : public libcomp::Manager
    {
    public:
        /**
         * Create a new worker.
         * @param messageQueue Message queue of the LogicWorker.
         */
        explicit LobbyManager(LogicWorker *pLogicWorker,
            const std::weak_ptr<
                libcomp::MessageQueue<libcomp::Message::Message *>>
                &messageQueue);

        /**
         * Cleanup the worker.
         */
        virtual ~LobbyManager();

        /**
         * Get the different types of messages handled by the manager.
         * @return List of message types handled by the manager
         */
        std::list<libcomp::Message::MessageType>
            GetSupportedTypes() const override;

        /**
         * Process a message from the queue.
         * @param pMessage Message to be processed
         * @return true on success, false on failure
         */
        bool ProcessMessage(const libcomp::Message::Message *pMessage) override;

    private:
        /**
         * Handle the incoming world list.
         * @returns true if the packet was parsed correctly; false otherwise.
         */
        bool HandlePacketLobbyWorldList(libcomp::ReadOnlyPacket &p);

        /**
         * Process a packet message.
         * @param pMessage Packet message to process.
         */
        bool ProcessPacketMessage(const libcomp::Message::Packet *pMessage);

        /// Pointer to the LogicWorker.
        LogicWorker *mLogicWorker;

        /// Message queue for the LogicWorker.
        std::weak_ptr<libcomp::MessageQueue<libcomp::Message::Message *>>
            mMessageQueue;

        /// List of the worlds and channels.
        std::shared_ptr<packets::PacketLobbyWorldList> mWorldList;
    };

} // namespace logic

#endif // LIBCLIENT_SRC_LOBBYMANAGER_H
