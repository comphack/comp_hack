/**
 * @file libobjgen/src/MetaObjectXmlParser.h
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief XML parser for one (or many dependent) metadata object(s).
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

#ifndef LIBOBJGEN_SRC_METAOBJECTXMLPARSER_H
#define LIBOBJGEN_SRC_METAOBJECTXMLPARSER_H

#include "MetaObject.h"
#include "MetaVariableReference.h"

 // tinyxml2 Includes
#include "PushIgnore.h"
#include "PopIgnore.h"
#include <tinyxml2.h>

namespace libobjgen
{

class MetaObjectXmlParser
{
public:
    MetaObjectXmlParser();
    ~MetaObjectXmlParser();

    std::string GetError() const;

    bool Load(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement& root);
    bool LoadTypeInformation(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement& root);
    bool LoadMembers(const std::string& object,
        const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement& root);

    bool FinalizeObjectAndReferences(const std::string& object);

    bool SetReferenceFieldDynamicSizes(
        const std::list<std::shared_ptr<libobjgen::MetaVariableReference>>& refs);

    std::shared_ptr<MetaObject> GetCurrentObject() const;
    std::shared_ptr<MetaObject> GetKnownObject(const std::string& object) const;
    std::unordered_map<std::string,
        std::shared_ptr<MetaObject>> GetKnownObjects() const;

protected:
    void SetXMLDefinition(const tinyxml2::XMLElement& root);

    bool LoadMember(const tinyxml2::XMLDocument& doc, const char *szName,
        const tinyxml2::XMLElement *pMember, bool& result);
    std::shared_ptr<MetaVariable> GetVariable(const tinyxml2::XMLDocument& doc,
        const char *szName, const char *szMemberName,
        const tinyxml2::XMLElement *pMember);

private:
    const tinyxml2::XMLElement *GetChild(const tinyxml2::XMLElement *pMember,
        const std::string name) const;

    bool DefaultsSpecified(const tinyxml2::XMLElement *pMember) const;

    bool HasCircularReference(const std::shared_ptr<MetaObject> obj,
        const std::set<std::string>& references) const;

    std::unordered_map<std::string, std::shared_ptr<MetaObject>> mKnownObjects;
    std::unordered_map<std::string, std::string> mObjectXml;

    std::shared_ptr<MetaObject> mObject;
    std::set<std::string> mMemberLoadedObjects;
    std::set<std::string> mFinalizedObjects;
    std::string mError;
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_METAOBJECTXMLPARSER_H
