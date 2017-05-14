/**
 * @file libcomp/src/DatabaseTransaction.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Grouped database updates to be run as a transaction.
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

#ifndef LIBCOMP_SRC_DATABASETRANSACTION_H
#define LIBCOMP_SRC_DATABASETRANSACTION_H

// libcomp Includes
#include "EnumMap.h"
#include "PersistentObject.h"

namespace libcomp
{

/**
 * Error codes used by the game client.
 */
enum DatabaseChangeType_t : uint8_t
{
    DATABASE_INSERT,
    DATABASE_UPDATE,
    DATABASE_DELETE
};

typedef EnumMap<DatabaseChangeType_t,
    std::list<std::shared_ptr<PersistentObject>>> DatabaseChangeMap;

/**
 * Database transaction containing one or more changges grouped
 * by a UUID to be processed at the same time.
 */
class DatabaseTransaction
{
public:
    /**
     * Create a new Database transaction
     */
    DatabaseTransaction()
    {
    }

    /**
     * Create a new Database transaction
     * @param uuid UUID of the grouped transaction changes
     */
    DatabaseTransaction(const libobjgen::UUID& uuid) : mUUID(uuid)
    {
    }

    /**
     * Clean up the database transaction
     */
    ~DatabaseTransaction()
    {
    }

    /**
     * Get the transaction UUID
     * @return UUID associated to the transaction
     */
    const libobjgen::UUID GetUUID()
    {
        return mUUID;
    }

    /**
     * Get the transaction changes
     * @return Changes associated to the transaction
     */
    DatabaseChangeMap& GetChanges()
    {
        return mChanges;
    }

private:
    /// UUID used to group the transaction changes.  Useful
    /// when tying a transaction back to a parent object the
    /// UUID belongs to.
    libobjgen::UUID mUUID;

    /// Changes associated to the transaction
    DatabaseChangeMap mChanges;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASETRANSACTION_H
