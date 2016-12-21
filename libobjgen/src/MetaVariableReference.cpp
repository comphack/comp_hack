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

MetaVariableReference::MetaVariableReference()
    : MetaVariable()
{
    mPersistentParent = false;
    mDynamicSizeCount = 0;
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

bool MetaVariableReference::GetPersistentParent() const
{
    return mPersistentParent;
}

bool MetaVariableReference::SetPersistentParent(bool persistentParent)
{
    mPersistentParent = persistentParent;
    return true;
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

bool MetaVariableReference::Load(std::istream& stream)
{
    LoadString(stream, mReferenceType);
    stream.read(reinterpret_cast<char*>(&mDynamicSizeCount),
        sizeof(mDynamicSizeCount));
    stream.read(reinterpret_cast<char*>(&mPersistentParent),
        sizeof(mPersistentParent));

    return stream.good() && IsValid();
}

bool MetaVariableReference::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid())
    {
        SaveString(stream, mReferenceType);
        stream.write(reinterpret_cast<const char*>(&mDynamicSizeCount),
            sizeof(mDynamicSizeCount));
        stream.write(reinterpret_cast<const char*>(&mPersistentParent),
            sizeof(mPersistentParent));

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
    return mDynamicSizeCount;
}

bool MetaVariableReference::SetDynamicSizeCount(uint16_t dynamicSizeCount)
{
    mDynamicSizeCount = dynamicSizeCount;
    return true;
}

std::string MetaVariableReference::GetCodeType() const
{
    std::string code;

    if(!mReferenceType.empty())
    {
        std::stringstream ss;
        ss << "libcomp::ObjectReference<" << mReferenceType << ">";

        code = ss.str();
    }

    return code;
}

std::string MetaVariableReference::GetConstructValue() const
{
    std::stringstream ss;
    if(mPersistentParent)
    {
        ss << GetCodeType() << "()";
    }
    else
    {
        ss << GetCodeType() << "(std::shared_ptr<" << mReferenceType << ">(new " << mReferenceType << "))";
    }

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
    bool topLevel = parentRef.length() == 0;
    if(mPersistentParent)
    {
        ss << GetName() << " = " << code << ";" << std::endl;
        //Should always be the case
        if(topLevel)
        {
            //The only valid value at this point is a UUID for the persistent reference
            //however we should check regardless
            auto var = mDefaultedVariables.front();
            if(mDefaultedVariables.size() == 1 && var->GetName() == "UID" &&
                var->GetMetaType() == MetaVariableType_t::TYPE_STRING)
            {
                ss << "libobjgen::UUID uuid(" << var->GetDefaultValueCode()
                    << ");" << std::endl;
                ss << GetName() << ".SetUUID(uuid)" << std::endl;
            }
        }
    }
    else
    {
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

                ss << generator.Tab(tabLevel + 1) << varName << ".Get()->Set"
                    << generator.GetCapitalName(var) << "(" << localVarName <<
                    ");" << std::endl;
            }
            ss << generator.Tab(tabLevel) << "}" << std::endl;
        }
        ss << generator.Tab(tabLevel);
        if(!topLevel)
        {
            ss << parentRef << ".Get()->";
        }
        ss << "Set" << generator.GetCapitalName(*this) << "(ref);" << std::endl;
    }
    return ss.str();
}

std::string MetaVariableReference::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;

    if(recursive)
    {
        std::stringstream ss;
        ss << "nullptr != " << name  << ".GetCurrentReference() && (!recursive || "
            << name << ".GetCurrentReference()->IsValid(recursive))";
        return ss.str();
    }
    else
    {
        return "";
    }
}

std::string MetaVariableReference::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    if(mPersistentParent)
    {
        return generator.ParseTemplate(1, "VariablePersistentReferenceLoad",
            replacements);
    }
    else
    {
        return generator.ParseTemplate(1, "VariableReferenceLoad",
            replacements);
    }
}

std::string MetaVariableReference::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    if(mPersistentParent)
    {
        return generator.ParseTemplate(1, "VariablePersistentReferenceSave",
            replacements);
    }
    else
    {
        return generator.ParseTemplate(1, "VariableReferenceSave",
            replacements);
    }
}

std::string MetaVariableReference::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    if(mPersistentParent)
    {
        return generator.ParseTemplate(1, "VariablePersistentReferenceLoadRaw",
            replacements);
    }
    else
    {
        return generator.ParseTemplate(1, "VariableReferenceLoadRaw",
            replacements);
    }
}

std::string MetaVariableReference::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    if(mPersistentParent)
    {
        return generator.ParseTemplate(1, "VariablePersistentReferenceSaveRaw",
            replacements);
    }
    else
    {
        return generator.ParseTemplate(1, "VariableReferenceSaveRaw",
            replacements);
    }
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
    replacements["@NODE@"] = node;

    if(mPersistentParent)
    {
        return generator.ParseTemplate(tabLevel, "VariablePersistentReferenceXmlLoad",
            replacements);
    }
    else
    {
        return generator.ParseTemplate(tabLevel, "VariableReferenceXmlLoad",
            replacements);
    }
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

    if(mPersistentParent)
    {
        return generator.ParseTemplate(tabLevel, "VariablePersistentReferenceXmlSave",
            replacements);
    }
    else
    {
        return generator.ParseTemplate(tabLevel, "VariableReferenceXmlSave",
            replacements);
    }
}

std::string MetaVariableReference::GetBindValueCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;
    ss << name << ".GetUUID()";

    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@VAR_NAME@"] = ss.str();
    replacements["@TYPE@"] = "UUID";

    return generator.ParseTemplate(tabLevel, "VariableGetTypeBind",
        replacements);
}

std::string MetaVariableReference::GetDatabaseLoadCode(
    const Generator& generator, const std::string& name, size_t tabLevel) const
{
    (void)name;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());

    return generator.ParseTemplate(tabLevel, "VariableDatabaseRefLoad",
        replacements);
}
