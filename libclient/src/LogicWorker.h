/**
 * @file libcomp/src/LogicWorker.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Worker for client<==>server interaction.
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

#ifndef LIBCLIENT_SRC_LOGICWORKER_H
#define LIBCLIENT_SRC_LOGICWORKER_H

// libcomp Includes
#include <Object.h>
#include <Packet.h>
#include <Worker.h>

namespace logic
{

//
// Forward declaration of managers.
//
class ConnectionManager;

/**
 * Worker for client<==>server interaction.
 */
class LogicWorker : public libcomp::Worker
{
public:
    /**
     * Create a new worker.
     */
    LogicWorker();

    /**
     * Cleanup the worker.
     */
    virtual ~LogicWorker();

    /**
     * Sent a message to the GameWorker message queue.
     * @param pMessage Message to send to the GameWorker.
     * @returns true if the message was sent; false otherwise.
     */
    bool SendToGame(libcomp::Message::Message *pMessage);

    /**
     * Set the message queue for the GameWorker. This message queue is used
     * to send events to the game thread. Get the worker by calling
     * @ref Worker::GetMessageQueue on the GameWorker.
     * @param messageQueue Reference to the message queue of the GameWorker.
     */
    void SetGameQueue(const std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>>& messageQueue);

    /**
     * Queue a packet and then send all queued packets to the remote host.
     * @param packet Packet to send to the remote host.
     */
    void SendPacket(libcomp::Packet& packet);

    /**
     * Queue a packet and then send all queued packets to the remote host.
     * @param packet Packet to send to the remote host.
     */
    void SendPacket(libcomp::ReadOnlyPacket& packet);

    /**
     * Queue packets and then send all queued packets to the remote host.
     * @param packets Packets to send to the remote host.
     */
    void SendPackets(const std::list<libcomp::Packet*>& packets);

    /**
     * Queue packets and then send all queued packets to the remote host.
     * @param packets Packets to send to the remote host.
     */
    void SendPackets(const std::list<libcomp::ReadOnlyPacket*>& packets);

    /**
     * Packetize and queue an object and then send all queued packets to the
     *   remote host.
     * @param obj Object to be packetized.
     * @return true if the object could be packetized; false otherwise.
     */
    bool SendObject(std::shared_ptr<libcomp::Object>& obj);

    /**
     * Packetize and queue objects and then send all queued packets to the
     *   remote host.
     * @param objs Objects to be packetized.
     * @return true if the objects could be packetized; false otherwise.
     */
    bool SendObjects(const std::list<std::shared_ptr<libcomp::Object>>& objs);

private:
    /// Manager for the client connection.
    ConnectionManager *mConnectionManager;

    /// Message queue for the GameWorker. Events are sent here.
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mGameMessageQueue;
};

} // namespace logic

#endif // LIBCLIENT_SRC_LOGICWORKER_H
