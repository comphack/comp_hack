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

#include "Database.h"

using namespace libcomp;

std::shared_ptr<Database> Database::sMain = nullptr;

Database::~Database()
{
    if(sMain.get() == this)
    {
        sMain = nullptr;
    }
}

bool Database::Execute(const String& query)
{
    return Prepare(query).Execute();
}

String Database::GetLastError() const
{
    return mError;
}

const std::shared_ptr<Database> Database::GetMainDatabase()
{
    return sMain;
}

void Database::SetMainDatabase(std::shared_ptr<Database> database)
{
    sMain = database;
}
