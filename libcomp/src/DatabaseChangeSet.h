/**
 * @file libcomp/src/DatabaseChangeSet.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Grouped database changes to be run via a queue and/or
 *	as a transaction.
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

#ifndef LIBCOMP_SRC_DATABASECHANGESET_H
#define LIBCOMP_SRC_DATABASECHANGESET_H

// libcomp Includes
#include "PersistentObject.h"

namespace libcomp
{

/**
 * Database change set grouped by a transaction UUID to be processed
 * as one operation.
 */
class DatabaseChangeSet
{
public:
    /**
     * Create a new database change set
     */
    DatabaseChangeSet()
    {
    }

    /**
     * Create a new database change set
     * @param uuid UUID of the grouped transaction changes
     */
    DatabaseChangeSet(const libobjgen::UUID& uuid) :
        mTransactionUUID(uuid)
    {
    }

    /**
     * Clean up the database change set
     */
    ~DatabaseChangeSet()
    {
    }

    /**
     * Static builder for a new DatabaseChangeSet pointer
     * @param uuid Optional UUID associated to the transaction
     * @return Pointer to a new DatabaseChangeSet
     */
    static std::shared_ptr<DatabaseChangeSet> Create(
        const libobjgen::UUID& uuid = NULLUUID)
    {
        return std::shared_ptr<DatabaseChangeSet>(
            new DatabaseChangeSet(uuid));
    }

    /**
     * Add an object for insertion to the change set
     * @param obj Pointer to the object to insert
     */
    void Insert(const std::shared_ptr<PersistentObject>& obj)
    {
        mInserts.push_back(obj);
        mInserts.unique();
    }

    /**
     * Add an object for update to the change set
     * @param obj Pointer to the object to update
     */
    void Update(const std::shared_ptr<PersistentObject>& obj)
    {
        mUpdates.push_back(obj);
        mUpdates.unique();
    }

    /**
     * Add an object for deletion to the change set
     * @param obj Pointer to the object to delete
     */
    void Delete(const std::shared_ptr<PersistentObject>& obj)
    {
        mDeletes.push_back(obj);
        mDeletes.unique();
    }

    /**
     * Get the transaction UUID
     * @return UUID associated to the transaction
     */
    const libobjgen::UUID GetTransactionUUID() const
    {
        return mTransactionUUID;
    }

    /**
     * Get the transaction inserts
     * @return Inserts associated to the transaction
     */
    std::list<std::shared_ptr<PersistentObject>> GetInserts() const
    {
        return mInserts;
    }

    /**
     * Get the transaction updates
     * @return Updates associated to the transaction
     */
    std::list<std::shared_ptr<PersistentObject>> GetUpdates() const
    {
        return mUpdates;
    }

    /**
     * Get the transaction deletes
     * @return Deletes associated to the transaction
     */
    std::list<std::shared_ptr<PersistentObject>> GetDeletes() const
    {
        return mDeletes;
    }

private:
    /// UUID used to group the transaction changes.  Useful
    /// when tying a transaction back to a parent object the
    /// UUID belongs to.
    libobjgen::UUID mTransactionUUID;

    /// Inserts associated to the change set
    std::list<std::shared_ptr<PersistentObject>> mInserts;

    /// Updates associated to the change set
    std::list<std::shared_ptr<PersistentObject>> mUpdates;

    /// Deletes associated to the change set
    std::list<std::shared_ptr<PersistentObject>> mDeletes;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASECHANGESET_H
