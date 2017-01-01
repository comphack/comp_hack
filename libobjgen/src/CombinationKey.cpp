/**
 * @file libobjgen/src/CombinationKey.cpp
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

#include "CombinationKey.h"

// MetaVariable Types
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Includes
#include <sstream>

using namespace libobjgen;

std::string CombinationKey::GetName() const
{
    return mName;
}

bool CombinationKey::SetName(const std::string& name)
{
    if(MetaObject::IsValidIdentifier(name))
    {
        mName = name;
        return true;
    }

    return false;
}

bool CombinationKey::AddVariable(const std::string& variableName)
{
    if(std::find(mVariables.begin(), mVariables.end(), variableName)
        == mVariables.end())
    {
        mVariables.push_back(variableName);
        return true;
    }

    return false;
}

std::list<std::string> CombinationKey::GetVariables()
{
    return mVariables;
}

bool CombinationKey::IsUnique() const
{
    return mUnique;
}

void CombinationKey::SetUnique(const bool unique)
{
    mUnique = unique;
}

bool CombinationKey::IsValid() const
{
    return MetaObject::IsValidIdentifier(mName) &&
        mVariables.size() > 0;
}

bool CombinationKey::Load(std::istream& stream)
{
    Generator::LoadString(stream, mName);
    stream.read(reinterpret_cast<char*>(&mUnique),
        sizeof(mUnique));
    
    size_t len;
    stream.read(reinterpret_cast<char*>(&len),
        sizeof(len));

    mVariables.clear();
    for(size_t i = 0; i < len; i++)
    {
        std::string val;
        Generator::LoadString(stream, val);
        mVariables.push_back(val);
    }

    return stream.good();
}

bool CombinationKey::Save(std::ostream& stream) const
{
    if(!IsValid())
    {
        return false;
    }

    Generator::SaveString(stream, mName);
    stream.write(reinterpret_cast<const char*>(&mUnique),
        sizeof(mUnique));
    
    size_t len = mVariables.size();
    stream.write(reinterpret_cast<const char*>(&len),
        sizeof(len));

    for(auto val : mVariables)
    {
        Generator::SaveString(stream, val);
    }

    return stream.good();
}

bool CombinationKey::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& root) const
{
    tinyxml2::XMLElement *pKeyElem = doc.NewElement("combokey");
    pKeyElem->SetAttribute("name", mName.c_str());

    if(IsUnique())
    {
        pKeyElem->SetAttribute("unique", "true");
    }

    std::stringstream ss;
    for(auto varName : mVariables)
    {
        if(!ss.str().empty())
        {
            ss << ",";
        }
        ss << varName;
    }
    pKeyElem->SetAttribute("members", ss.str().c_str());

    root.InsertEndChild(pKeyElem);

    return true;
}

bool CombinationKey::LoadCombinationKeyList(std::istream& stream,
    std::unordered_map<std::string, std::shared_ptr<CombinationKey>>& keys)
{
    size_t keyCount;
    stream.read(reinterpret_cast<char*>(&keyCount),
        sizeof(keyCount));

    if(stream.good())
    {
        for(size_t i = 0; i < keyCount; i++)
        {
            auto key = std::shared_ptr<CombinationKey>(new CombinationKey);
            if(!key->Load(stream))
            {
                return false;
            }
            keys[key->GetName()] = key;
        }
    }

    return stream.good() && keyCount == keys.size();
}

bool CombinationKey::SaveCombinationKeyList(std::ostream& stream,
    const std::unordered_map<std::string, std::shared_ptr<CombinationKey>>& keys)
{
    size_t keyCount = keys.size();
    stream.write(reinterpret_cast<const char*>(&keyCount),
        sizeof(keyCount));
    
    if(stream.good())
    {
        for(auto pair : keys)
        {
            if(!pair.second->Save(stream))
            {
                return false;
            }
        }
    }

    return stream.good();
}
