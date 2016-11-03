/**
 * @file libobjgen/src/MetaObject.h
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for an object.
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

#ifndef LIBOBJGEN_SRC_METAOBJECT_H
#define LIBOBJGEN_SRC_METAOBJECT_H

// Standard C++11 Includes
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

// tinyxml2 Includes
#include "PushIgnore.h"
#include <tinyxml2.h>
#include "PopIgnore.h"

namespace libobjgen
{

class MetaVariable;

class MetaObject
{
public:
    typedef std::list<std::shared_ptr<MetaVariable>> VariableList;
    typedef std::unordered_map<std::string, std::shared_ptr<MetaVariable>>
        VariableMap;

    MetaObject();
    ~MetaObject();

    std::string GetName() const;
    bool SetName(const std::string& name);

    std::string GetBaseObject() const;
    bool SetBaseObject(const std::string& name);

    std::string GetError() const;

    bool AddVariable(const std::string& name,
        const std::shared_ptr<MetaVariable>& var);
    bool RemoveVariable(const std::string& name);

    VariableList::const_iterator VariablesBegin() const;
    VariableList::const_iterator VariablesEnd() const;

    static bool IsValidIdentifier(const std::string& ident);

    bool IsValid() const;

    uint16_t GetDynamicSizeCount() const;

    bool Load(std::istream& stream);
    bool Save(std::ostream& stream) const;

    bool Load(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement& root);
    bool Save(tinyxml2::XMLDocument& doc,
        tinyxml2::XMLElement& root) const;

    bool HasCircularReference() const;

    std::set<std::string> GetReferences() const;

protected:
    static std::shared_ptr<MetaVariable> CreateType(
        const std::string& typeName);

    bool LoadMember(const tinyxml2::XMLDocument& doc, const char *szName,
        const tinyxml2::XMLElement *pMember, bool& result);
    bool LoadArray(const tinyxml2::XMLDocument& doc, const char *szName,
        const tinyxml2::XMLElement *pMember, bool& result);
    bool LoadList(const tinyxml2::XMLDocument& doc, const char *szName,
        const tinyxml2::XMLElement *pMember, bool& result);

private:
    bool HasCircularReference(const std::set<std::string>& references) const;

    std::string mName;
    std::string mBaseObject;
    std::string mError;
    VariableList mVariables;
    VariableMap mVariableMapping;
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_METAOBJECT_H
