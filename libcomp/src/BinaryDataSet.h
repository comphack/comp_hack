/**
 * @file libcomp/src/BinaryDataSet.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage an objgen XML or binary data file.
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

#ifndef LIBCOMP_SRC_BINARYDATASET_H
#define LIBCOMP_SRC_BINARYDATASET_H

// Standard C++11 Includes
#include <map>
#include <memory>

// libcomp Includes
#include <Object.h>

// tinyxml2 Includes
#include <PushIgnore.h>
#include <tinyxml2.h>
#include <PopIgnore.h>

namespace libcomp
{

class BinaryDataSet
{
public:
    BinaryDataSet(std::function<std::shared_ptr<libcomp::Object>()> allocator,
        std::function<uint32_t(const std::shared_ptr<
            libcomp::Object>&)> mapper);

    bool Load(std::istream& file);
    bool Save(std::ostream& file) const;

    bool LoadXml(tinyxml2::XMLDocument& doc);

    std::string GetXml() const;
    std::string GetTabular() const;

    std::list<std::shared_ptr<libcomp::Object>> GetObjects() const;
    std::shared_ptr<libcomp::Object> GetObjectByID(uint32_t id) const;

private:
    std::list<std::string> ReadNodes(tinyxml2::XMLElement *node,
        int16_t dataMode) const;

    std::function<std::shared_ptr<libcomp::Object>()> mObjectAllocator;
    std::function<uint32_t(const std::shared_ptr<
        libcomp::Object>&)> mObjectMapper;

    std::list<std::shared_ptr<libcomp::Object>> mObjects;
    std::map<uint32_t, std::shared_ptr<libcomp::Object>> mObjectMap;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_BINARYDATASET_H
