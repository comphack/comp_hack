/**
 * @file libobjgen/src/CombinationKey.h
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief Combination key for multiple variables on an object.
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

#ifndef LIBOBJGEN_SRC_COMBINATIONKEY_H
#define LIBOBJGEN_SRC_COMBINATIONKEY_H

// Standard C++11 Includes
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

// tinyxml2 Includes
#include <tinyxml2.h>

namespace libobjgen
{

class CombinationKey
{
public:
    CombinationKey() { }

    std::string GetName() const;
    bool SetName(const std::string& name);

    bool AddVariable(const std::string& variableName);
    std::list<std::string> GetVariables();

    bool IsUnique() const;
    void SetUnique(const bool unique);

    bool IsValid() const;

    bool Load(std::istream& stream);
    bool Save(std::ostream& stream) const;
    bool Save(tinyxml2::XMLDocument& doc,
        tinyxml2::XMLElement& root) const;

    static bool LoadCombinationKeyList(std::istream& stream,
        std::unordered_map<std::string, std::shared_ptr<CombinationKey>>& keys);
    static bool SaveCombinationKeyList(std::ostream& stream,
        const std::unordered_map<std::string, std::shared_ptr<CombinationKey>>& keys);

private:
    std::string mName;
    bool mUnique;

    std::list<std::string> mVariables;
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_COMBINATIONKEY_H
