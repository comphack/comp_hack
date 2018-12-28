/**
 * @file libcomp/src/BinaryDataNamedSet.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Extended binary data set container that handles generic name
 *  retrieval for selection controls.
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

#ifndef LIBCOMP_SRC_BINARYDATANAMEDSET_H
#define LIBCOMP_SRC_BINARYDATANAMEDSET_H

// libcomp Includes
#include <BinaryDataSet.h>

namespace libcomp
{

class String;

} // namespace libcomp

class BinaryDataNamedSet : public libcomp::BinaryDataSet
{
public:
    BinaryDataNamedSet(std::function<uint32_t(
            const std::shared_ptr<libcomp::Object>&)> mapper);
    BinaryDataNamedSet(
        std::function<std::shared_ptr<libcomp::Object>()> allocator,
        std::function<uint32_t(
		const std::shared_ptr<libcomp::Object>&)> mapper,
        std::function<libcomp::String(
			const std::shared_ptr<libcomp::Object>&)> namer);
    virtual ~BinaryDataNamedSet();

    uint32_t GetMapID(const std::shared_ptr<libcomp::Object>& obj) const;

    libcomp::String GetName(const std::shared_ptr<libcomp::Object>& obj) const;

    void MapRecords(std::vector<std::shared_ptr<libcomp::Object>>& objs,
        std::vector<libcomp::String>& explicitNames);

private:
    std::function<libcomp::String(const std::shared_ptr<
        libcomp::Object>&)> mObjectNamer;

    std::unordered_map<uint32_t, libcomp::String> mExplicitNames;
};

#endif // LIBCOMP_SRC_BINARYDATANAMEDSET_H
