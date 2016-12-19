/**
 * @file libcomp/src/DatabaseBind.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to bind a column value to the database query.
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

#include "DatabaseBind.h"

// libcomp Includes
#include "Database.h"

using namespace libcomp;

DatabaseBind::DatabaseBind(const String& column) : mColumn(column)
{
}

DatabaseBind::~DatabaseBind()
{
}

String DatabaseBind::GetColumn() const
{
    return mColumn;
}

void DatabaseBind::SetColumn(const String& column)
{
    mColumn = column;
}

DatabaseBindText::DatabaseBindText(const String& column,
    const String& value) : DatabaseBind(column), mValue(value)
{
}

DatabaseBindText::~DatabaseBindText()
{
}

bool DatabaseBindText::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}

DatabaseBindBlob::DatabaseBindBlob(const String& column,
    const std::vector<char>& value) : DatabaseBind(column), mValue(value)
{
}

DatabaseBindBlob::~DatabaseBindBlob()
{
}

bool DatabaseBindBlob::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}

DatabaseBindUUID::DatabaseBindUUID(const String& column,
    const libobjgen::UUID& value) : DatabaseBind(column), mValue(value)
{
}

DatabaseBindUUID::~DatabaseBindUUID()
{
}

bool DatabaseBindUUID::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}

DatabaseBindInt::DatabaseBindInt(const String& column, int32_t value) :
    DatabaseBind(column), mValue(value)
{
}

DatabaseBindInt::~DatabaseBindInt()
{
}

bool DatabaseBindInt::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}

DatabaseBindBigInt::DatabaseBindBigInt(const String& column, int64_t value) :
    DatabaseBind(column), mValue(value)
{
}

DatabaseBindBigInt::~DatabaseBindBigInt()
{
}

bool DatabaseBindBigInt::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}

DatabaseBindFloat::DatabaseBindFloat(const String& column, float value) :
    DatabaseBind(column), mValue(value)
{
}

DatabaseBindFloat::~DatabaseBindFloat()
{
}

bool DatabaseBindFloat::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}

DatabaseBindDouble::DatabaseBindDouble(const String& column, double value) :
    DatabaseBind(column), mValue(value)
{
}

DatabaseBindDouble::~DatabaseBindDouble()
{
}

bool DatabaseBindDouble::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}

DatabaseBindBool::DatabaseBindBool(const String& column, bool value) :
    DatabaseBind(column), mValue(value)
{
}

DatabaseBindBool::~DatabaseBindBool()
{
}

bool DatabaseBindBool::Bind(DatabaseQuery& db)
{
    return db.Bind(mColumn, mValue);
}
