/**
 * @file server/channel/src/ManagerConnection.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager to handle channel connections to the world server.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_MANAGERCONNECTION_H
#define SERVER_CHANNEL_SRC_MANAGERCONNECTION_H

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
class ManagerConnection : public libcomp::Manager
{
public:
    /**
     * Create a new manager.
     * @param server Pointer to the server that uses this manager
     */
    ManagerConnection(std::weak_ptr<libcomp::BaseServer> server);

    /**
     * Clean up the manager.
     */
    virtual ~ManagerConnection();

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

    /**
     * Send a request to the connected world for information
     *  to be handled once the response is received.
     */
    void RequestWorldInfo();

    /**
     * Get the world connection.
     * @return Pointer to the world connection
     */
    const std::shared_ptr<libcomp::InternalConnection> GetWorldConnection() const;

    /**
     * Set the world connection after establishing a connection.
     * @param worldConnection Pointer to the world connection
     */
    void SetWorldConnection(const std::shared_ptr<libcomp::InternalConnection>& worldConnection);

    /**
     * Get client connection by username.
     * @param username Client account username
     * @return Pointer to the client connection
     */
    const std::shared_ptr<ChannelClientConnection> GetClientConnection(
        const libcomp::String& username);

    /**
     * Set an active client connection after its account has
     * been detected.
     * @param client Pointer to the client connection
     */
    void SetClientConnection(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Remove a client connection.
     * @param client Pointer to the client connection
     */
    void RemoveClientConnection(const std::shared_ptr<
        ChannelClientConnection>& client);

    /**
     * Get the client connection associated to the supplied entity ID.
     * @param id Entity ID or world ID associated to the client
     *  state to retrieve
     * @param worldID true if the ID is from the world, false if it is a
     *  local entity ID
     * @return Pointer to the client connection associated to the ID or
     *  nullptr if it does not exist
     */
    const std::shared_ptr<ChannelClientConnection>
        GetEntityClient(int32_t id, bool worldID = false);

    /**
     * Read a list of world CIDs from the supplied packet and convert them
     * to client connections.
     * @param p Packet containing the connection list
     * @param success Output parameter which specifies that the packet was read
     *  was successful
     * @return List of client connections associated to the world CIDs in the
     *  packet
     */
    std::list<std::shared_ptr<ChannelClientConnection>>
        GatherWorldTargetClients(libcomp::ReadOnlyPacket& p, bool& success);

    /**
     * Get all connections associated to the supplied connection's party
     * currently on the channel, optionally restricted to the same zone
     * @param client Pointer to the source client connection
     * @param includeSelf If true the source connection will be included
     *  in the returned set
     * @param zoneRestrict If true the connections will be restricted to
     *  only entities in the same zone
     * @return List of client connections in the same party
     */
    std::list<std::shared_ptr<ChannelClientConnection>>
        GetPartyConnections(const std::shared_ptr<
        ChannelClientConnection>& client, bool includeSelf, bool zoneRestrict);

    /**
     * Broadcast the supplied packet to each client connection stored.
     * Use sparingly!
     * @param packet Packet to send
     */
    void BroadcastPacketToClients(libcomp::Packet& packet);

    /**
     * Schedule future server work to execute HandleClientTimeouts every
     * 10 seconds.
     * @param timeout Time in seconds that needs to pass for a client
     *  connection to time out
     * @return true if the work was scheduled, false if it was not
     */
    bool ScheduleClientTimeoutHandler(uint16_t timeout);

    /**
     * Cycle through the current client connections and disconnect clients
     * that not pinged the server for a while.
     * @param now The current server time used to check for timeouts
     * @param timeout Time in seconds that needs to pass for a client
     *  connection to time out
     */
    void HandleClientTimeouts(uint64_t now, uint16_t timeout);

private:
    /// Static list of supported message types for the manager.
    static std::list<libcomp::Message::MessageType> sSupportedTypes;

    /// Pointer to the world connection.
    std::shared_ptr<libcomp::InternalConnection> mWorldConnection;

    /// Map of active client connections by account username
    std::unordered_map<libcomp::String,
        std::shared_ptr<ChannelClientConnection>> mClientConnections;

    /// Pointer to the server that uses this manager.
    std::weak_ptr<libcomp::BaseServer> mServer;

    /// Server lock for shared resources
    std::mutex mLock;
};

} // namespace world

#endif // SERVER_CHANNEL_SRC_MANAGERCONNECTION_H
