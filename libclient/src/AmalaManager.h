/**
 * @file libcomp/src/AmalaManager.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manages the custom amala network packets.
 *
 * This file is part of the COMP_hack Client Library (libclient).
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

#ifndef LIBCLIENT_SRC_AMALAMANAGER_H
#define LIBCLIENT_SRC_AMALAMANAGER_H

// libcomp Includes
#include <Manager.h>
#include <MessagePacket.h>
#include <MessageQueue.h>

namespace libcomp
{

namespace Message
{

class MessageClient;

} // namespace Message

} // namespace libcomp

namespace logic
{
    class LogicWorker;

    /**
     * Worker for client<==>server interaction.
     */
    class AmalaManager : public libcomp::Manager
    {
    public:
        /**
         * Create a new worker.
         * @param messageQueue Message queue of the LogicWorker.
         */
        explicit AmalaManager(LogicWorker *pLogicWorker,
            const std::weak_ptr<
                libcomp::MessageQueue<libcomp::Message::Message *>>
                &messageQueue);

        /**
         * Cleanup the worker.
         */
        virtual ~AmalaManager();

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

        /**
         * Process a client message.
         * @param pMessage Client message to process.
         */
        bool ProcessClientMessage(
            const libcomp::Message::MessageClient *pMessage);

    private:
        /**
         * Handle the incoming account dump.
         * @returns true if the packet was parsed correctly; false otherwise.
         */
        bool HandlePacketChannelAmalaAccountDumpHeader(libcomp::ReadOnlyPacket &p);

        /**
         * Handle the incoming account dump.
         * @returns true if the packet was parsed correctly; false otherwise.
         */
        bool HandlePacketChannelAmalaAccountDumpPart(libcomp::ReadOnlyPacket &p);

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

        /// Account dump SHA1 hash.
        libcomp::String mAccountDumpSHA1;

        /// Account dump account name.
        libcomp::String mAccountDumpName;

        /// Account dump SHA1 hash.
        std::vector<char> mAccountDump;

        /// Size of the account dump.
        uint32_t mAccountDumpSize;

        /// Number of parts.
        uint32_t mAccountDumpPartCount;

        /// Account dump file path.
        libcomp::String mAccountDumpPath;
    };

} // namespace logic

#endif // LIBCLIENT_SRC_AMALAMANAGER_H
