/**
 * @file libobjgen/src/MetaVariableSet.cpp
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief Metadata for a member variable that is a variable set.
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

#include "MetaVariableSet.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Libraries
#include <sstream>

using namespace libobjgen;

MetaVariableSet::MetaVariableSet(
    const std::shared_ptr<MetaVariable>& elementType) : MetaVariable(),
    mElementType(elementType)
{
}

MetaVariableSet::~MetaVariableSet()
{
}

size_t MetaVariableSet::GetSize() const
{
    return 0;
}

std::shared_ptr<MetaVariable> MetaVariableSet::GetElementType() const
{
    return mElementType;
}

MetaVariable::MetaVariableType_t MetaVariableSet::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_SET;
}

std::string MetaVariableSet::GetType() const
{
    return "set";
}

bool MetaVariableSet::IsCoreType() const
{
    return false;
}

bool MetaVariableSet::IsScriptAccessible() const
{
    return mElementType && mElementType->IsScriptAccessible();
}

bool MetaVariableSet::IsValid() const
{
    return mElementType && mElementType->IsValid() && !IsLookupKey();
}

bool MetaVariableSet::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    return stream.good() && mElementType->Load(stream) && IsValid();
}

bool MetaVariableSet::Save(std::ostream& stream) const
{
    MetaVariable::Save(stream);

    return IsValid() && mElementType->Save(stream);
}

bool MetaVariableSet::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    return BaseLoad(root) && IsValid();
}

bool MetaVariableSet::Save(tinyxml2::XMLDocument& doc,
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

uint16_t MetaVariableSet::GetDynamicSizeCount() const
{
    return 1;
}

std::string MetaVariableSet::GetCodeType() const
{
    if(mElementType)
    {
        std::stringstream ss;
        ss << "std::set<" << mElementType->GetCodeType() << ">";

        return ss.str();
    }
    else
    {
        return std::string();
    }
}

std::string MetaVariableSet::GetConstructValue() const
{
    return std::string();
}

std::string MetaVariableSet::GetValidCondition(const Generator& generator,
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

std::string MetaVariableSet::GetLoadCode(const Generator& generator,
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
            replacements["@PERSIST_COPY@"] =
                generator.GetPersistentRefCopyCode(mElementType, name);

            code = generator.ParseTemplate(0, "VariableSetLoad",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableSet::GetSaveCode(const Generator& generator,
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

            code = generator.ParseTemplate(0, "VariableSetSave",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableSet::GetLoadRawCode(const Generator& generator,
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
            replacements["@PERSIST_COPY@"] =
                generator.GetPersistentRefCopyCode(mElementType, name);

            code = generator.ParseTemplate(0, "VariableSetLoadRaw",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableSet::GetSaveRawCode(const Generator& generator,
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

            code = generator.ParseTemplate(0, "VariableSetSaveRaw",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableSet::GetXmlLoadCode(const Generator& generator,
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

        code = generator.ParseTemplate(tabLevel, "VariableSetXmlLoad",
            replacements);
    }

    return code;
}

std::string MetaVariableSet::GetXmlSaveCode(const Generator& generator,
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

        code = generator.ParseTemplate(0, "VariableSetXmlSave",
            replacements);
    }

    return code;
}

std::string MetaVariableSet::GetAccessDeclarations(const Generator& generator,
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
            "VariableSetAccessDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableSet::GetAccessFunctions(const Generator& generator,
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
        replacements["@PERSISTENT_CODE@"] = object.IsPersistent() ?
            ("mDirtyFields.insert(\"" + GetName() + "\");") : "";

        ss << std::endl << generator.ParseTemplate(0, "VariableSetAccessFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableSet::GetUtilityDeclarations(const Generator& generator,
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
            "VariableSetUtilityDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableSet::GetUtilityFunctions(const Generator& generator,
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

        auto entryValidation = mElementType->GetValidCondition(generator, "val", false);

        replacements["@ELEMENT_VALIDATION_CODE@"] = entryValidation.length() > 0
            ? entryValidation : "([&]() { (void)val; return true; })()";

        ss << std::endl << generator.ParseTemplate(0, "VariableSetUtilityFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableSet::GetAccessScriptBindings(const Generator& generator,
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
            "VariableSetAccessScriptBindings", replacements) << std::endl;
    }

    return ss.str();
}
