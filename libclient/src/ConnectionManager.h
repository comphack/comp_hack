/**
 * @file libcomp/src/ConnectionManager.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manages the active client connection to the server.
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

#ifndef LIBCLIENT_SRC_CONNECTIONMANAGER_H
#define LIBCLIENT_SRC_CONNECTIONMANAGER_H

// libobjgen Includes
#include <UUID.h>

// libcomp Includes
#include <ConnectionMessage.h>
#include <EncryptedConnection.h>
#include <Manager.h>
#include <MessageClient.h>
#include <MessagePacket.h>
#include <MessageQueue.h>
#include <ReadOnlyPacket.h>

// Standard C++11 Includes
#include <thread>

namespace logic
{
    class LogicWorker;

    /**
     * Worker for client<==>server interaction.
     */
    class ConnectionManager : public libcomp::Manager
    {
    public:
        /**
         * Create a new worker.
         * @param messageQueue Message queue of the LogicWorker.
         */
        explicit ConnectionManager(LogicWorker *pLogicWorker,
            const std::weak_ptr<
                libcomp::MessageQueue<libcomp::Message::Message *>>
                &messageQueue);

        /**
         * Cleanup the worker.
         */
        virtual ~ConnectionManager();

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
         * Close any active connection and initiate a new lobby connection.
         * @param connectionID ID for the connection.
         * @param host Hostname to connect to.
         * @param port TCP port to connect to on the host.
         * @returns true if the connection has started; false otherwise.
         *
         * @note This function should only be called from the logic thread.
         */
        bool ConnectLobby(const libcomp::String &connectionID = "lobby",
            const libcomp::String &host = "127.0.0.1", uint16_t port = 10666);

        /**
         * Close any active connection and initiate a new channel connection.
         * @param connectionID ID for the connection.
         * @param host Hostname to connect to.
         * @param port TCP port to connect to on the host.
         * @returns true if the connection has started; false otherwise.
         *
         * @note This function should only be called from the logic thread.
         */
        bool ConnectChannel(const libcomp::String &connectionID = "channel",
            const libcomp::String &host = "127.0.0.1", uint16_t port = 14666);

        /**
         * Close the active connection.
         * @returns true if the connection was closed; false otherwise.
         *
         * @note This function should only be called from the logic thread.
         */
        bool CloseConnection();

        /**
         * Queue a packet and then send all queued packets to the remote host.
         * @param packet Packet to send to the remote host.
         */
        void SendPacket(libcomp::Packet &packet);

        /**
         * Queue a packet and then send all queued packets to the remote host.
         * @param packet Packet to send to the remote host.
         */
        void SendPacket(libcomp::ReadOnlyPacket &packet);

        /**
         * Queue packets and then send all queued packets to the remote host.
         * @param packets Packets to send to the remote host.
         */
        void SendPackets(const std::list<libcomp::Packet *> &packets);

        /**
         * Queue packets and then send all queued packets to the remote host.
         * @param packets Packets to send to the remote host.
         */
        void SendPackets(const std::list<libcomp::ReadOnlyPacket *> &packets);

        /**
         * Packetize and queue an object and then send all queued packets to the
         *   remote host.
         * @param obj Object to be packetized.
         * @return true if the object could be packetized; false otherwise.
         */
        bool SendObject(const std::shared_ptr<libcomp::Object> &obj);

        /**
         * Packetize and queue objects and then send all queued packets to the
         *   remote host.
         * @param objs Objects to be packetized.
         * @return true if the objects could be packetized; false otherwise.
         */
        bool SendObjects(
            const std::list<std::shared_ptr<libcomp::Object>> &objs);

        /**
         * Determine if there is an active encrypted connection.
         * @returns true if connected; false otherwise.
         *
         * @note This function should only be called from the logic thread.
         */
        bool IsConnected() const;

        /**
         * Determine if the active connection is connected to the lobby.
         * @returns true if connected to the lobby; false otherwise.
         *
         * @note This function should only be called from the logic thread.
         */
        bool IsLobbyConnection() const;

        /**
         * Determine if the active connection is connected to the lobby.
         * @returns true if connected to the lobby; false otherwise.
         *
         * @note This function should only be called from the logic thread.
         */
        bool IsChannelConnection() const;

        /**
         * Get the active connection.
         * @returns The active connection.
         *
         * @note This function should only be called from the logic thread.
         */
        std::shared_ptr<libcomp::EncryptedConnection> GetConnection() const;

    private:
        /**
         * Start authentication with the lobby server.
         */
        void AuthenticateLobby();

        /**
         * Start authentication with the channel server.
         */
        void AuthenticateChannel();

        /**
         * Handle the incoming login reply.
         * @returns true if the packet was parsed correctly; false otherwise.
         */
        bool HandlePacketLobbyLogin(libcomp::ReadOnlyPacket &p);

        /**
         * Handle the incoming auth reply.
         * @returns true if the packet was parsed correctly; false otherwise.
         */
        bool HandlePacketLobbyAuth(libcomp::ReadOnlyPacket &p);

        /**
         * Handle the incoming login reply.
         * @returns true if the packet was parsed correctly; false otherwise.
         */
        bool HandlePacketChannelLogin(libcomp::ReadOnlyPacket &p);

        /**
         * Handle the incoming auth reply.
         * @returns true if the packet was parsed correctly; false otherwise.
         */
        bool HandlePacketChannelAuth(libcomp::ReadOnlyPacket &p);

        /**
         * Setup a new connection.
         * @param conn Connection to setup.
         * @param connectionID ID for the connection.
         * @param host Hostname to connect to.
         * @param port TCP port to connect to on the host.
         * @returns true if the connection has started; false otherwise.
         */
        bool SetupConnection(
            const std::shared_ptr<libcomp::EncryptedConnection> &conn,
            const libcomp::String &connectionID, const libcomp::String &host,
            uint16_t port);

        /**
         * Process a packet message.
         * @param pMessage Packet message to process.
         */
        bool ProcessPacketMessage(const libcomp::Message::Packet *pMessage);

        /**
         * Process a connection message.
         * @param pMessage Connection message to process.
         */
        bool ProcessConnectionMessage(
            const libcomp::Message::ConnectionMessage *pMessage);

        /**
         * Process a client message.
         * @param pMessage Client message to process.
         */
        bool ProcessClientMessage(
            const libcomp::Message::MessageClient *pMessage);

        /// Pointer to the LogicWorker.
        LogicWorker *mLogicWorker;

        /// ASIO service.
        asio::io_service mService;

        /// ASIO service thread.
        std::thread mServiceThread;

        /// Active connection to the lobby or channel server.
        std::shared_ptr<libcomp::EncryptedConnection> mActiveConnection;

        /// Message queue for the LogicWorker.
        std::weak_ptr<libcomp::MessageQueue<libcomp::Message::Message *>>
            mMessageQueue;

        /// Username for authentication.
        libcomp::String mUsername;

        /// Password for authentication.
        libcomp::String mPassword;

        /// Client version for authentication.
        uint32_t mClientVersion;

        /// Session key passed from the lobby.
        uint32_t mSessionKey;

        /// UUID of the machine the client is connecting from.
        libobjgen::UUID mMachineUUID;
    };

} // namespace logic

#endif // LIBCLIENT_SRC_CONNECTIONMANAGER_H
