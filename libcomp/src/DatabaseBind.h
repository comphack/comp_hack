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

#ifndef LIBCOMP_SRC_DATABASEBIND_H
#define LIBCOMP_SRC_DATABASEBIND_H

// libcomp Includes
#include "CString.h"

namespace libcomp
{

class DatabaseQuery;

class DatabaseBind
{
public:
    DatabaseBind(const String& column);
    virtual ~DatabaseBind();

    String GetColumn() const;
    void SetColumn(const String& column);

    virtual bool Bind(DatabaseQuery& db) = 0;

protected:
    String mColumn;
};

class DatabaseBindText : public DatabaseBind
{
public:
    DatabaseBindText(const String& column, const String& value);
    virtual ~DatabaseBindText();

    virtual bool Bind(DatabaseQuery& db);

private:
    String mValue;
};

class DatabaseBindBlob : public DatabaseBind
{
public:
    DatabaseBindBlob(const String& column, const std::vector<char>& value);
    virtual ~DatabaseBindBlob();

    virtual bool Bind(DatabaseQuery& db);

private:
    std::vector<char> mValue;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEBIND_H
