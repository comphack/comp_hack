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
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include "MetaVariableInt.h"

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

const std::vector<std::pair<std::string, std::string>>
MetaVariableEnum::GetValues() const
{
    return mValues;
}

bool MetaVariableEnum::SetValues(const std::vector<std::pair<std::string,
    std::string>>& values)
{
    if(ContainsDuplicateValues(values))
    {
        return false;
    }

    for(auto val : mValues)
    {
        if(!NumericValueIsValid(val.second))
        {
            return false;
        }
    }

    mValues = values;
    
    //The first value is always required
    if(mValues.size() > 0 && mValues[0].second.empty())
    {
        if(ValueExists("0", false))
        {
            return false;
        }

        mValues[0].second = "0";
    }

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
    // Limited use but available
    return true;
}

bool MetaVariableEnum::IsValid() const
{
    if(ContainsDuplicateValues(mValues))
    {
        return false;
    }

    for(auto val : mValues)
    {
        if(!NumericValueIsValid(val.second))
        {
            return false;
        }
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
        return mValues.size() > 0 && ValueExists(mDefaultValue, true);
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
        std::pair<std::string, std::string> val;
        Generator::LoadString(stream, val.first);
        Generator::LoadString(stream, val.second);
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
            Generator::SaveString(stream, val.first);
            Generator::SaveString(stream, val.second);
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
    
    const char *szUnderlying = root.Attribute("underlying");

    if(nullptr != szUnderlying)
    {
        SetUnderlyingType(szUnderlying);
    }

    mValues.clear();
    const tinyxml2::XMLElement *pVariableChild = root.FirstChildElement();
    while(nullptr != pVariableChild)
    {
        if(std::string("value") == pVariableChild->Name())
        {
            auto numAttr = pVariableChild->Attribute("num");

            std::string val(pVariableChild->GetText());
            std::string num(numAttr != nullptr ? numAttr : "");
            if(val.length() > 0 && !ValueExists(val, true)
                && !ValueExists(num, false))
            {
                if(mValues.size() == 0 && num.empty())
                {
                    num = "0";
                }

                std::pair<std::string, std::string> pair(val, num);
                mValues.push_back(pair);
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
    else if(mValues.size() > 0)
    {
        mDefaultValue = mValues.front().first;
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
        pVariableValue->SetText(val.first.c_str());
        if(!val.second.empty())
        {
            pVariableValue->SetAttribute("num", val.second.c_str());
        }
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
    ss << Generator::GetCapitalName(*this) << "_t";
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
            << "::" << mValues[0].first << " && " << name
            << " <= " << GetCodeType() << "::" << mValues[mValues.size() - 1].first;
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
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
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
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
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
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
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
    replacements["@VAR_NAME@"] = name;
    replacements["@VAR_TYPE@"] = GetCodeType();

    return generator.ParseTemplate(tabLevel, "VariableDatabaseCastLoad",
        replacements);
}

std::string MetaVariableEnum::GetAccessDeclarations(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessDeclarations(generator,
        object, name, tabLevel);

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@VAR_TYPE@"] = GetCodeType();
    replacements["@UNDERLYING_TYPE@"] = mUnderlyingType;
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

    ss << generator.ParseTemplate(tabLevel,
        "VariableEnumAccessDeclarations", replacements) << std::endl;

    return ss.str();
}

std::string MetaVariableEnum::GetAccessFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessFunctions(generator, object, name);

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@VAR_TYPE@"] = GetCodeType();
    replacements["@UNDERLYING_TYPE@"] = mUnderlyingType;
    replacements["@OBJECT_NAME@"] = object.GetName();
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

    ss << std::endl << generator.ParseTemplate(0, "VariableEnumAccessFunctions",
        replacements) << std::endl;

    return ss.str();
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
        cases << "case " << GetCodeType() << "::" << val.first << ": return "
            << generator.Escape(val.first) << "; break;" << std::endl;
    }

    std::stringstream conditions;
    for(auto val : mValues)
    {
        conditions << "if(val == " << generator.Escape(val.first) << ") return "
            << GetCodeType() << "::" << val.first << ";" << std::endl;
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

std::string MetaVariableEnum::GetAccessScriptBindings(const Generator& generator,
    const MetaObject& object, const std::string& name,
    size_t tabLevel) const
{
    (void)name;

    std::stringstream ss;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_TYPE@"] = GetCodeType();
    replacements["@UNDERLYING_TYPE@"] = mUnderlyingType;
    replacements["@OBJECT_NAME@"] = object.GetName();
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);

    ss << generator.ParseTemplate(tabLevel,
        "VariableEnumAccessScriptBindings", replacements) << std::endl;

    return ss.str();
}

bool MetaVariableEnum::NumericValueIsValid(const std::string& num) const
{
    if(num.empty())
    {
        return true;
    }

    bool ok = false;
    if(mUnderlyingType == "int8_t")
    {
        MetaVariableInt<int8_t>::StringToValue(num, &ok);
    }
    else if(mUnderlyingType == "uint8_t")
    {
        MetaVariableInt<uint8_t>::StringToValue(num, &ok);
    }
    else if(mUnderlyingType == "int16_t")
    {
        MetaVariableInt<int16_t>::StringToValue(num, &ok);
    }
    else if(mUnderlyingType == "uint16_t")
    {
        MetaVariableInt<uint16_t>::StringToValue(num, &ok);
    }
    else if(mUnderlyingType == "int32_t" || mUnderlyingType.empty())
    {
        MetaVariableInt<int32_t>::StringToValue(num, &ok);
    }
    else if(mUnderlyingType == "uint32_t")
    {
        MetaVariableInt<uint32_t>::StringToValue(num, &ok);
    }
    else if(mUnderlyingType == "int64_t")
    {
        MetaVariableInt<int64_t>::StringToValue(num, &ok);
    }
    else if(mUnderlyingType == "uint64_t")
    {
        MetaVariableInt<uint64_t>::StringToValue(num, &ok);
    }
    
    return ok;
}

bool MetaVariableEnum::ValueExists(const std::string& val, bool first) const
{
    for(auto pair : mValues)
    {
        if(first && pair.first == val)
        {
            return true;
        }
        else if(!first && pair.second == val && val != "")
        {
            return true;
        }
    }

    return false;
}

bool MetaVariableEnum::ContainsDuplicateValues(const std::vector<
    std::pair<std::string, std::string>>& values) const
{
    for(size_t i = 0; i < values.size(); i++)
    {
        for(size_t k = i + 1; k < values.size(); k++)
        {
            if(values[i].first == values[k].first ||
                (values[i].second == values[k].second &&
                !values[i].second.empty()))
            {
                return true;
            }
        }
    }

    return false;
}
