/**
 * @file libobjgen/src/MetaVariableArray.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for a member variable that is an array of variables.
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

#include "MetaVariableArray.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Libraries
#include <sstream>

using namespace libobjgen;

MetaVariableArray::MetaVariableArray(
    const std::shared_ptr<MetaVariable>& elementType) : MetaVariable(),
    mElementCount(0), mElementType(elementType)
{
}

MetaVariableArray::~MetaVariableArray()
{
}

size_t MetaVariableArray::GetSize() const
{
    if(mElementType)
    {
        return mElementType->GetSize() * mElementCount;
    }
    else
    {
        return 0;
    }
}

uint16_t MetaVariableArray::GetDynamicSizeCount() const
{
    return 1;
}

std::shared_ptr<MetaVariable> MetaVariableArray::GetElementType() const
{
    return mElementType;
}

size_t MetaVariableArray::GetElementCount() const
{
    return mElementCount;
}

void MetaVariableArray::SetElementCount(size_t elementCount)
{
    mElementCount = elementCount;
}

MetaVariable::MetaVariableType_t MetaVariableArray::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_ARRAY;
}

std::string MetaVariableArray::GetType() const
{
    return "array";
}

bool MetaVariableArray::IsCoreType() const
{
    return false;
}

bool MetaVariableArray::IsValid() const
{
    return 0 != mElementCount && mElementType && mElementType->IsValid();
}

bool MetaVariableArray::IsValid(const void *pData, size_t dataSize) const
{
    (void)pData;
    (void)dataSize;

    /// @todo Fix
    return true;
}

bool MetaVariableArray::Load(std::istream& stream)
{
    return IsValid() && mElementType->Load(stream);
}

bool MetaVariableArray::Save(std::ostream& stream) const
{
    return IsValid() && mElementType->Save(stream);
}

bool MetaVariableArray::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    bool status = true;

    const char *szSize = root.Attribute("size");

    if(nullptr != szSize)
    {
        std::stringstream ss(szSize);

        size_t elementCount = 0;
        ss >> elementCount;

        if(ss && 0 < elementCount)
        {
            SetElementCount(elementCount);
        }
        else
        {
            status = false;
        }
    }
    else
    {
        SetElementCount(0);

        status = false;
    }

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableArray::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());

    if(GetElementCount() != 0)
    {
        pVariableElement->SetAttribute("size", std::to_string(GetElementCount()).c_str());
    }
    
    if(!mElementType)
    {
        return false;
    }

    mElementType->Save(doc, *pVariableElement, "element");

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

std::string MetaVariableArray::GetCodeType() const
{
    if(mElementType)
    {
        std::stringstream ss;
        ss << "std::array<" << mElementType->GetCodeType() << ", "
            << mElementCount << ">";

        return ss.str();
    }
    else
    {
        return std::string();
    }
}

std::string MetaVariableArray::GetConstructValue() const
{
    if(mElementType)
    {
        std::string value = mElementType->GetConstructValue();

        if(!value.empty() && 0 < mElementCount)
        {
            std::stringstream ss;
            ss << "{{ " << value;

            for(size_t i = 1; i < mElementCount; ++i)
            {
                ss << ", " << value;
            }

            ss << " }}";

            value = ss.str();
        }

        return value;
    }
    else
    {
        return std::string();
    }
}

std::string MetaVariableArray::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
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

std::string MetaVariableArray::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetLoadCode(generator, "value", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_LOAD_CODE@"] = code;
            replacements["@STREAM@"] = stream + std::string(".stream");

            code = generator.ParseTemplate(0, "VariableArrayLoad",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableArray::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetSaveCode(generator, "value", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_SAVE_CODE@"] = code;
            replacements["@STREAM@"] = stream + std::string(".stream");

            code = generator.ParseTemplate(0, "VariableArraySave",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableArray::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetLoadRawCode(generator, "value", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_LOAD_CODE@"] = code;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableArrayLoad",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableArray::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(mElementType && MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        code = mElementType->GetSaveRawCode(generator, "value", stream);

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@VAR_SAVE_CODE@"] = code;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableArraySave",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableArray::GetXmlLoadCode(const Generator& generator,
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
        replacements["@ELEMENT_COUNT@"] = std::to_string(mElementCount);
        replacements["@DEFAULT_VALUE@"] = mElementType->GetDefaultValueCode();

        code = generator.ParseTemplate(tabLevel, "VariableArrayXmlLoad",
            replacements);
    }

    return code;
}

std::string MetaVariableArray::GetXmlSaveCode(const Generator& generator,
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

        code = generator.ParseTemplate(0, "VariableArrayXmlSave",
            replacements);
    }

    return code;
}

std::string MetaVariableArray::GetAccessDeclarations(const Generator& generator,
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
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableArrayAccessDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableArray::GetAccessFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessFunctions(generator, object, name);

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
        replacements["@ELEMENT_COUNT@"] = std::to_string(mElementCount);

        ss << std::endl << generator.ParseTemplate(0, "VariableArrayAccessFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableArray::GetUtilityDeclarations(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

        ss << generator.ParseTemplate(tabLevel,
            "VariableArrayUtilityDeclarations", replacements) << std::endl;
    }

    return ss.str();
}

std::string MetaVariableArray::GetUtilityFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;

    if(mElementType)
    {
        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = name;
        replacements["@VAR_TYPE@"] = mElementType->GetCodeType();
        replacements["@OBJECT_NAME@"] = object.GetName();
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
        replacements["@ELEMENT_COUNT@"] = std::to_string(mElementCount);
        replacements["@ELEMENT_VALIDATION_CODE@"] = mElementType->GetValidCondition(
            generator, "val", true);

        ss << std::endl << generator.ParseTemplate(0, "VariableArrayUtilityFunctions",
            replacements) << std::endl;
    }

    return ss.str();
}
