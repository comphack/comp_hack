/**
 * @file libobjgen/src/MetaVariableEnum.cpp
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief Meta data for an enum based object member variable.
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

#include "MetaVariableEnum.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Libraries
#include <sstream>

using namespace libobjgen;

MetaVariableEnum::MetaVariableEnum() : MetaVariable()
{
}

MetaVariableEnum::~MetaVariableEnum()
{
}

uint32_t MetaVariableEnum::GetDefaultValue() const
{
    return mDefaultValue;
}

void MetaVariableEnum::SetDefaultValue(const uint32_t value)
{
    mDefaultValue = value;
}

std::string MetaVariableEnum::GetTypePrefix() const
{
    return mTypePrefix;
}

void MetaVariableEnum::SetTypePrefix(const std::string& prefix)
{
    mTypePrefix = prefix;
}

short MetaVariableEnum::GetSizeType() const
{
    return mSizeType;
}

std::string MetaVariableEnum::GetSizeTypeString() const
{
    switch(mSizeType)
    {
        case 8:
            return "uint8_t";
        case 16:
            return "uint16_t";
        case 32:
            return "uint32_t";
        default:
            break;
    }

    return "";
}

bool MetaVariableEnum::SetSizeType(const short sizeType)
{
    switch(sizeType)
    {
        case 8:
        case 16:
        case 32:
            break;
        default:
            return false;
    }

    mSizeType = sizeType;
    return true;
}

const std::vector<std::string> MetaVariableEnum::GetValues() const
{
    return mValues;
}

size_t MetaVariableEnum::GetSize() const
{
    switch(mSizeType)
    {
        case 8:
            return sizeof(uint8_t);
        case 16:
            return sizeof(uint16_t);
        case 32:
        default:
            return sizeof(uint32_t);
    }
}

MetaVariable::MetaVariableType_t MetaVariableEnum::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_ENUM;
}

std::string MetaVariableEnum::GetType() const
{
    return "enum";
}

bool MetaVariableEnum::IsCoreType() const
{
    return false;
}

bool MetaVariableEnum::IsScriptAccessible() const
{
    return false;
}

bool MetaVariableEnum::IsValid() const
{
    //Dupe check
    if(mValues.size() > 0)
    {
        for(auto iter = mValues.begin(); iter <= mValues.end()-1; iter++)
        {
            if(std::find(iter + 1, mValues.end(), *iter) != mValues.end())
            {
                return false;
            }
        }
    }

    switch(mSizeType)
    {
        case 8:
        case 16:
        case 32:
            break;
        default:
            return false;
    }

    return mValues.size() > 0 && mDefaultValue <= mValues.size();
}

bool MetaVariableEnum::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    stream.read(reinterpret_cast<char*>(&mDefaultValue),
        sizeof(mDefaultValue));

    stream.read(reinterpret_cast<char*>(&mSizeType),
        sizeof(mSizeType));

    Generator::LoadString(stream, mTypePrefix);

    size_t len;
    stream.read(reinterpret_cast<char*>(&len),
        sizeof(len));

    mValues.clear();
    for(size_t i = 0; i < len; i++)
    {
        std::string val;
        Generator::LoadString(stream, val);
        mValues.push_back(val);
    }

    return stream.good() && IsValid();
}

bool MetaVariableEnum::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid() && MetaVariable::Save(stream))
    {
        stream.write(reinterpret_cast<const char*>(&mDefaultValue),
            sizeof(mDefaultValue));

        stream.write(reinterpret_cast<const char*>(&mSizeType),
            sizeof(mSizeType));

        Generator::SaveString(stream, mTypePrefix);

        size_t len = mValues.size();
        stream.write(reinterpret_cast<const char*>(&len),
            sizeof(len));

        for(auto val : mValues)
        {
            Generator::SaveString(stream, val);
        }

        result = stream.good();
    }

    return result;
}

bool MetaVariableEnum::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    bool status = true;

    mValues.clear();
    const tinyxml2::XMLElement *pVariableChild = root.FirstChildElement();
    while(nullptr != pVariableChild)
    {
        if(std::string("value") == pVariableChild->Name())
        {
            std::string val(pVariableChild->GetText());
            if(val.length() > 0 && !ValueExists(val))
            {
                mValues.push_back(val);
            }
            else
            {
                return false;
            }
        }

        pVariableChild = pVariableChild->NextSiblingElement();
    }

    auto defaultVal = root.Attribute("default");

    if(defaultVal && ValueExists(defaultVal))
    {
        std::string val(defaultVal);
        for(size_t i = 0; i < mValues.size(); i++)
        {
            if(mValues[i] == val)
            {
                mDefaultValue = (uint32_t)i;
                break;
            }
        }
    }
    
    const char *szSize = root.Attribute("size");

    if(nullptr != szSize)
    {
        std::string size(szSize);

        if("8" == size)
        {
            SetSizeType(8);
        }
        else if("16" == size)
        {
            SetSizeType(16);
        }
        else if("32" == size)
        {
            SetSizeType(32);
        }
        else
        {
            mError = "The only valid size values are 8, 16 and 32.";

            status = false;
        }
    }
    else
    {
        SetSizeType(32);
    }

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableEnum::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());
    pVariableElement->SetAttribute("default", mValues[mDefaultValue].c_str());

    if(8 != mSizeType && (16 == mSizeType || 32 == mSizeType))
    {
        pVariableElement->SetAttribute("size", std::to_string(mSizeType).c_str());
    }

    for(auto val : mValues)
    {
        tinyxml2::XMLElement *pVariableValue = doc.NewElement(elementName);
        pVariableValue->SetText(val.c_str());
        pVariableElement->InsertEndChild(pVariableValue);
    }

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

std::string MetaVariableEnum::GetArgumentType() const
{
    std::stringstream ss;

    ss << "const " << GetCodeType();

    return ss.str();
}

std::string MetaVariableEnum::GetCodeType() const
{
    std::stringstream ss;
    if(mTypePrefix.length() > 0)
    {
        ss << mTypePrefix << "::";
    }
    ss << GetName() << "_t";
    return ss.str();
}

std::string MetaVariableEnum::GetConstructValue() const
{
    std::stringstream ss;
    ss << GetCodeType() << "::" << GetDefaultValueCode();
    return ss.str();
}

std::string MetaVariableEnum::GetDefaultValueCode() const
{
    return mValues[mDefaultValue];
}

std::string MetaVariableEnum::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;
    (void)recursive;

    std::stringstream ss;

    if(mValues.size() > 0)
    {
        ss << name << " >= " << GetCodeType()
            << "::" << mValues[0] << " && " << name
            << " <= " << GetCodeType() << "::" << mValues[mValues.size() - 1];
    }

    return ss.str();
}

std::string MetaVariableEnum::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    return GetLoadRawCode(generator, name, stream + std::string(".stream"));
}

std::string MetaVariableEnum::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    return GetSaveRawCode(generator, name, stream + std::string(".stream"));
}

std::string MetaVariableEnum::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    code = generator.ParseTemplate(0, "VariableEnumLoad",
        replacements);

    return code;
}

std::string MetaVariableEnum::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    code = generator.ParseTemplate(0, "VariableEnumSave",
        replacements);

    return code;
}

std::string MetaVariableEnum::GetXmlLoadCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& node, size_t tabLevel) const
{
    (void)name;
    (void)doc;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = GetName();
    replacements["@VAR_CODE_TYPE@"] = GetCodeType();
    replacements["@DEFAULT@"] = GetDefaultValueCode();
    replacements["@NODE@"] = node;

    return generator.ParseTemplate(tabLevel, "VariableEnumXmlLoad",
        replacements);
}

std::string MetaVariableEnum::GetXmlSaveCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& parent, size_t tabLevel, const std::string elemName) const
{
    (void)doc;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = GetName();
    replacements["@VAR_XML_NAME@"] = generator.Escape(GetName());
    replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
    replacements["@GETTER@"] = GetInternalGetterCode(generator, name);
    replacements["@PARENT@"] = parent;

    return generator.ParseTemplate(tabLevel, "VariableEnumXmlSave",
        replacements);
}

std::string MetaVariableEnum::GetBindValueCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@VAR_NAME@"] = name;
    replacements["@TYPE@"] = "Int";

    return generator.ParseTemplate(tabLevel, "VariableGetTypeBind",
        replacements);
}

std::string MetaVariableEnum::GetDatabaseLoadCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    (void)name;

    std::map<std::string, std::string> replacements;
    replacements["@DATABASE_TYPE@"] = GetSizeTypeString();
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@SET_FUNCTION@"] = std::string("Set") +
        generator.GetCapitalName(*this);
    replacements["@VAR_TYPE@"] = GetCodeType();

    return generator.ParseTemplate(tabLevel, "VariableDatabaseCastLoad",
        replacements);
}

std::string MetaVariableEnum::GetUtilityDeclarations(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    (void)name;

    std::stringstream ss;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_TYPE@"] = GetCodeType();
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

    ss << generator.ParseTemplate(tabLevel,
        "VariableEnumUtilityDeclarations", replacements) << std::endl;

    return ss.str();
}

std::string MetaVariableEnum::GetUtilityFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    (void)name;

    std::stringstream ss;

    std::stringstream cases;
    for(auto val : mValues)
    {
        cases << "case " << GetCodeType() << "::" << val << ": return "
            << generator.Escape(val) << "; break;" << std::endl;
    }

    std::stringstream conditions;
    for(auto val : mValues)
    {
        conditions << "if(val == " << generator.Escape(val) << ") return "
            << GetCodeType() << "::" << val << ";" << std::endl;
    }

    std::map<std::string, std::string> replacements;
    replacements["@VAR_TYPE@"] = GetCodeType();
    replacements["@OBJECT_NAME@"] = object.GetName();
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
    replacements["@CASES@"] = cases.str();
    replacements["@CONDITIONS@"] = conditions.str();

    ss << std::endl << generator.ParseTemplate(0, "VariableEnumUtilityFunctions",
        replacements) << std::endl;

    return ss.str();
}

bool MetaVariableEnum::ValueExists(const std::string& val)
{
    if(mValues.size() > 0)
    {
        return std::find(mValues.begin(), mValues.end(), val) != mValues.end();
    }

    return false;
}
