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

const std::vector<std::string> MetaVariableEnum::GetValues() const
{
    return mValues;
}

size_t MetaVariableEnum::GetSize() const
{
    return sizeof(uint32_t);
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

    return mValues.size() > 0 && mDefaultValue <= mValues.size();
}

bool MetaVariableEnum::IsValid(const void *pData, size_t dataSize) const
{
    (void)pData;
    (void)dataSize;

    /// @todo Fix
    return true;
}

bool MetaVariableEnum::Load(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(&mDefaultValue),
        sizeof(mDefaultValue));

    size_t len;
    stream.read(reinterpret_cast<char*>(&len),
        sizeof(len));

    mValues.clear();
    for(size_t i = 0; i < len; i++)
    {
        std::string val;
        LoadString(stream, val);
        mValues.push_back(val);
    }

    return stream.good() && IsValid();
}

bool MetaVariableEnum::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid())
    {
        stream.write(reinterpret_cast<const char*>(&mDefaultValue),
            sizeof(mDefaultValue));

        size_t len = mValues.size();
        stream.write(reinterpret_cast<const char*>(&len),
            sizeof(len));

        for(auto val : mValues)
        {
            SaveString(stream, val);
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

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableEnum::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());
    pVariableElement->SetAttribute("default", mValues[mDefaultValue].c_str());

    for(auto val : mValues)
    {
        tinyxml2::XMLElement *pVariableValue = doc.NewElement(elementName);
        pVariableValue->SetText(val.c_str());
        pVariableElement->InsertEndChild(pVariableValue);
    }

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

std::string MetaVariableEnum::GetCodeType() const
{
    std::stringstream ss;
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
    (void)name;
    (void)recursive;

    std::stringstream ss;

    if(mValues.size() > 0)
    {
        ss << generator.GetMemberName(*this) << " >= " << GetCodeType()
            << "::" << mValues[0] << "&& " << generator.GetMemberName(*this)
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
    replacements["@VAR_NAME@"] = generator.Escape(GetName());
    replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
    replacements["@GETTER@"] = GetInternalGetterCode(generator, name);
    replacements["@PARENT@"] = parent;

    return generator.ParseTemplate(tabLevel, "VariableEnumXmlSave",
        replacements);
}

std::string MetaVariableEnum::GetStringValueCode(const std::string& name) const
{
    std::stringstream ss;
    ss << "libcomp::String((uint32_t)" << name << ")";
    return ss.str();
}

std::string MetaVariableEnum::GetUtilityDeclarations(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    (void)generator;
    (void)name;
    (void)tabLevel;

    std::stringstream ss;
    ss << "std::string GetEnumString(" << GetCodeType() << " val) const;";
    ss << GetCodeType() << " GetEnumValue(std::string& val, bool& success);";
    return ss.str();
}

std::string MetaVariableEnum::GetUtilityFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    (void)generator;
    (void)name;

    std::stringstream ss;
    ss << "std::string " << object.GetName() << "::GetEnumString("
        << GetCodeType() << " val) const" << std::endl;
    ss << "{" << std::endl;
    ss << generator.Tab() << "switch(val)" << std::endl;
    ss << generator.Tab() << "{" << std::endl;
    for(auto val : mValues)
    {
        ss << generator.Tab(2) << "case " << GetCodeType() << "::" << val << ":" << std::endl;
        ss << generator.Tab(3) << "return " << generator.Escape(val) << ";" << std::endl;
        ss << generator.Tab(3) << "break;" << std::endl;
    }
    ss << generator.Tab(2) << "default:" << std::endl;
    ss << generator.Tab(3) << "break;" << std::endl;
    ss << generator.Tab() << "}" << std::endl;
    ss << generator.Tab() << "return \"\";" << std::endl;
    ss << "}" << std::endl << std::endl;

    ss << object.GetName() << "::" << GetCodeType() << " " << object.GetName()
        << "::GetEnumValue(std::string& val, bool& success)" << std::endl;
    ss << "{" << std::endl;
    ss << generator.Tab() << "success = true;" << std::endl;
    for(auto val : mValues)
    {
        ss << generator.Tab() << "if(val == " << generator.Escape(val) << ") return "
            << GetCodeType() << "::" << val << ";" << std::endl;
    }
    ss << generator.Tab() << "success = false;" << std::endl;
    ss << generator.Tab() << "return (" << GetCodeType() << ")0;" << std::endl;
    ss << "}" << std::endl;

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
