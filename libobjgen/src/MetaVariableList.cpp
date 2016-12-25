/**
 * @file libobjgen/src/MetaVariableList.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for a member variable that is a list of variables.
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

#include "MetaVariableList.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Libraries
#include <sstream>

using namespace libobjgen;

MetaVariableList::MetaVariableList(
    const std::shared_ptr<MetaVariable>& elementType) : MetaVariable(),
    mElementType(elementType)
{
}

MetaVariableList::~MetaVariableList()
{
}

size_t MetaVariableList::GetSize() const
{
    return 0;
}

std::shared_ptr<MetaVariable> MetaVariableList::GetElementType() const
{
    return mElementType;
}

MetaVariable::MetaVariableType_t MetaVariableList::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_LIST;
}

std::string MetaVariableList::GetType() const
{
    return "list";
}

bool MetaVariableList::IsCoreType() const
{
    return false;
}

bool MetaVariableList::IsScriptAccessible() const
{
    return mElementType && mElementType->IsScriptAccessible();
}

bool MetaVariableList::IsValid() const
{
    return mElementType && mElementType->IsValid() && !IsLookupKey();
}

bool MetaVariableList::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    return IsValid() && mElementType->Load(stream);
}

bool MetaVariableList::Save(std::ostream& stream) const
{
    MetaVariable::Save(stream);

    return IsValid() && mElementType->Save(stream);
}

bool MetaVariableList::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    return BaseLoad(root) && IsValid();
}

bool MetaVariableList::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());
    
    if(!mElementType)
    {
        return false;
    }

    mElementType->Save(doc, *pVariableElement, "element");

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

uint16_t MetaVariableList::GetDynamicSizeCount() const
{
    return 1;
}

std::string MetaVariableList::GetCodeType() const
{
    if(mElementType)
    {
        std::stringstream ss;
        ss << "std::list<" << mElementType->GetCodeType() << ">";

        return ss.str();
    }
    else
    {
        return std::string();
    }
}

std::string MetaVariableList::GetConstructValue() const
{
    return std::string();
}

std::string MetaVariableList::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;

    std::string code;

    if(mElementType)
    {
        code = mElementType->GetValidCondition(generator, "value", recursive);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_VALID_CODE@"] = code;

            code = generator.ParseTemplate(0, "VariableArrayValidCondition",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableList::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetLoadCode(generator, "element", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
            replacements["@VAR_LOAD_CODE@"] = code;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableListLoad",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableList::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetSaveCode(generator, "element", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_SAVE_CODE@"] = code;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableListSave",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableList::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetLoadRawCode(generator, "element", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
            replacements["@VAR_LOAD_CODE@"] = code;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableListLoadRaw",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableList::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetSaveRawCode(generator, "element", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_SAVE_CODE@"] = code;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableListSaveRaw",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableList::GetXmlLoadCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& node, size_t tabLevel) const
{
    (void)name;

    std::string code;

    if(mElementType)
    {
        std::string elemCode = mElementType->GetXmlLoadCode(generator,
            generator.GetMemberName(mElementType), doc, "element", tabLevel + 1);

        std::map<std::string, std::string> replacements;
        replacements["@VAR_CODE_TYPE@"] = GetCodeType();
        replacements["@NODE@"] = node;
        replacements["@ELEMENT_ACCESS_CODE@"] = elemCode;

        code = generator.ParseTemplate(tabLevel, "VariableListXmlLoad",
            replacements);
    }

    return code;
}

std::string MetaVariableList::GetXmlSaveCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& parent, size_t tabLevel, const std::string elemName) const
{
    std::string code;

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@GETTER@"] = GetInternalGetterCode(generator, name);
        replacements["@VAR_NAME@"] = generator.Escape(GetName());
        replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
        replacements["@VAR_XML_SAVE_CODE@"] = mElementType->GetXmlSaveCode(
            generator, "element", doc, parent, tabLevel + 1, "element");
        replacements["@PARENT@"] = parent;

        code = generator.ParseTemplate(0, "VariableListXmlSave",
            replacements);
    }

    return code;
}

std::string MetaVariableList::GetAccessDeclarations(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessDeclarations(generator,
        object, name, tabLevel);

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@VAR_ARG_TYPE@"] = mElementType->GetArgumentType();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableListAccessDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableList::GetAccessFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessFunctions(generator, object, name);

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@VAR_ARG_TYPE@"] = mElementType->GetArgumentType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << std::endl << generator.ParseTemplate(0, "VariableListAccessFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableList::GetUtilityDeclarations(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@VAR_ARG_TYPE@"] = mElementType->GetArgumentType();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableListUtilityDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableList::GetUtilityFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@VAR_ARG_TYPE@"] = mElementType->GetArgumentType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        auto entryValidation = mElementType->GetValidCondition(generator, "val", true);

        replacements["@ELEMENT_VALIDATION_CODE@"] = entryValidation.length() > 0
            ? entryValidation : "([&]() { (void)val; return true; })()";

        ss << std::endl << generator.ParseTemplate(0, "VariableListUtilityFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableList::GetAccessScriptBindings(const Generator& generator,
    const MetaObject& object, const std::string& name,
    size_t tabLevel) const
{
    std::stringstream ss;

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@VAR_ARG_TYPE@"] = mElementType->GetArgumentType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableListAccessScriptBindings", replacements) << std::endl;
    }

    return ss.str();
}
