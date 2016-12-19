/**
 * @file libobjgen/src/MetaVariableReference.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for a member variable that is a reference to another object.
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

#include "MetaVariableReference.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"

// Standard C++11 Libraries
#include <sstream>

using namespace libobjgen;

MetaVariableReference::MetaVariableReference() : MetaVariable()
{
}

MetaVariableReference::~MetaVariableReference()
{
}

size_t MetaVariableReference::GetSize() const
{
    return 16u; // Size of a UUID.
}

MetaVariable::MetaVariableType_t MetaVariableReference::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_REF;
}

std::string MetaVariableReference::GetType() const
{
    return mReferenceType + "*";
}

std::string MetaVariableReference::GetReferenceType() const
{
    return mReferenceType;
}

bool MetaVariableReference::SetReferenceType(const std::string& referenceType)
{
    bool status = false;

    if(MetaObject::IsValidIdentifier(referenceType))
    {
        status = true;
        mReferenceType = referenceType;
    }

    return status;
}

void MetaVariableReference::AddDefaultedVariable(std::shared_ptr<MetaVariable>& var)
{
    mDefaultedVariables.push_back(var);
}

const std::list<std::shared_ptr<MetaVariable>> MetaVariableReference::GetDefaultedVariables() const
{
    return mDefaultedVariables;
}

bool MetaVariableReference::IsCoreType() const
{
    return false;
}

bool MetaVariableReference::IsValid() const
{
    // Validating that the object exists happens elsewhere
    return MetaObject::IsValidIdentifier(mReferenceType) && !IsLookupKey();
}

bool MetaVariableReference::IsValid(const void *pData, size_t dataSize) const
{
    (void)pData;
    (void)dataSize;

    /// @todo Fix
    return true;
}

bool MetaVariableReference::Load(std::istream& stream)
{
    LoadString(stream, mReferenceType);

    return stream.good() && IsValid();
}

bool MetaVariableReference::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid())
    {
        SaveString(stream, mReferenceType);

        result = stream.good();
    }

    return result;
}

bool MetaVariableReference::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    bool status = true;

    //Reference type should already be set

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableReference::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", (GetReferenceType() + "*").c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

uint16_t MetaVariableReference::GetDynamicSizeCount() const
{
    /// @todo Lookup the reference type and call this for that type!
    return 0;
}

std::string MetaVariableReference::GetCodeType() const
{
    std::string code;

    if(!mReferenceType.empty())
    {
        std::stringstream ss;
        ss << "std::shared_ptr<" << mReferenceType << ">";

        code = ss.str();
    }

    return code;
}

std::string MetaVariableReference::GetConstructValue() const
{
    std::stringstream ss;
    ss << GetCodeType() << "(new " << GetReferenceType() << ")";

    return ss.str();
}

std::string MetaVariableReference::GetConstructorCode(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    (void)object;
    (void)name;

    std::stringstream ss;
    ss << generator.Tab(tabLevel) << "{" << std::endl;
    ss << GetConstructorCode(generator, "ref", "", tabLevel + 1);
    ss << generator.Tab(tabLevel) << "}" << std::endl;
    return ss.str();
}

std::string MetaVariableReference::GetConstructorCode(const Generator& generator,
    const std::string& varName, const std::string& parentRef, size_t tabLevel) const
{
    std::string code = GetConstructValue();

    std::stringstream ss;
    ss << generator.Tab(tabLevel) << "auto " << varName << " = " << code << ";" << std::endl;
    if(mDefaultedVariables.size() > 0)
    {
        ss << generator.Tab(tabLevel) << "{" << std::endl;
        for(auto var : mDefaultedVariables)
        {
            std::stringstream ssVar;
            ssVar << var->GetName() << "Value";
            std::string localVarName = ssVar.str();

            if(var->GetMetaType() == MetaVariableType_t::TYPE_REF)
            {
                ss << generator.Tab(tabLevel + 1) << "{" << std::endl;
                ss << GetConstructorCode(generator, parentRef + "_ref",
                    localVarName, tabLevel + 1);
                ss << generator.Tab(tabLevel + 1) << "}" << std::endl;
            }
            else
            {
                ss << generator.Tab(tabLevel + 1) << "auto " << localVarName
                    << " = " << var->GetConstructValue() << ";" << std::endl;
            }

            ss << generator.Tab(tabLevel + 1) << varName << "->Set"
                << generator.GetCapitalName(var) << "(" << localVarName <<
                ");" << std::endl;
        }
        ss << generator.Tab(tabLevel) << "}" << std::endl;
    }
    ss << generator.Tab(tabLevel);
    if(parentRef.length() > 0)
    {
        ss << parentRef << "->";
    }
    ss << "Set" << generator.GetCapitalName(*this) << "(ref);" << std::endl;

    return ss.str();
}

std::string MetaVariableReference::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;

    std::stringstream ss;

    if(recursive)
    {
        ss << name  << " && (!recursive || " << name
            << "->IsValid(recursive))";
    }
    else
    {
        ss << name;
    }

    return ss.str();
}

std::string MetaVariableReference::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    return generator.ParseTemplate(1, "VariableReferenceLoad",
        replacements);
}

std::string MetaVariableReference::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    return generator.ParseTemplate(1, "VariableReferenceSave",
        replacements);
}

std::string MetaVariableReference::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    return generator.ParseTemplate(1, "VariableReferenceLoadRaw",
        replacements);
}

std::string MetaVariableReference::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    return generator.ParseTemplate(1, "VariableReferenceSaveRaw",
        replacements);
}

std::string MetaVariableReference::GetXmlLoadCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& node, size_t tabLevel) const
{
    (void)node;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@VAR_CODE_TYPE@"] = GetCodeType();
    replacements["@DOC@"] = doc;
    replacements["@PARENT@"] = node;

    return generator.ParseTemplate(tabLevel, "VariableReferenceXmlLoad",
        replacements);
}

std::string MetaVariableReference::GetXmlSaveCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& parent, size_t tabLevel, const std::string elemName) const
{
    (void)name;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@VAR_XML_NAME@"] = generator.Escape(GetName());
    replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
    replacements["@DOC@"] = doc;
    replacements["@PARENT@"] = parent;

    return generator.ParseTemplate(tabLevel, "VariableReferenceXmlSave",
        replacements);
}

std::string MetaVariableReference::GetBindValueCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::string columnName = GetName();

    std::transform(columnName.begin(), columnName.end(),
        columnName.begin(), ::tolower);

    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(columnName);
    replacements["@VAR_NAME@"] = name;
    replacements["@TYPE@"] = "UUID";

    return generator.ParseTemplate(tabLevel, "VariableGetTypeBind",
        replacements);
}
