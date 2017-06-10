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

#include "DatabaseChangeSet.h"

// libcomp Includes
#include "DatabaseBind.h"

using namespace libcomp;

DatabaseChangeSet::DatabaseChangeSet()
{
}

DatabaseChangeSet::DatabaseChangeSet(const libobjgen::UUID& uuid) :
    mTransactionUUID(uuid)
{
}

DatabaseChangeSet::~DatabaseChangeSet()
{
}

std::shared_ptr<DatabaseChangeSet> DatabaseChangeSet::Create(
    const libobjgen::UUID& uuid)
{
    return std::make_shared<DBStandardChangeSet>(uuid);
}

const libobjgen::UUID DatabaseChangeSet::GetTransactionUUID() const
{
    return mTransactionUUID;
}

DBStandardChangeSet::DBStandardChangeSet()
{
}

DBStandardChangeSet::DBStandardChangeSet(const libobjgen::UUID& uuid) :
    DatabaseChangeSet(uuid)
{
}

DBStandardChangeSet::~DBStandardChangeSet()
{
}

void DBStandardChangeSet::Insert(const std::shared_ptr<PersistentObject>& obj)
{
    mInserts.push_back(obj);
    mInserts.unique();
}

void DBStandardChangeSet::Update(const std::shared_ptr<PersistentObject>& obj)
{
    mUpdates.push_back(obj);
    mUpdates.unique();
}

void DBStandardChangeSet::Delete(const std::shared_ptr<PersistentObject>& obj)
{
    mDeletes.push_back(obj);
    mDeletes.unique();
}

std::list<std::shared_ptr<PersistentObject>> DBStandardChangeSet::GetInserts() const
{
    return mInserts;
}

std::list<std::shared_ptr<PersistentObject>> DBStandardChangeSet::GetUpdates() const
{
    return mUpdates;
}

std::list<std::shared_ptr<PersistentObject>> DBStandardChangeSet::GetDeletes() const
{
    return mDeletes;
}

DBOperationalChange::DBOperationalChange(const std::shared_ptr<PersistentObject>& record,
    DBOperationType type)
    : mType(type), mRecord(record)
{
}

DBOperationalChange::~DBOperationalChange()
{
}

DBOperationalChange::DBOperationType DBOperationalChange::GetType()
{
    return mType;
}

std::shared_ptr<PersistentObject> DBOperationalChange::GetRecord()
{
    return mRecord;
}

DBExplicitUpdate::DBExplicitUpdate(const std::shared_ptr<PersistentObject>& record)
    : DBOperationalChange(record, DBOperationType::DBOP_EXPLICIT)
{
    if(mRecord)
    {
        mMetadata = record->GetObjectMetadata();
        for(auto bind : record->GetMemberBindValues(true, false))
        {
            mStoredValues[bind->GetColumn().C()] = bind;
        }
    }
}

DBExplicitUpdate::~DBExplicitUpdate()
{
    // Delete all the bindings created by the change set
    std::set<DatabaseBind*> bindings;
    for(auto bPair : mStoredValues)
    {
        bindings.insert(bPair.second);
    }

    for(auto bPair : mExpectedValues)
    {
        bindings.insert(bPair.second);
    }

    for(auto bPair : mChanges)
    {
        bindings.insert(bPair.second);
    }

    for(auto bind : bindings)
    {
        delete bind;
    }
}

DatabaseBind* DBExplicitUpdate::Verify(const libcomp::String& column,
    libobjgen::MetaVariable::MetaVariableType_t validType)
{
    std::set<libobjgen::MetaVariable::MetaVariableType_t> types = { validType };
    return Verify(column, types);
}

DatabaseBind* DBExplicitUpdate::Verify(const libcomp::String& column,
    const std::set<libobjgen::MetaVariable::MetaVariableType_t>& validTypes)
{
    auto key = column.C();
    auto var = mMetadata ? mMetadata->GetVariable(key) : nullptr;
    auto existing = mChanges.find(key);
    auto stored = mStoredValues.find(key);
    auto expected = mExpectedValues.find(key);

    return ((var && validTypes.find(var->GetMetaType()) != validTypes.end())
        && existing == mChanges.end()
        && expected == mExpectedValues.end()
        && stored != mStoredValues.end()) ? stored->second : 0;
}

bool DBExplicitUpdate::Bind(const libcomp::String& column, DatabaseBind* val,
    DatabaseBind* expected)
{
    auto varName = column.C();
    mChanges[varName] = val;
    mExpectedValues[varName] = expected;
    return true;
}

std::unordered_map<std::string, DatabaseBind*> DBExplicitUpdate::GetExpectedValues() const
{
    return mExpectedValues;
}

std::unordered_map<std::string, DatabaseBind*> DBExplicitUpdate::GetChanges() const
{
    return mChanges;
}

DBOperationalChangeSet::DBOperationalChangeSet()
{
}

DBOperationalChangeSet::DBOperationalChangeSet(const libobjgen::UUID& uuid) :
    DatabaseChangeSet(uuid)
{
}

DBOperationalChangeSet::~DBOperationalChangeSet()
{
}

std::list<std::shared_ptr<DBOperationalChange>> DBOperationalChangeSet::GetOperations()
{
    return mOperations;
}

void DBOperationalChangeSet::AddOperation(const std::shared_ptr<DBOperationalChange>& op)
{
    mOperations.push_back(op);
}

void DBOperationalChangeSet::Insert(const std::shared_ptr<PersistentObject>& obj)
{
    mOperations.push_back(std::make_shared<DBOperationalChange>(obj,
        DBOperationalChange::DBOperationType::DBOP_INSERT));
}

void DBOperationalChangeSet::Update(const std::shared_ptr<PersistentObject>& obj)
{
    mOperations.push_back(std::make_shared<DBOperationalChange>(obj,
        DBOperationalChange::DBOperationType::DBOP_UPDATE));
}

void DBOperationalChangeSet::Delete(const std::shared_ptr<PersistentObject>& obj)
{
    mOperations.push_back(std::make_shared<DBOperationalChange>(obj,
        DBOperationalChange::DBOperationType::DBOP_DELETE));
}

namespace libcomp
{

static std::set<libobjgen::MetaVariable::MetaVariableType_t> validInt32Types =
    {
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_S8,
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_S16,
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_S32,
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_U8,
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_U16
    };

template<>
bool DBExplicitUpdate::SetFrom<int32_t>(const libcomp::String& column, int32_t value, int32_t expected)
{
    return Verify(column, validInt32Types) && Bind(column, new DatabaseBindInt(column, value),
        new DatabaseBindInt(column, expected));
}

template<>
bool DBExplicitUpdate::Set<int32_t>(const libcomp::String& column, int32_t value)
{
    DatabaseBind* val = Verify(column, validInt32Types);
    return val && SetFrom<int32_t>(column, value, ((DatabaseBindInt*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::AddFrom<int32_t>(const libcomp::String& column, int32_t value, int32_t expected)
{
    return Verify(column, validInt32Types) && Bind(column, new DatabaseBindInt(column, expected + value),
        new DatabaseBindInt(column, expected));
}

template<>
bool DBExplicitUpdate::Add<int32_t>(const libcomp::String& column, int32_t value)
{
    DatabaseBind* val = Verify(column, validInt32Types);
    return val && AddFrom<int32_t>(column, value, ((DatabaseBindInt*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SubtractFrom<int32_t>(const libcomp::String& column, int32_t value, int32_t expected)
{
    return Verify(column, validInt32Types) && Bind(column, new DatabaseBindInt(column, expected - value),
        new DatabaseBindInt(column, expected));
}

template<>
bool DBExplicitUpdate::Subtract<int32_t>(const libcomp::String& column, int32_t value)
{
    DatabaseBind* val = Verify(column, validInt32Types);
    return val && SubtractFrom<int32_t>(column, value, ((DatabaseBindInt*)val)->GetValue());
}

static std::set<libobjgen::MetaVariable::MetaVariableType_t> validInt64Types =
    {
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_S64,
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_U32,
        libobjgen::MetaVariable::MetaVariableType_t::TYPE_U64
    };

template<>
bool DBExplicitUpdate::SetFrom<int64_t>(const libcomp::String& column, int64_t value, int64_t expected)
{
    return Verify(column, validInt64Types) && Bind(column, new DatabaseBindBigInt(column, value),
        new DatabaseBindBigInt(column, expected));
}

template<>
bool DBExplicitUpdate::Set<int64_t>(const libcomp::String& column, int64_t value)
{
    DatabaseBind* val = Verify(column, validInt64Types);
    return val && SetFrom<int64_t>(column, value, ((DatabaseBindBigInt*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::AddFrom<int64_t>(const libcomp::String& column, int64_t value, int64_t expected)
{
    return Verify(column, validInt64Types) && Bind(column, new DatabaseBindBigInt(column, expected + value),
        new DatabaseBindBigInt(column, expected));
}

template<>
bool DBExplicitUpdate::Add<int64_t>(const libcomp::String& column, int64_t value)
{
    DatabaseBind* val = Verify(column, validInt64Types);
    return val && AddFrom<int64_t>(column, value, ((DatabaseBindBigInt*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SubtractFrom<int64_t>(const libcomp::String& column, int64_t value, int64_t expected)
{
    return Verify(column, validInt64Types) && Bind(column, new DatabaseBindBigInt(column, expected - value),
        new DatabaseBindBigInt(column, expected));
}

template<>
bool DBExplicitUpdate::Subtract<int64_t>(const libcomp::String& column, int64_t value)
{
    DatabaseBind* val = Verify(column, validInt64Types);
    return val && SubtractFrom<int64_t>(column, value, ((DatabaseBindBigInt*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SetFrom<float>(const libcomp::String& column, float value, float expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT)
        && Bind(column, new DatabaseBindFloat(column, value),
            new DatabaseBindFloat(column, expected));
}

template<>
bool DBExplicitUpdate::Set<float>(const libcomp::String& column, float value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT);
    return val && SetFrom<float>(column, value, ((DatabaseBindFloat*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::AddFrom<float>(const libcomp::String& column, float value, float expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT)
        && Bind(column, new DatabaseBindFloat(column, expected + value),
            new DatabaseBindFloat(column, expected));
}

template<>
bool DBExplicitUpdate::Add<float>(const libcomp::String& column, float value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT);
    return val && AddFrom<float>(column, value, ((DatabaseBindFloat*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SubtractFrom<float>(const libcomp::String& column, float value, float expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT)
        && Bind(column, new DatabaseBindFloat(column, expected - value),
            new DatabaseBindFloat(column, expected));
}

template<>
bool DBExplicitUpdate::Subtract<float>(const libcomp::String& column, float value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_FLOAT);
    return val && SubtractFrom<float>(column, value, ((DatabaseBindFloat*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SetFrom<double>(const libcomp::String& column, double value, double expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE)
        && Bind(column, new DatabaseBindDouble(column, value),
            new DatabaseBindDouble(column, expected));
}

template<>
bool DBExplicitUpdate::Set<double>(const libcomp::String& column, double value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE);
    return val && SetFrom<double>(column, value, ((DatabaseBindDouble*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::AddFrom<double>(const libcomp::String& column, double value, double expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE)
        && Bind(column, new DatabaseBindDouble(column, expected + value),
            new DatabaseBindDouble(column, expected));
}

template<>
bool DBExplicitUpdate::Add<double>(const libcomp::String& column, double value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE);
    return val && AddFrom<double>(column, value, ((DatabaseBindDouble*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SubtractFrom<double>(const libcomp::String& column, double value, double expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE)
        && Bind(column, new DatabaseBindDouble(column, expected - value),
            new DatabaseBindDouble(column, expected));
}

template<>
bool DBExplicitUpdate::Subtract<double>(const libcomp::String& column, double value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_DOUBLE);
    return val && SubtractFrom<double>(column, value, ((DatabaseBindDouble*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SetFrom<libcomp::String>(const libcomp::String& column, libcomp::String value,
    libcomp::String expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_STRING)
        && Bind(column, new DatabaseBindText(column, value),
            new DatabaseBindText(column, expected));
}

template<>
bool DBExplicitUpdate::Set<libcomp::String>(const libcomp::String& column, libcomp::String value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_STRING);
    return val && SetFrom<libcomp::String>(column, value, ((DatabaseBindText*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SetFrom<bool>(const libcomp::String& column, bool value, bool expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_BOOL)
        && Bind(column, new DatabaseBindBool(column, value),
            new DatabaseBindBool(column, expected));
}

template<>
bool DBExplicitUpdate::Set<bool>(const libcomp::String& column, bool value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_BOOL);
    return val && SetFrom<bool>(column, value, ((DatabaseBindBool*)val)->GetValue());
}

template<>
bool DBExplicitUpdate::SetFrom<libobjgen::UUID>(const libcomp::String& column, libobjgen::UUID value,
    libobjgen::UUID expected)
{
    return Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_BOOL)
        && Bind(column, new DatabaseBindUUID(column, value),
            new DatabaseBindUUID(column, expected));
}

template<>
bool DBExplicitUpdate::Set<libobjgen::UUID>(const libcomp::String& column, libobjgen::UUID value)
{
    DatabaseBind* val = Verify(column, libobjgen::MetaVariable::MetaVariableType_t::TYPE_BOOL);
    return val && SetFrom<libobjgen::UUID>(column, value, ((DatabaseBindUUID*)val)->GetValue());
}

}