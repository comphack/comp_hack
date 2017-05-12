/**
 * @file libobjgen/src/MetaVariableMap.h
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief Meta data for a member variable that is a map of variables.
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

#include "MetaVariableMap.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Libraries
#include <sstream>

using namespace libobjgen;

MetaVariableMap::MetaVariableMap(const std::shared_ptr<MetaVariable>& keyElementType,
    const std::shared_ptr<MetaVariable>& valueElementType) : MetaVariable(),
    mKeyElementType(keyElementType), mValueElementType(valueElementType)
{
}

MetaVariableMap::~MetaVariableMap()
{
}

size_t MetaVariableMap::GetSize() const
{
    return 0;
}

std::shared_ptr<MetaVariable> MetaVariableMap::GetKeyElementType() const
{
    return mKeyElementType;
}

std::shared_ptr<MetaVariable> MetaVariableMap::GetValueElementType() const
{
    return mValueElementType;
}

MetaVariable::MetaVariableType_t MetaVariableMap::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_MAP;
}

std::string MetaVariableMap::GetType() const
{
    return "map";
}

bool MetaVariableMap::IsCoreType() const
{
    return false;
}

bool MetaVariableMap::IsScriptAccessible() const
{
    return mKeyElementType && mKeyElementType->IsScriptAccessible()
        && mValueElementType && mValueElementType->IsScriptAccessible();
}

bool MetaVariableMap::IsValid() const
{
    return mKeyElementType && mKeyElementType->IsValid() &&
        mValueElementType && mValueElementType->IsValid() &&
        !IsLookupKey();
}

bool MetaVariableMap::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    return mKeyElementType->Load(stream) && mValueElementType->Load(stream) && IsValid();
}

bool MetaVariableMap::Save(std::ostream& stream) const
{
    MetaVariable::Save(stream);

    return IsValid() && mKeyElementType->Save(stream) && mValueElementType->Save(stream);
}

bool MetaVariableMap::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    return BaseLoad(root) && IsValid();
}

bool MetaVariableMap::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());

    if(!mKeyElementType || !mValueElementType)
    {
        return false;
    }

    mKeyElementType->Save(doc, *pVariableElement, "key");
    mValueElementType->Save(doc, *pVariableElement, "value");

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

uint16_t MetaVariableMap::GetDynamicSizeCount() const
{
    return 1;
}

std::string MetaVariableMap::GetCodeType() const
{
    if(mKeyElementType && mValueElementType)
    {
        std::stringstream ss;
        ss << "std::unordered_map<" << mKeyElementType->GetCodeType() << ", "
            << mValueElementType->GetCodeType() << ">";

        return ss.str();
    }
    else
    {
        return std::string();
    }
}

std::string MetaVariableMap::GetConstructValue() const
{
    return std::string();
}

std::string MetaVariableMap::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;

    std::string code;

    if(mKeyElementType && mValueElementType)
    {
        std::string keyCode = mKeyElementType->GetValidCondition(generator, "value", recursive);
        std::string valueCode = mValueElementType->GetValidCondition(generator, "value", recursive);

        if(!keyCode.empty() && !valueCode.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_KEY_VALID_CODE@"] = keyCode;
            replacements["@VAR_VALUE_VALID_CODE@"] = valueCode;

            code = generator.ParseTemplate(0, "VariableMapValidCondition",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableMap::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mKeyElementType && mValueElementType
        && MetaObject::IsValidIdentifier(name) && MetaObject::IsValidIdentifier(stream))
    {
        std::string keyCode = mKeyElementType->GetLoadCode(generator, "element", stream);
        std::string valueCode = mValueElementType->GetLoadCode(generator, "element", stream);

        if(!keyCode.empty() && !valueCode.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_KEY_TYPE@"] = mKeyElementType->GetCodeType();
            replacements["@VAR_KEY_LOAD_CODE@"] = keyCode;
            replacements["@VAR_VALUE_TYPE@"] = mValueElementType->GetCodeType();
            replacements["@VAR_VALUE_LOAD_CODE@"] = valueCode;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableMapLoad",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableMap::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mKeyElementType && mValueElementType
        && MetaObject::IsValidIdentifier(name) && MetaObject::IsValidIdentifier(stream))
    {
        std::string keyCode = mKeyElementType->GetSaveCode(generator, "element", stream);
        std::string valueCode = mValueElementType->GetSaveCode(generator, "element", stream);

        if(!keyCode.empty() && !valueCode.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_KEY_SAVE_CODE@"] = keyCode;
            replacements["@VAR_VALUE_SAVE_CODE@"] = valueCode;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableMapSave",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableMap::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mKeyElementType && mValueElementType
        && MetaObject::IsValidIdentifier(name) &&  MetaObject::IsValidIdentifier(stream))
    {
        std::string keyCode = mKeyElementType->GetLoadRawCode(generator, "element", stream);
        std::string valueCode = mValueElementType->GetLoadRawCode(generator, "element", stream);

        if(!keyCode.empty() && !valueCode.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_KEY_TYPE@"] = mKeyElementType->GetCodeType();
            replacements["@VAR_KEY_LOAD_CODE@"] = keyCode;
            replacements["@VAR_VALUE_TYPE@"] = mValueElementType->GetCodeType();
            replacements["@VAR_VALUE_LOAD_CODE@"] = valueCode;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableMapLoadRaw",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableMap::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mKeyElementType && mValueElementType
        && MetaObject::IsValidIdentifier(name) && MetaObject::IsValidIdentifier(stream))
    {
        std::string keyCode = mKeyElementType->GetSaveRawCode(generator, "element", stream);
        std::string valueCode = mValueElementType->GetSaveRawCode(generator, "element", stream);

        if(!keyCode.empty() && !valueCode.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_KEY_SAVE_CODE@"] = keyCode;
            replacements["@VAR_VALUE_SAVE_CODE@"] = valueCode;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableMapSaveRaw",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableMap::GetXmlLoadCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& node, size_t tabLevel) const
{
    (void)name;

    std::string code;

    if(mKeyElementType && mValueElementType)
    {
        std::string keyCode = mKeyElementType->GetXmlLoadCode(generator,
            generator.GetMemberName(mKeyElementType), doc, "keyNode", tabLevel + 1);
        std::string valueCode = mValueElementType->GetXmlLoadCode(generator,
            generator.GetMemberName(mValueElementType), doc, "valueNode", tabLevel + 1);

        std::map<std::string, std::string> replacements;
        replacements["@VAR_CODE_TYPE@"] = GetCodeType();
        replacements["@NODE@"] = node;
        replacements["@KEY_NODE@"] = "keyNode";
        replacements["@VALUE_NODE@"] = "valueNode";
        replacements["@KEY_ACCESS_CODE@"] = keyCode;
        replacements["@VALUE_ACCESS_CODE@"] = valueCode;

        code = generator.ParseTemplate(tabLevel, "VariableMapXmlLoad",
            replacements);
    }

    return code;
}

std::string MetaVariableMap::GetXmlSaveCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& parent, size_t tabLevel, const std::string elemName) const
{
    std::string code;

    if(mKeyElementType && mValueElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@GETTER@"] = GetInternalGetterCode(generator, name);
        replacements["@VAR_NAME@"] = generator.Escape(GetName());
        replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
        replacements["@VAR_XML_KEY_SAVE_CODE@"] = mKeyElementType->GetXmlSaveCode(
            generator, "element", doc, parent, tabLevel + 1, "key");
        replacements["@VAR_XML_VALUE_SAVE_CODE@"] = mValueElementType->GetXmlSaveCode(
            generator, "element", doc, parent, tabLevel + 1, "value");
        replacements["@PARENT@"] = parent;

        code = generator.ParseTemplate(0, "VariableMapXmlSave",
            replacements);
    }

    return code;
}

std::string MetaVariableMap::GetAccessDeclarations(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessDeclarations(generator,
        object, name, tabLevel);

    if(mKeyElementType && mValueElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_KEY_TYPE@"] = mKeyElementType->GetCodeType();
        replacements["@VAR_KEY_ARG_TYPE@"] = mKeyElementType->GetArgumentType();
        replacements["@VAR_VALUE_TYPE@"] = mValueElementType->GetCodeType();
        replacements["@VAR_VALUE_ARG_TYPE@"] = mValueElementType->GetArgumentType();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableMapAccessDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableMap::GetAccessFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessFunctions(generator, object, name);

    if(mKeyElementType && mValueElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_KEY_TYPE@"] = mKeyElementType->GetCodeType();
        replacements["@VAR_KEY_ARG_TYPE@"] = mKeyElementType->GetArgumentType();
        replacements["@VAR_VALUE_TYPE@"] = mValueElementType->GetCodeType();
        replacements["@VAR_VALUE_ARG_TYPE@"] = mValueElementType->GetArgumentType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
        replacements["@PERSISTENT_CODE@"] = object.IsPersistent() ?
            ("mDirtyFields.insert(\"" + GetName() + "\");") : "";

        ss << std::endl << generator.ParseTemplate(0, "VariableMapAccessFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableMap::GetUtilityDeclarations(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;

    if(mKeyElementType && mValueElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_KEY_TYPE@"] = mKeyElementType->GetCodeType();
        replacements["@VAR_KEY_ARG_TYPE@"] = mKeyElementType->GetArgumentType();
        replacements["@VAR_VALUE_TYPE@"] = mValueElementType->GetCodeType();
        replacements["@VAR_VALUE_ARG_TYPE@"] = mValueElementType->GetArgumentType();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableMapUtilityDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableMap::GetUtilityFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;

    if(mKeyElementType && mValueElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_KEY_TYPE@"] = mKeyElementType->GetCodeType();
        replacements["@VAR_KEY_ARG_TYPE@"] = mKeyElementType->GetArgumentType();
        replacements["@VAR_VALUE_TYPE@"] = mValueElementType->GetCodeType();
        replacements["@VAR_VALUE_ARG_TYPE@"] = mValueElementType->GetArgumentType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        auto keyValidation = mKeyElementType->GetValidCondition(generator, "key", false);
        auto valueValidation = mValueElementType->GetValidCondition(generator, "val", false);

        replacements["@KEY_VALIDATION_CODE@"] = keyValidation.length() > 0
            ? keyValidation : "([&]() { (void)key; return true; })()";
        replacements["@VALUE_VALIDATION_CODE@"] = valueValidation.length() > 0
            ? valueValidation : "([&]() { (void)val; return true; })()";

        ss << std::endl << generator.ParseTemplate(0, "VariableMapUtilityFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableMap::GetAccessScriptBindings(const Generator& generator,
    const MetaObject& object, const std::string& name,
    size_t tabLevel) const
{
    std::stringstream ss;
    
    if(mKeyElementType && mValueElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_KEY_TYPE@"] = mKeyElementType->GetCodeType();
        replacements["@VAR_KEY_ARG_TYPE@"] = mKeyElementType->GetArgumentType();
        replacements["@VAR_VALUE_TYPE@"] = mValueElementType->GetCodeType();
        replacements["@VAR_VALUE_ARG_TYPE@"] = mValueElementType->GetArgumentType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableMapAccessScriptBindings", replacements) << std::endl;
    }

    return ss.str();
}
