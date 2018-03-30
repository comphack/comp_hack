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

#ifndef LIBCOMP_SRC_DATABASECHANGESET_H
#define LIBCOMP_SRC_DATABASECHANGESET_H

// libcomp Includes
#include "PersistentObject.h"

// libobjgen Includes
#include "MetaObject.h"
#include "MetaVariable.h"

namespace libcomp
{

class DataBind;
class PersistentObject;

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
    DatabaseChangeSet();

    /**
     * Create a new database change set
     * @param uuid UUID of the grouped transaction changes
     */
    DatabaseChangeSet(const libobjgen::UUID& uuid);

    /**
     * Clean up the database change set
     */
    virtual ~DatabaseChangeSet();

    /**
     * Static builder for a new standard DatabaseChangeSet pointer
     * @param uuid Optional UUID associated to the transaction
     * @return Pointer to a new DatabaseChangeSet
     */
    static std::shared_ptr<DatabaseChangeSet> Create(
        const libobjgen::UUID& uuid = NULLUUID);

    /**
     * Add an object for insertion to the change set
     * @param obj Pointer to the object to insert
     */
    virtual void Insert(const std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Add an object for update to the change set
     * @param obj Pointer to the object to update
     */
    virtual void Update(const std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Add an object for deletion to the change set
     * @param obj Pointer to the object to delete
     */
    virtual void Delete(const std::shared_ptr<PersistentObject>& obj) = 0;

    /**
     * Get the transaction UUID
     * @return UUID associated to the transaction
     */
    const libobjgen::UUID GetTransactionUUID() const;

private:
    /// UUID used to group the transaction changes.  Useful
    /// when tying a transaction back to a parent object the
    /// UUID belongs to.
    libobjgen::UUID mTransactionUUID;
};

/**
 * Standard database change set consisting of inserts, updates or
 * deletes which will be processed in that order, NOT the order they
 * were added in. Here is some normal usecase example code:
 * @code
 * // Using objects::Item instances "item1", "item2" and "item3" and database
 * // instance "db"
 *
 * // Create a standard change set using the static create function
 * // This is equivalent to:
 * // auto changeset = std::make_shared<libcomp::DBStandardChangeSet>();
 * auto changeset = libcomp::DatabaseChangeSet::Create();
 *
 * // Mark new object item1 as an insert
 * changeset->Insert(item1);
 *
 * // Mark existing object item2 as an update
 * changeset->Update(item2);
 *
 * // Mark existing object item3 as a delete
 * changeset->Delete(item3);
 *
 * // Process all 3 at once as an all or nothing operation. It doesn't
 * // matter in this instance what order they are processed in as they
 * // do not have fields referencing one another but the order would be
 * // item1 inserted, item2 updated, item3 deleted
 * db.ProcessChangeSet(changeset);
 * @endcode
 */
class DBStandardChangeSet : public DatabaseChangeSet
{
public:
    /**
     * Create a new standard database change set
     */
    DBStandardChangeSet();

    /**
     * Create a new standard database change set
     * @param uuid UUID of the grouped transaction changes
     */
    DBStandardChangeSet(const libobjgen::UUID& uuid);

    /**
     * Clean up the database change set
     */
    virtual ~DBStandardChangeSet();

    virtual void Insert(const std::shared_ptr<PersistentObject>& obj);
    virtual void Update(const std::shared_ptr<PersistentObject>& obj);
    virtual void Delete(const std::shared_ptr<PersistentObject>& obj);

    /**
     * Get the change set inserts
     * @return Inserts associated to the change set
     */
    std::list<std::shared_ptr<PersistentObject>> GetInserts() const;

    /**
     * Get the change set updates
     * @return Updates associated to the change set
     */
    std::list<std::shared_ptr<PersistentObject>> GetUpdates() const;

    /**
     * Get the change set deletes
     * @return Deletes associated to the change set
     */
    std::list<std::shared_ptr<PersistentObject>> GetDeletes() const;

private:
    /// Inserts associated to the change set
    std::list<std::shared_ptr<PersistentObject>> mInserts;

    /// Updates associated to the change set
    std::list<std::shared_ptr<PersistentObject>> mUpdates;

    /// Deletes associated to the change set
    std::list<std::shared_ptr<PersistentObject>> mDeletes;
};

/**
 * Represents a single operation that can be strung together
 * with others to form a database agnostic complex query.
 */
class DBOperationalChange
{
public:
    /**
     * Specifies the type of operation is contained within a
     * DBOperationalChange
     */
    enum class DBOperationType
    {
        DBOP_INSERT,    //!< Record insert using in memory values
        DBOP_UPDATE,    //!< Record update using in memory values
        DBOP_DELETE,    //!< Record delete by UUID
        DBOP_EXPLICIT,  //!< Explicitly defined database-side operation
    };

    /**
     * Create a new DBOperationalChange
     * @param record Pointer to the affected record
     * @param type Type of operation
     */
    DBOperationalChange(const std::shared_ptr<PersistentObject>& record,
        DBOperationType type);

    /**
     * Clean up the DBOperationalChange
     */
    virtual ~DBOperationalChange();

    /**
     * Get the change's DBOperationType
     * @return The change's DBOperationType
     */
    DBOperationType GetType();

    /**
     * Get the affected record
     * @return Pointer to the affected record
     */
    std::shared_ptr<PersistentObject> GetRecord();

protected:
    /// Type of operational change
    DBOperationType mType;

    /// Pointer to the affected record
    std::shared_ptr<PersistentObject> mRecord;
};

/**
 * Explicit operational update to a record containing one or many
 * field updates that are bound to match an expected "pre-update" value.
 * Useful in handling sensitive operations within transactions that
 * are prone to race conditions.
 */ 
class DBExplicitUpdate : public DBOperationalChange
{
public:
    /**
     * Create a new DBExplicitUpdate
     * @param record Pointer to the affected record
     */
    DBExplicitUpdate(const std::shared_ptr<PersistentObject>& record);

    /**
     * Clean up the DBExplicitUpdate, deleting all the bindings
     * generated for its execution
     */
    virtual ~DBExplicitUpdate();

    /**
     * Prepares an update to the specificed column to use the supplied
     * value based upon the current record value being set in the DB.
     * @param column Column to bind a value to
     * @param value Value to bind to the column
     * @return true if the update is valid, false if it is not
     */
    template <typename T>
    bool Set(const libcomp::String& column, T value);

    /**
     * Prepares an update to the specificed column to use the supplied
     * value based upon an expcted value being set in the DB.
     * @param column Column to bind a value to
     * @param value Value to bind to the column
     * @param expected Value that is expected to be set on the record
     *  when the operation is run
     * @return true if the update is valid, false if it is not
     */
    template <typename T>
    bool SetFrom(const libcomp::String& column, T value, T expected);

    /**
     * Prepares an update to the specificed column to increase by the supplied
     * value based upon the current record value being set in the DB.
     * @param column Column to bind a value to
     * @param value Value to bind to the column via addition
     * @return true if the update is valid, false if it is not
     */
    template <typename T>
    bool Add(const libcomp::String& column, T value);

    /**
     * Prepares an update to the specificed column to increase by the supplied
     * value based upon an expcted value being set in the DB.
     * @param column Column to bind a value to
     * @param value Value to bind to the column via addition
     * @param expected Value that is expected to be set on the record
     *  when the operation is run
     * @return true if the update is valid, false if it is not
     */
    template <typename T>
    bool AddFrom(const libcomp::String& column, T value, T expected);

    /**
     * Prepares an update to the specificed column to decrease by the supplied
     * value based upon the current record value being set in the DB.
     * @param column Column to bind a value to
     * @param value Value to bind to the column via subtraction
     * @return true if the update is valid, false if it is not
     */
    template <typename T>
    bool Subtract(const libcomp::String& column, T value);

    /**
     * Prepares an update to the specificed column to decrease by the supplied
     * value based upon an expcted value being set in the DB.
     * @param column Column to bind a value to
     * @param value Value to bind to the column via subtraction
     * @param expected Value that is expected to be set on the record
     *  when the operation is run
     * @return true if the update is valid, false if it is not
     */
    template <typename T>
    bool SubtractFrom(const libcomp::String& column, T value, T expected);

    /**
     * Get the column bindings to match upon when checking if the update is valid
     * @return Bindings representing the expected values before the update
     */
    std::unordered_map<std::string, DatabaseBind*> GetExpectedValues() const;

    /**
     * Get the column bindings to set if the update is valid
     * @return Bindings representing the expected values after the update
     */
    std::unordered_map<std::string, DatabaseBind*> GetChanges() const;

private:
    /**
     * Verify that the supplied column is not already bound and is a real
     * column of the supplied type and returns the current record value
     * binding upon success.
     * @param column Name of the column to verify
     * @param validType Type of the column to verify
     * @return Pointer to the current record value binding or null on faliure
     */
    DatabaseBind* Verify(const libcomp::String& column,
        libobjgen::MetaVariable::MetaVariableType_t validType);

    /**
     * Verify that the supplied column is not already bound and is a real
     * column of one of the supplied types and returns the current record value
     * binding upon success.
     * @param column Name of the column to verify
     * @param validTypes Valid types of the column to verify
     * @return Pointer to the current record value binding or null on faliure
     */
    DatabaseBind* Verify(const libcomp::String& column,
        const std::set<libobjgen::MetaVariable::MetaVariableType_t>& validTypes);

    /**
     * Stores the bindings of the supplied new and expected values to bind
     * to a database query during execution.
     * @param column Name of the column
     * @param binding Binding reprsenting the new value to set
     * @param expected Binding reprsenting the value expected during execution
     * @return true on success, false on failure
     */
    bool Bind(const libcomp::String& column, DatabaseBind* binding,
        DatabaseBind* expected);

    /// Map of values set on the record at the time of the change set's
    /// creation by column name
    std::unordered_map<std::string, DatabaseBind*> mStoredValues;

    /// Map of values for columns to check before update by column name
    std::unordered_map<std::string, DatabaseBind*> mExpectedValues;

    /// Map of values to set during execution by column name
    std::unordered_map<std::string, DatabaseBind*> mChanges;

    /// Pointer to the metadata of the affected record
    std::shared_ptr<libobjgen::MetaObject> mMetadata;
};

/**
 * Operational database change set consisting of inserts, updates,
 * deletes or explicit updates which will be processed in the order
 * they were added in. Here is some normal usecase example code:
 * @code
 * // Using objects::Item instance "item", objects::Inventory instance
 * // "inventory", related objects::Account instance "account" as well
 * // as database instance "db"
 *
 * // Create the changeset
 * auto opChangeset = std::make_shared<libcomp::DBOperationalChangeSet>();
 *
 * // Create an explicit update for the account, spending 5 CP. This
 * // assumes account has at least 5 CP in memory before attempting this logic
 * auto expl = std::make_shared<libcomp::DBExplicitUpdate>(account);
 * expl->SubtractFrom<int64_t>("CP", 5, acccount->GetCP() - 5);
 *
 * // Add the update to the operation set
 * opChangeset->AddOperation(expl);
 *
 * // Insert the item which has already been allocated a box slot
 * opChangeset->Insert(item);
 *
 * // Update the inventory which has already had its box slot set in memory
 * opChangeset->Update(inventory);
 *
 * // Process the 3 changes as an all or nothing operation in the order
 * // they were specified.
 * if(!db.ProcessChangeSet(opChangeset))
 * {
 *     // Handle any failure reporting here. If it fails, the account will
 *     // be in the state it was already in, the item will not be inserted
 *     // and the inventory will have reloaded with its current database values
 * }
 * @endcode
 */
class DBOperationalChangeSet : public DatabaseChangeSet
{
public:
    /**
     * Create a new operational database change set
     */
    DBOperationalChangeSet();

    /**
     * Create a new operational database change set
     * @param uuid UUID of the grouped transaction changes
     */
    DBOperationalChangeSet(const libobjgen::UUID& uuid);

    /**
     * Clean up the database change set
     */
    virtual ~DBOperationalChangeSet();

    /**
     * Get the list of changes contained in the change set
     * @return List of changes contained in the change set
     */
    std::list<std::shared_ptr<DBOperationalChange>> GetOperations();

    /**
     * Add either a standard or explicit operational change to
     * the set
     * @param op Operational change to add to the change set
     */
    void AddOperation(const std::shared_ptr<DBOperationalChange>& op);

    virtual void Insert(const std::shared_ptr<PersistentObject>& obj);
    virtual void Update(const std::shared_ptr<PersistentObject>& obj);
    virtual void Delete(const std::shared_ptr<PersistentObject>& obj);

private:
    /// List of changes contained in the change set
    std::list<std::shared_ptr<DBOperationalChange>> mOperations;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASECHANGESET_H
