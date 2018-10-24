/**
 * @file libcomp/src/DataSyncManager.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Manages synchronizing data between two or more servers.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#ifndef LIBCOMP_SRC_DATASYNCMANAGER_H
#define LIBCOMP_SRC_DATASYNCMANAGER_H

// libcomp Includes
#include "CString.h"
#include "InternalConnection.h"

// Standard C++11 Includes
#include <set>
#include <unordered_map>

namespace libcomp
{

class Database;
class Packet;
class ReadOnlyPacket;

/**
 * Manager to synchronize data between two or more servers.
 */
class DataSyncManager
{
public:
    /**
     * Create a new DataSyncManager.
     */
    DataSyncManager();

    /**
     * Clean up the DataSyncManager.
     */
    ~DataSyncManager();

    /**
     * Register a new server connection and its sync types. Once registered, all
     * updates will be queued up automatically when updated in the manager.
     * @param connection Pointer to the connection
     * @param types Set of all types that should be sent to the connection when
     *  updated
     * @return false if the connection could not be registered for any reason
     */
    bool RegisterConnection(const std::shared_ptr<
        InternalConnection>& connection, std::set<std::string> types);

    /**
     * Remove a connection from the set of synchronized servers.
     * @param connection Pointer to the connection
     * @return true if the connection was removed successfully
     */
    bool RemoveConnection(const std::shared_ptr<
        InternalConnection>& connection);

    /**
     * Build and send a synchronization request packet for all updated data
     * in the queue. One packet will be sent for each type assigned to each
     * server connection.
     */
    void SyncOutgoing();

    /**
     * Update all data read from the supplied packet on this server and relay
     * to other servers if this is the master server for the type.
     * @param p Packet to read incoming record updates from
     * @param source Identifier for the server sending the updates
     * @return true if the packet was read without issue
     */
    bool SyncIncoming(libcomp::ReadOnlyPacket& p,
        const libcomp::String& source = "");

    /**
     * Update a specific record on the server if this is the master server
     * for the type. If it is not or the change needs to be relayed, queue
     * the update to be sent to the other servers.
     * @param record Pointer to the record being updated
     * @param type Type name of the object being updated
     * @return true if the record was queued for outgoing sync, false if
     *  no additional sync is necessary
     */
    virtual bool UpdateRecord(const std::shared_ptr<libcomp::Object>& record,
        const libcomp::String& type);

    /**
     * Perform a record update and sync to the other servers immediately
     * @param record Pointer to the record being updated
     * @param type Type name of the object being updated
     * @return true if the record was synced, false if no additional sync
     *  is necessary
     */
    bool SyncRecordUpdate(const std::shared_ptr<libcomp::Object>& record,
        const libcomp::String& type);

    /**
     * Remove a specific record on the server if this is the master server
     * for the type. If it is not or the change needs to be relayed, queue
     * the update to be sent to the other servers.
     * @param record Pointer to the record being removed
     * @param type Type name of the object being removed
     * @return true if the record was queued for outgoing sync, false if
     *  no additional sync is necessary
     */
    virtual bool RemoveRecord(const std::shared_ptr<libcomp::Object>& record,
        const libcomp::String& type);

    /**
     * Perform a record removal and sync to the other servers immediately
     * @param record Pointer to the record being removed
     * @param type Type name of the object being removed
     * @return true if the record was synced, false if no additional sync
     *  is necessary
     */
    bool SyncRecordRemoval(const std::shared_ptr<libcomp::Object>& record,
        const libcomp::String& type);

    /**
     * Helper function for quick creation of a record useful for
     * ObjectConfig's BuildHandler property.
     * @return Pointer to a new record of the specified type
     */
    template<class T> std::shared_ptr<libcomp::Object> New()
    {
        return std::make_shared<T>();
    }

    /**
     * Helper function for updating a record useful for ObjectConfig's
     * UpdateHandler property.
     * @param type Type name of the object being removed
     * @param obj Pointer to the record being updated
     * @param isRemove true if the change is a remove, false if it is
     *  an insert or an update
     * @param source Identifier for the server sending the updates
     * @return true if the update was performed without issue
     */
    template<class D, class T> int8_t Update(const libcomp::String& type,
        const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
        const libcomp::String& source)
    {
        if(std::is_base_of<DataSyncManager, D>::value)
        {
            return ((D*)this)->template Update<T>(type, obj, isRemove,
                source);
        }

        return SYNC_FAILED;
    }

    /**
     * Helper function for performing a post record update operation useful
     * for ObjectConfig's SyncCompleteHandler property.
     * @param type Type name of the object being removed
     * @param objs List of pointers to the records being updated and a flag
     *  indicating if the object was being removed
     * @param source Identifier for the server sending the updates
     */
    template<class D, class T> void SyncComplete(const libcomp::String& type,
        const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>& objs,
        const libcomp::String& source)
    {
        if(std::is_base_of<DataSyncManager, D>::value)
        {
            ((D*)this)->template SyncComplete<T>(type, objs, source);
        }
    }

protected:
    /// Internal sync manager code indicating that the update handler
    /// completed without error and the record should be queued for
    /// sync if applicable.
    const static int8_t SYNC_UPDATED = 0;

    /// Internal sync manager code indicating that the update handler
    /// completed without error and the record should NOT be queued
    /// for sync if applicable.
    const static int8_t SYNC_HANDLED = 1;

    /// Internal sync manager code indicating that the update handler
    /// failed to complete successfully.
    const static int8_t SYNC_FAILED = -1;

    /**
     * Container class for a specific object's synchronization configuration
     * for the server.
     */
    class ObjectConfig
    {
    public:
        /**
         * Create a new empty ObjectConfig.
         */
        ObjectConfig() : ServerOwned(false)
        {
        }

        /**
         * Create a new ObjectConfig with properties specified.
         * @param name Type name of the object
         * @param serverOwned true if the server is the master server
         *  for the object
         * @param database Pointer to the database to use when loading
         *  if the object is persistent
         */
        ObjectConfig(const libcomp::String& name, bool serverOwned,
            std::shared_ptr<Database> database = nullptr)
            : Name(name), DB(database), ServerOwned(serverOwned),
            DynamicHandler(false)
        {
        }

        /// Type name of the object
        libcomp::String Name;

        /// Pointer to the database to use when loading if the object
        /// is persistent
        std::shared_ptr<Database> DB;

        /// Specifies that the server this manager belongs to is the
        /// master server
        bool ServerOwned;

        /// Specifies that there is a dynamic update handler that should
        /// always be called when an update is passed to the manager
        bool DynamicHandler;

        /// Pointer to the function to use when the record is being updated.
        /// Parameters are as follows:
        /// 1) Object type name
        /// 2) Pointer to the object record
        /// 3) Indicator that the update is actually a remove
        /// 4) Identification string for the server sending an update
        /// The return value should use the codes in this header
        /// This is not necessary when the object type is persistent.
        std::function<int8_t(DataSyncManager&, const libcomp::String&,
            const std::shared_ptr<libcomp::Object>&, bool,
            const libcomp::String&)> UpdateHandler;

        /// Optional pointer to the function to use when the the complete
        /// record set has been synched.
        /// Parameters are as follows:
        /// 1) Object type name
        /// 2) List of pointers to the object record and flags that indicate
        ///    if the record was removed
        /// 3) Identification string for the server sending an update
        std::function<void(DataSyncManager&, const libcomp::String&,
            const std::list<std::pair<std::shared_ptr<libcomp::Object>, bool>>&,
            const libcomp::String&)> SyncCompleteHandler;

        /// Pointer to the function to use when creating a new record of
        /// the specified type
        /// This is not used when the object type is persistent.
        std::function<std::shared_ptr<libcomp::Object>(
            DataSyncManager&)> BuildHandler;
    };

    /**
     * Build and queue a data sync request based upon the supplied type
     * and record sets.
     * @param type Type name of the object being synchronized
     * @param connection Pointer to the connection to queue the packet on
     * @param updates Set of all inserts and updates that have been made
     * @param removes Set of all removes that have been made
     */
    void QueueOutgoing(const libcomp::String& type,
        const std::shared_ptr<InternalConnection>& connection,
        const std::set<std::shared_ptr<libcomp::Object>>& updates,
        const std::set<std::shared_ptr<libcomp::Object>>& removes);

    /**
     * Write complete outgoing record packet to send from one record.
     * @param p Packet to write the changes to
     * @param isPersistent true if the object's UUID should be written
     *  to the packet, false if the entire object definition should be
     *  written instead
     * @param type Type name of the object being synchronized
     * @param record Record to write to the packet
     */
    void WriteOutgoingRecord(libcomp::Packet& p, bool isPersistent,
        const libcomp::String& type, const std::shared_ptr<
        libcomp::Object>& record);

    /// Map of registered synchronized types by name
    std::unordered_map<std::string,
        std::shared_ptr<ObjectConfig>> mRegisteredTypes;

    /// Server lock for shared resources
    std::mutex mLock;

private:
    /**
     * Write outgoing records to the supplied packet as part of a sync
     * operation.
     * @param p Packet to write the changes to
     * @param isPersistent true if the object's UUID should be written
     *  to the packet, false if the entire object definition should be
     *  written instead
     * @param records Set of records to write to the packet
     */
    void WriteOutgoingRecords(libcomp::Packet& p, bool isPersistent,
        const std::set<std::shared_ptr<libcomp::Object>>& records);

    /// Map of all record inserts and updates queued for synchronization
    std::unordered_map<std::string,
        std::set<std::shared_ptr<libcomp::Object>>> mOutboundUpdates;

    /// Map of all record removes queued for synchronization
    std::unordered_map<std::string,
        std::set<std::shared_ptr<libcomp::Object>>> mOutboundRemoves;

    /// Map of all configurated server connections to their synchronized
    /// object types
    std::unordered_map<std::shared_ptr<InternalConnection>,
        std::set<std::string>> mConnections;
};

} // namspace libcomp

#endif // LIBCOMP_SRC_DATASYNCMANAGER_H