/**
 * @file libobjgen/src/UUID.h
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to handle a UUID.
 *
 * This file is part of the COMP_hack Object Generator Library (libobjgen).
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

#ifndef LIBOBJGEN_SRC_UUID_H
#define LIBOBJGEN_SRC_UUID_H

// Standard Includes
#include <stdint.h>

// Standard C++ Includes
#include <string>
#include <vector>

#define NULLUUID libobjgen::UUID()

namespace libobjgen
{

class UUID
{
public:
    UUID();
    UUID(const std::string& other);
    UUID(const std::vector<char>& data);

    static UUID Random();

    std::string ToString() const;
    std::vector<char> ToData() const;

    bool IsNull() const;

    bool operator==(UUID other) const;
    bool operator!=(UUID other) const;

private:
    uint64_t mTimeAndVersion;
    uint64_t mClockSequenceAndNode;
};

} // namespace libcomp

#endif // LIBOBJGEN_SRC_UUID_H
