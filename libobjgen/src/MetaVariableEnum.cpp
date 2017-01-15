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
#include <regex>
#include <sstream>

using namespace libobjgen;

MetaVariableEnum::MetaVariableEnum() : MetaVariable(),
    mUnderlyingType("int32_t")
{
}

MetaVariableEnum::~MetaVariableEnum()
{
}

std::string MetaVariableEnum::GetDefaultValue() const
{
    return mDefaultValue;
}

void MetaVariableEnum::SetDefaultValue(const std::string& value)
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

std::string MetaVariableEnum::GetUnderlyingType() const
{
    return mUnderlyingType;
}

void MetaVariableEnum::SetUnderlyingType(const std::string& underlyingType)
{
    mUnderlyingType = underlyingType;
}

const std::vector<std::string> MetaVariableEnum::GetValues() const
{
    return mValues;
}

bool MetaVariableEnum::SetValues(const std::vector<std::string>& values)
{
    if(ContainsDupilicateValues(values))
    {
        return false;
    }

    mValues = values;
    return true;
}

size_t MetaVariableEnum::GetSize() const
{
    size_t sz = 4;

    if(std::string::npos != mUnderlyingType.find("8"))
    {
        sz = 1;
    }
    else if(std::string::npos != mUnderlyingType.find("16"))
    {
        sz = 2;
    }
    else if(std::string::npos != mUnderlyingType.find("64"))
    {
        sz = 8;
    }

    return sz;
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
    if(ContainsDupilicateValues(mValues))
    {
        return false;
    }

    std::smatch match;

    if(!std::regex_match(mUnderlyingType, match, std::regex(
        "^u{0,1}int(8|16|32|64)_t$")))
    {
        return false;
    }

    if(IsInherited())
    {
        return !mDefaultValue.empty() && mValues.size() == 0;
    }
    else
    {
        return mValues.size() > 0 && ValueExists(mDefaultValue);
    }
}

bool MetaVariableEnum::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    Generator::LoadString(stream, mDefaultValue);
    Generator::LoadString(stream, mUnderlyingType);
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
        Generator::SaveString(stream, mDefaultValue);
        Generator::SaveString(stream, mUnderlyingType);
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

    if(defaultVal)
    {
        mDefaultValue = std::string(defaultVal);
    }
    
    const char *szUnderlying = root.Attribute("underlying");

    if(nullptr != szUnderlying)
    {
        SetUnderlyingType(szUnderlying);
    }

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableEnum::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());
    pVariableElement->SetAttribute("default", mDefaultValue.c_str());
    pVariableElement->SetAttribute("underlying", mUnderlyingType.c_str());

    for(auto val : mValues)
    {
        tinyxml2::XMLElement *pVariableValue = doc.NewElement("value");
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
    return mDefaultValue;
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
    std::stringstream ss;
    ss << "static_cast<int32_t>(" << name << ")";

    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@VAR_NAME@"] = ss.str();
    replacements["@TYPE@"] = "Int";

    return generator.ParseTemplate(tabLevel, "VariableGetTypeBind",
        replacements);
}

std::string MetaVariableEnum::GetDatabaseLoadCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    (void)name;

    std::map<std::string, std::string> replacements;
    replacements["@DATABASE_TYPE@"] = "int32_t";
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

bool MetaVariableEnum::ValueExists(const std::string& val) const
{
    if(mValues.size() > 0)
    {
        return std::find(mValues.begin(), mValues.end(), val) != mValues.end();
    }

    return false;
}

bool MetaVariableEnum::ContainsDupilicateValues(const std::vector<std::string>& values) const
{
    if(values.size() > 0)
    {
        for(auto iter = values.begin(); iter <= values.end()-1; iter++)
        {
            if(std::find(iter + 1, values.end(), *iter) != values.end())
            {
                return true;
            }
        }
    }

    return false;
}
