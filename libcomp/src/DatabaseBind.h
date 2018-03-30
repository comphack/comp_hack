/**
 * @file libcomp/src/DatabaseBind.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to bind a column value to the database query.
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

#ifndef LIBCOMP_SRC_DATABASEBIND_H
#define LIBCOMP_SRC_DATABASEBIND_H

// libobjgen Includes
#include "UUID.h"

// libcomp Includes
#include "CString.h"

namespace libcomp
{

class DatabaseQuery;

/**
 * Base class for binding database columns to allow quick access to
 * getting and setting the associated column.
 * @sa PersistentObject::GetMemberBindValues
 */
class DatabaseBind
{
public:
    /**
     * Create a new database column binding.
     * @param column Database column to bind the actions to
     */
    DatabaseBind(const String& column);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBind();

    /**
     * Get the column being bound.
     * @return Column being bound
     */
    String GetColumn() const;

    /**
     * Set the column being bound.
     * @param column being bound
     */
    void SetColumn(const String& column);

    /**
     * Set the binding based on a database specific query
     * implementation.
     * @param db A database specific query
     * @return true on success, false on failure
     */
    virtual bool Bind(DatabaseQuery& db) = 0;

    /**
     * Set the binding based on a database specific query
     * implementation by binding index.
     * @param db A database specific query
     * @param idx Index to bind to
     * @return true on success, false on failure
     */
    virtual bool Bind(DatabaseQuery& db, size_t idx) = 0;

protected:
    /// Column being bound
    String mColumn;
};

/**
 * Text column database binding by column name and value.
 */
class DatabaseBindText : public DatabaseBind
{
public:
    /**
     * Create a new database text column binding with a value.
     * @param column Database text column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindText(const String& column, const String& value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindText();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    String GetValue() const;

private:
    /// String value to bind
    String mValue;
};

/**
 * Blob column database binding by column name and value.
 */
class DatabaseBindBlob : public DatabaseBind
{
public:
    /**
     * Create a new database blob column binding with a value.
     * @param column Database blob column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindBlob(const String& column, const std::vector<char>& value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindBlob();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    std::vector<char> GetValue() const;

private:
    /// Blob value to bind
    std::vector<char> mValue;
};

/**
 * UUID column database binding by column name and value.
 */
class DatabaseBindUUID : public DatabaseBind
{
public:
    /**
     * Create a new database UUID column binding with a value.
     * @param column Database UUID column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindUUID(const String& column, const libobjgen::UUID& value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindUUID();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    libobjgen::UUID GetValue() const;

private:
    /// UUID value to bind
    libobjgen::UUID mValue;
};

/**
 * 32-bit integer column database binding by column name and value.
 */
class DatabaseBindInt : public DatabaseBind
{
public:
    /**
     * Create a new database 32-bit integer column binding with a value.
     * @param column Database 32-bit integer column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindInt(const String& column, int32_t value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindInt();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    int32_t GetValue() const;

private:
    /// 32-bit integer value to bind
    int32_t mValue;
};

/**
 * 64-bit integer column database binding by column name and value.
 */
class DatabaseBindBigInt : public DatabaseBind
{
public:
    /**
     * Create a new database 64-bit integer column binding with a value.
     * @param column Database 64-bit integer column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindBigInt(const String& column, int64_t value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindBigInt();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    int64_t GetValue() const;

private:
    /// 64-bit integer value to bind
    int64_t mValue;
};

/**
 * Float column database binding by column name and value.
 */
class DatabaseBindFloat : public DatabaseBind
{
public:
    /**
     * Create a new database float column binding with a value.
     * @param column Database float column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindFloat(const String& column, float value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindFloat();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    float GetValue() const;

private:
    /// Float value to bind
    float mValue;
};

/**
 * Double column database binding by column name and value.
 */
class DatabaseBindDouble : public DatabaseBind
{
public:
    /**
     * Create a new database double column binding with a value.
     * @param column Database double column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindDouble(const String& column, double value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindDouble();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    double GetValue() const;

private:
    /// Double value to bind
    double mValue;
};

/**
 * Boolean column database binding by column name and value.
 */
class DatabaseBindBool : public DatabaseBind
{
public:
    /**
     * Create a new database boolean column binding with a value.
     * @param column Database boolean column to bind the actions to
     * @param value Value to bind to the column
     */
    DatabaseBindBool(const String& column, bool value);

    /**
     * Clean up the binding.
     */
    virtual ~DatabaseBindBool();

    virtual bool Bind(DatabaseQuery& db);

    virtual bool Bind(DatabaseQuery& db, size_t idx);

    /**
     * Get the value being bound
     * @return The value being bound
     */
    bool GetValue() const;

private:
    /// Boolean value to bind
    bool mValue;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEBIND_H
