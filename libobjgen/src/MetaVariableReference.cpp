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
    mNamespace = "objects";
    mPersistentReference = false;
    mNullDefault = false;
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

std::string MetaVariableReference::GetReferenceType(bool includeNamespace) const
{
    return (includeNamespace ? (mNamespace + "::") : "") + mReferenceType;
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

std::string MetaVariableReference::GetNamespace() const
{
    return mNamespace;
}

bool MetaVariableReference::SetNamespace(const std::string& ns)
{
    if(!ns.empty() && MetaObject::IsValidIdentifier(ns))
    {
        mNamespace = ns;
        return true;
    }

    return false;
}

bool MetaVariableReference::IsPersistentReference() const
{
    return mPersistentReference;
}

bool MetaVariableReference::SetPersistentReference(bool persistentReference)
{
    mPersistentReference = persistentReference;
    return true;
}

bool MetaVariableReference::IsGeneric() const
{
    return mNamespace == "libcomp" &&
        (mReferenceType == "Object" || mReferenceType == "PersistentObject");
}

void MetaVariableReference::SetGeneric()
{
    mNamespace = "libcomp";
    if(mPersistentReference)
    {
        mReferenceType = "PersistentObject";
    }
    else
    {
        mReferenceType = "Object";
    }
}

bool MetaVariableReference::GetNullDefault() const
{
    return mNullDefault;
}

bool MetaVariableReference::SetNullDefault(bool nullDefault)
{
    mNullDefault = nullDefault;
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

bool MetaVariableReference::IsScriptAccessible() const
{
    return true;
}

bool MetaVariableReference::IsValid() const
{
    // Validating that the object exists happens elsewhere
    return MetaObject::IsValidIdentifier(mReferenceType) &&
        (mNamespace.empty() || MetaObject::IsValidIdentifier(mNamespace)) &&
        (!mNullDefault || !IsPersistentReference());
}

bool MetaVariableReference::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    Generator::LoadString(stream, mReferenceType);
    Generator::LoadString(stream, mNamespace);
    stream.read(reinterpret_cast<char*>(&mDynamicSizeCount),
        sizeof(mDynamicSizeCount));
    stream.read(reinterpret_cast<char*>(&mPersistentReference),
        sizeof(mPersistentReference));
    stream.read(reinterpret_cast<char*>(&mNullDefault),
        sizeof(mNullDefault));

    if(stream.good())
    {
        mDefaultedVariables.clear();
        MetaVariable::LoadVariableList(stream, mDefaultedVariables);
    }

    return stream.good() && IsValid();
}

bool MetaVariableReference::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid() && MetaVariable::Save(stream))
    {
        Generator::SaveString(stream, mReferenceType);
        Generator::SaveString(stream, mNamespace);
        stream.write(reinterpret_cast<const char*>(&mDynamicSizeCount),
            sizeof(mDynamicSizeCount));
        stream.write(reinterpret_cast<const char*>(&mPersistentReference),
            sizeof(mPersistentReference));
        stream.write(reinterpret_cast<const char*>(&mNullDefault),
            sizeof(mNullDefault));

        MetaVariable::SaveVariableList(stream, mDefaultedVariables);

        result = stream.good();
    }

    return result;
}

bool MetaVariableReference::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    bool status = true;

    // Reference type and namespace should be set and verified
    // elsewhere before this point

    auto nullVal = root.Attribute("nulldefault");

    if(nullVal)
    {
        mNullDefault = Generator::GetXmlAttributeBoolean(nullVal);
    }

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableReference::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", (GetReferenceType() + "*").c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());

    if(!mNamespace.empty() && mNamespace != "objects")
    {
        pVariableElement->SetAttribute("namespace", mNamespace.c_str());
    }

    pVariableElement->SetAttribute("nulldefault", GetNullDefault());

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
    if(mPersistentReference)
    {
        return "libcomp::ObjectReference<" + GetReferenceType(true) + ">";
    }
    else
    {
        return "std::shared_ptr<" + GetReferenceType(true) + ">";
    }
}

std::string MetaVariableReference::GetConstructValue() const
{
    std::stringstream defaultVal;
    if(mPersistentReference)
    {
        defaultVal << GetCodeType() << "()";
    }
    else if(mNullDefault)
    {
        return "nullptr";
    }
    else
    {
        defaultVal << GetCodeType() << "(";
        if(!IsGeneric())
        {
            defaultVal << "new " << GetReferenceType(true);
        }
        defaultVal << ")";
    }

    std::stringstream ss;
    if(mDefaultedVariables.size() > 0)
    {
        ss << "([&]() -> " << GetCodeType() << std::endl
            << "{" << std::endl;
        ss << Generator::Tab(1) << "auto refDefault = " << defaultVal.str() << ";" << std::endl;
        ss << GetConstructorCodeDefaults("refDefault", "", 1);
        ss << Generator::Tab(1) << "return refDefault;" << std::endl
            << "})()";
    }
    else
    {
        ss << defaultVal.str();
    }

    return ss.str();
}

std::string MetaVariableReference::GetConstructorCodeDefaults(const std::string& varName,
    const std::string& parentRef, size_t tabLevel) const
{
    std::stringstream ss;
    bool topLevel = parentRef.length() == 0;
    if(mPersistentReference)
    {
        //Should always be the case
        if(topLevel)
        {
            //The only valid value at this point is a UUID for the persistent reference
            //however we should check regardless
            auto var = mDefaultedVariables.size() > 0 ? mDefaultedVariables.front() : nullptr;
            if(mDefaultedVariables.size() == 1 && var->GetName() == "UID" &&
                var->GetMetaType() == MetaVariableType_t::TYPE_STRING)
            {
                ss << Generator::Tab(tabLevel) << "libobjgen::UUID uuid(" << var->GetDefaultValueCode()
                    << ");" << std::endl;
                ss << Generator::Tab(tabLevel) << varName << ".SetUUID(uuid)" << std::endl;
            }
        }
    }
    else if(mDefaultedVariables.size() > 0)
    {
        ss << Generator::Tab(tabLevel) << "{" << std::endl;
        for(auto var : mDefaultedVariables)
        {
            std::stringstream ssVar;
            ssVar << var->GetName() << "Value";
            std::string localVarName = ssVar.str();

            if(var->GetMetaType() == MetaVariableType_t::TYPE_REF)
            {
                ss << Generator::Tab(tabLevel + 1) << "{" << std::endl;
                ss << GetConstructorCodeDefaults(parentRef + "_ref",
                    localVarName, tabLevel + 1);
                ss << Generator::Tab(tabLevel + 1) << "}" << std::endl;
            }
            else
            {
                ss << Generator::Tab(tabLevel + 1) << "auto " << localVarName
                    << " = " << var->GetConstructValue() << ";" << std::endl;
            }

            ss << Generator::Tab(tabLevel + 1) << varName << "->Set"
                << Generator::GetCapitalName(var) << "(" << localVarName <<
                ");" << std::endl;
        }
        ss << Generator::Tab(tabLevel) << "}" << std::endl;
    }
    return ss.str();
}

std::string MetaVariableReference::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;

    if(recursive)
    {
        if(mPersistentReference)
        {
            std::stringstream ss;
            ss << "nullptr != " << name << ".GetCurrentReference() && (!recursive || "
                << name << ".GetCurrentReference()->IsValid(recursive))";
            return ss.str();
        }
        else
        {
            std::stringstream ss;
            ss << "nullptr != " << name << " && (!recursive || "
                << name << "->IsValid(recursive))";
            return ss.str();
        }
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
    replacements["@CONSTRUCT_VALUE@"] = GetConstructValue();

    if(mPersistentReference)
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
    if(mNullDefault)
    {
        // Null default references do not write to or load from streams
        return "";
    }

    std::map<std::string, std::string> replacements;
    replacements["@VAR_NAME@"] = name;
    replacements["@STREAM@"] = stream;

    if(mPersistentReference)
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
    replacements["@CONSTRUCT_VALUE@"] = GetConstructValue();

    if(mPersistentReference)
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

    if(mPersistentReference)
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
    (void)name;
    (void)node;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_CODE_TYPE@"] = GetCodeType();
    replacements["@DOC@"] = doc;
    replacements["@NODE@"] = node;
    replacements["@CONSTRUCT_VALUE@"] = GetConstructValue();
    replacements["@REF_TYPE@"] = GetReferenceType(true);

    if(mPersistentReference)
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

    if(mPersistentReference)
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

std::string MetaVariableReference::GetAccessDeclarations(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessDeclarations(generator,
        object, name, tabLevel);

    if(mPersistentReference && !IsGeneric())
    {
        ss << generator.Tab(tabLevel) << "const std::shared_ptr<"
            << GetReferenceType(true) << "> Load" << generator.GetCapitalName(*this)
            << "(const std::shared_ptr<libcomp::Database>& db = nullptr);" << std::endl;
    }

    return ss.str();
}

std::string MetaVariableReference::GetAccessFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;
    ss << MetaVariable::GetAccessFunctions(generator, object, name);

    if(mPersistentReference && !IsGeneric())
    {
        ss << "const std::shared_ptr<" << GetReferenceType(true) << "> "
            << object.GetName() << "::Load" << generator.GetCapitalName(*this)
            << "(const std::shared_ptr<libcomp::Database>& db)" << std::endl;
        ss << "{" << std::endl;
        ss << generator.Tab(1) << "return " << generator.GetMemberName(*this)
            << ".Get(db);" << std::endl;
        ss << "}" << std::endl;
    }

    return ss.str();
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
