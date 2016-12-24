/**
 * @file libobjgen/src/MetaVariableBool.cpp
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief Meta data for a boolean object member variable.
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

#include "MetaVariableBool.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Libraries
#include <sstream>

using namespace libobjgen;

MetaVariableBool::MetaVariableBool() : MetaVariable()
{
}

MetaVariableBool::~MetaVariableBool()
{
}

bool MetaVariableBool::GetDefaultValue() const
{
    return mDefaultValue;
}

void MetaVariableBool::SetDefaultValue(const bool value)
{
    mDefaultValue = value;
}

size_t MetaVariableBool::GetSize() const
{
    return sizeof(bool);
}

MetaVariable::MetaVariableType_t MetaVariableBool::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_BOOL;
}

std::string MetaVariableBool::GetType() const
{
    return "bool";
}

bool MetaVariableBool::IsCoreType() const
{
    return true;
}

bool MetaVariableBool::IsValid() const
{
    return true;
}

bool MetaVariableBool::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    stream.read(reinterpret_cast<char*>(&mDefaultValue),
        sizeof(mDefaultValue));

    return stream.good() && IsValid();
}

bool MetaVariableBool::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid() && MetaVariable::Save(stream))
    {
        stream.write(reinterpret_cast<const char*>(&mDefaultValue),
            sizeof(mDefaultValue));

        result = stream.good();
    }

    return result;
}

bool MetaVariableBool::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    bool status = true;

    auto defaultVal = root.Attribute("default");

    if(defaultVal)
    {
        mDefaultValue = Generator::GetXmlAttributeBoolean(defaultVal);
    }

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableBool::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());
    pVariableElement->SetAttribute("default", GetDefaultValue());

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

std::string MetaVariableBool::GetCodeType() const
{
    return GetType();
}

std::string MetaVariableBool::GetConstructValue() const
{
    return GetDefaultValueCode();
}

std::string MetaVariableBool::GetDefaultValueCode() const
{
    return mDefaultValue ? "true" : "false";
}

std::string MetaVariableBool::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;
    (void)name;
    (void)recursive;

    //Nothing to validate

    return "";
}

std::string MetaVariableBool::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    return GetLoadRawCode(generator, name, stream + std::string(".stream"));
}

std::string MetaVariableBool::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    return GetSaveRawCode(generator, name, stream + std::string(".stream"));
}

std::string MetaVariableBool::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    code = generator.ParseTemplate(0, "VariableBoolLoad",
        replacements);

    return code;
}

std::string MetaVariableBool::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    code = generator.ParseTemplate(0, "VariableBoolSave",
        replacements);

    return code;
}

std::string MetaVariableBool::GetXmlLoadCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& node, size_t tabLevel) const
{
    (void)name;
    (void)doc;

    std::map<std::string, std::string> replacements;
    replacements["@NODE@"] = node;

    return generator.ParseTemplate(tabLevel, "VariableBoolXmlLoad",
        replacements);
}

std::string MetaVariableBool::GetXmlSaveCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& parent, size_t tabLevel, const std::string elemName) const
{
    (void)doc;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = generator.Escape(GetName());
    replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
    replacements["@GETTER@"] = GetInternalGetterCode(generator, name);
    replacements["@PARENT@"] = parent;

    return generator.ParseTemplate(tabLevel, "VariableBoolXmlSave",
        replacements);
}

std::string MetaVariableBool::GetBindValueCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@VAR_NAME@"] = name;
    replacements["@TYPE@"] = "Bool";

    return generator.ParseTemplate(tabLevel, "VariableGetTypeBind",
        replacements);
}

std::string MetaVariableBool::GetDatabaseLoadCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    (void)name;

    std::map<std::string, std::string> replacements;
    replacements["@DATABASE_TYPE@"] = "bool";
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@SET_FUNCTION@"] = std::string("Set") +
        generator.GetCapitalName(*this);
    replacements["@VAR_TYPE@"] = GetCodeType();

    return generator.ParseTemplate(tabLevel, "VariableDatabaseCastLoad",
        replacements);
}
