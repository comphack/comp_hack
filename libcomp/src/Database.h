/**
 * @file libcomp/src/Database.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class to handle the database.
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

#ifndef LIBCOMP_SRC_DATABASE_H
#define LIBCOMP_SRC_DATABASE_H

// libcomp Includes
#include "CString.h"
#include "DatabaseQuery.h"

namespace libcomp
{

class Database
{
public:
    virtual bool Open(const String& address, const String& username = String(),
        const String& password = String()) = 0;
    virtual bool Close() = 0;
    virtual bool IsOpen() const = 0;

    virtual DatabaseQuery Prepare(const String& query) = 0;
    virtual bool Execute(const String& query);
    virtual bool Setup() = 0;
    virtual bool Use() = 0;

    String GetLastError() const;

protected:
    String mError;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASE_H
