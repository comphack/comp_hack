/**
 * @file libcomp/src/SqratInt64.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Squirrel binding for 64-bit integers.
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

#ifndef LIBCOMP_SRC_SQRATINT64_H
#define LIBCOMP_SRC_SQRATINT64_H

// squirrel Includes
#include <squirrel.h>

// libcomp Includes
#include "CString.h"

namespace Sqrat
{

class s64
{
private:
    uint64_t mValue;

public:
    s64();
    s64(int32_t value);
    s64(const s64& other);

    static SQInteger _cmp(HSQUIRRELVM vm);
    static SQInteger _add(HSQUIRRELVM vm);
    static SQInteger _sub(HSQUIRRELVM vm);
    static SQInteger _mul(HSQUIRRELVM vm);
    static SQInteger _div(HSQUIRRELVM vm);
    static SQInteger _mod(HSQUIRRELVM vm);

    libcomp::String _tostring() const;

    void set(int64_t value);
    int64_t value() const;
    int32_t valueTruncated() const;

    static SQInteger equal(HSQUIRRELVM vm);
};

class u64
{
private:
    uint64_t mValue;

public:
    u64();
    u64(int32_t value);
    u64(const u64& other);

    static SQInteger _cmp(HSQUIRRELVM vm);
    static SQInteger _add(HSQUIRRELVM vm);
    static SQInteger _sub(HSQUIRRELVM vm);
    static SQInteger _mul(HSQUIRRELVM vm);
    static SQInteger _div(HSQUIRRELVM vm);
    static SQInteger _mod(HSQUIRRELVM vm);

    libcomp::String _tostring() const;

    void set(uint64_t value);
    uint64_t value() const;
    int32_t valueTruncated() const;

    static SQInteger equal(HSQUIRRELVM vm);
};

} // namespace Sqrat

#endif // LIBCOMP_SRC_SQRATINT64_H
