/**
 * @file libobjgen/src/MetaObject.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for an object.
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

#include "MetaObject.h"

// MetaVariable Types
#include "MetaVariable.h"
#include "MetaVariableArray.h"
#include "MetaVariableEnum.h"
#include "MetaVariableInt.h"
#include "MetaVariableList.h"
#include "MetaVariableMap.h"
#include "MetaVariableReference.h"
#include "MetaVariableString.h"

// Standard C++11 Includes
#include <regex>
#include <set>
#include <sstream>

using namespace libobjgen;

MetaObject::MetaObject()
{
    mError = "Not initialized";
}

MetaObject::~MetaObject()
{
}

std::string MetaObject::GetName() const
{
    return mName;
}

std::string MetaObject::GetBaseObject() const
{
    return mBaseObject;
}

bool MetaObject::GetPersistent() const
{
    return mPersistent;
}

std::string MetaObject::GetSourceLocation() const
{
    return mSourceLocation;
}

std::string MetaObject::GetXMLDefinition() const
{
    return mXmlDefinition;
}

std::string MetaObject::GetError() const
{
    return mError;
}

bool MetaObject::SetName(const std::string& name)
{
    if(IsValidIdentifier(name))
    {
        mName = name;
        return true;
    }

    return false;
}

bool MetaObject::SetBaseObject(const std::string& baseObject)
{
    bool result = true;

    mBaseObject = baseObject;

    return result;
}

void MetaObject::SetSourceLocation(const std::string& location)
{
    mSourceLocation = location;
}

void MetaObject::SetXMLDefinition(const std::string& xmlDefinition)
{
    mXmlDefinition = xmlDefinition;
}

void MetaObject::SetXMLDefinition(const tinyxml2::XMLElement& root)
{
    tinyxml2::XMLPrinter printer;
    root.Accept(&printer);

    std::stringstream ss;
    ss << printer.CStr();
    SetXMLDefinition(ss.str());
}

bool MetaObject::AddVariable(const std::shared_ptr<MetaVariable>& var)
{
    bool result = false;

    std::string name = var->GetName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if(IsValidIdentifier(name) && mVariableMapping.end() == mVariableMapping.find(name))
    {
        mVariables.push_back(var);
        mVariableMapping[name] = var;

        result = true;
    }

    return result;
}

bool MetaObject::RemoveVariable(const std::string& name)
{
    bool result = false;

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto entry = mVariableMapping.find(lowerName);

    if(mVariableMapping.end() != entry)
    {
        auto orderedEntry = std::find(mVariables.begin(),
            mVariables.end(), entry->second);

        if(mVariables.end() != orderedEntry)
        {
            mVariables.erase(orderedEntry);
            mVariableMapping.erase(entry);

            result = true;
        }
    }

    return result;
}

std::shared_ptr<MetaVariable> MetaObject::GetVariable(const std::string& name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto entry = mVariableMapping.find(lowerName);

    if(mVariableMapping.end() != entry)
    {
        return entry->second;
    }

    return nullptr;
}

MetaObject::VariableList::const_iterator MetaObject::VariablesBegin() const
{
    return mVariables.begin();
}

MetaObject::VariableList::const_iterator MetaObject::VariablesEnd() const
{
    return mVariables.end();
}

uint16_t MetaObject::GetDynamicSizeCount() const
{
    uint16_t count = 0;

    for(auto var : mVariables)
    {
        count = static_cast<uint16_t>(count + var->GetDynamicSizeCount());
    }

    return count;
}

bool MetaObject::IsValidIdentifier(const std::string& ident)
{
    static const std::string keywordStrings[] = {
        "_Pragma", "alignas", "alignof", "and", "and_eq", "asm",
        "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto", "bitand",
        "bitor", "bool", "break", "case", "catch", "char", "char16_t",
        "char32_t", "class", "compl", "concept", "const", "const_cast",
        "constexpr", "continue", "decltype", "default", "delete", "do",
        "double", "dynamic_cast", "else", "enum", "explicit", "export",
        "extern", "false", "final", "float", "for", "friend", "goto", "if",
        "import", "inline", "int", "long", "module", "mutable", "namespace",
        "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or",
        "or_eq", "override", "private", "protected", "public", "register",
        "reinterpret_cast", "requires", "return", "short", "signed", "sizeof",
        "static", "static_assert", "static_cast", "struct", "switch",
        "synchronized", "template", "this", "thread_local", "throw",
        "transaction_safe", "transaction_safe_dynamic", "true", "try",
        "typedef", "typeid", "typename", "union", "unsigned", "using",
        "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",

        "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t",
        "int64_t", "uint64_t",
    };

    static const std::set<std::string> keywords(keywordStrings,
        keywordStrings + (sizeof(keywordStrings) / sizeof(keywordStrings[0])));

    bool result = true;

    if(keywords.end() != keywords.find(ident))
    {
        result = false;
    }

    if(!std::regex_match(ident, std::regex(
        "^[a-zA-Z_](?:[a-zA-Z0-9][a-zA-Z0-9_]*)?$")))
    {
        result = false;
    }

    return result;
}

bool MetaObject::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    bool result = false;
    bool error = false;

    mError.clear();
    SetXMLDefinition(root);

    const char *szTagName = root.Name();

    if(nullptr != szTagName && "object" == std::string(szTagName))
    {
        const char *szName = root.Attribute("name");
        const char *szBaseObject = root.Attribute("baseobject");
        const char *szPersistent = root.Attribute("persistent");

        if(nullptr != szName && SetName(szName))
        {
            if(nullptr != szPersistent && nullptr == szBaseObject)
            {
                std::string attr(szPersistent);
                mPersistent = "1" == attr || "true" == attr || "on" == attr || "yes" == attr;
            }
            else
            {
                mPersistent = nullptr == szBaseObject;
            }

            if(mPersistent)
            {
                const char *szLocation = root.Attribute("location");
                if(nullptr != szLocation)
                {
                    SetSourceLocation(szLocation);
                }
            }
            else if(nullptr != szBaseObject)
            {
                //Objects cannot be both derived and persistent
                SetBaseObject(szBaseObject);

                //Base objects override the need for member variables
                result = true;
            }
            
            const tinyxml2::XMLElement *pMember = root.FirstChildElement();

            while(!error && nullptr != pMember)
            {
                if(std::string("member") == pMember->Name())
                {
                    error = error || LoadMember(doc, szName, pMember, result);
                }

                pMember = pMember->NextSiblingElement();
            }
        }
        else
        {
            mError = "Object does not have a name attribute.";
        }
    }
    else
    {
        std::stringstream ss;

        if(nullptr != szTagName)
        {
            ss << "Invalid element '" << szTagName << "' detected.";
        }
        else
        {
            ss << "Invalid element detected.";
        }

        mError = ss.str();
    }

    if(!error && mVariables.empty() && mBaseObject.empty())
    {
        std::stringstream ss;
        ss << "Object '" << GetName() << "' has no member variables.";

        mError = ss.str();
        error = true;
    }
    else if(error)
    {
        // Make sure this list is clear if an error occurred.
        mVariables.clear();
    }

    return result && !error;
}

bool MetaObject::LoadMember(const tinyxml2::XMLDocument& doc,
    const char *szName, const tinyxml2::XMLElement *pMember, bool& result)
{
    const char *szMemberName = pMember->Attribute("name");

    if(nullptr != szMemberName && IsValidIdentifier(szMemberName))
    {
        std::shared_ptr<MetaVariable> var = GetVariable(doc, szName, szMemberName, pMember);
        if(var)
        {
            var->SetName(szMemberName);

            if(var->IsLookupKey() && !GetPersistent())
            {
                std::stringstream ss;
                ss << "Non-persistent object member variable '"
                    << szMemberName << "' on object '" << szName
                    << "' marked as a lookup key.";

                mError = ss.str();
            }
            else if(AddVariable(var))
            {
                // At least one variable is added now. The result
                // is OK unless an error causes a problem.
                result = true;
            }
            else
            {
                std::stringstream ss;
                ss << "Failed to add member variable '"
                    << szMemberName << "' to object '" << szName
                    << "'. A variable by the same name may "
                    << "already exist.";

                mError = ss.str();
            }
        }
    }
    else
    {
        std::stringstream ss;
        ss << "Member variable for object '" << szName
            << "' does not have a valid name attribute.";

        mError = ss.str();
    }

    return mError.length() > 0;
}

std::shared_ptr<MetaVariable> MetaObject::GetVariable(const tinyxml2::XMLDocument& doc,
    const char *szName, const char *szMemberName, const tinyxml2::XMLElement *pMember)
{
    std::shared_ptr<MetaVariable> retval = nullptr;

    const char *szMemberType = pMember->Attribute("type");
    if(nullptr == szMemberType)
    {
        std::stringstream ss;
        ss << "Member variable '" << szMemberName
            << "' for object '" << szName
            << "' does not have a type attribute.";

        mError = ss.str();
    }
    else
    {
        const std::string memberType(szMemberType);
        std::shared_ptr<MetaVariable> var = CreateType(memberType);

        if(!var && (memberType == "list" || memberType == "array" || memberType == "map"))
        {
            std::map<std::string, std::shared_ptr<MetaVariable>> subElems;
            if(memberType == "list" || memberType == "array")
            {
                subElems["element"];
            }
            else if(memberType == "map")
            {
                subElems["key"];
                subElems["value"];
            }

            for(auto kv : subElems)
            {
                const tinyxml2::XMLElement *seMember = GetChild(pMember, kv.first);

                if(seMember)
                {
                    subElems[kv.first] = GetVariable(doc, szName, szMemberName, seMember);
                }
            }

            if(memberType == "list")
            {
                if(nullptr != subElems["element"])
                {
                    var = std::shared_ptr<MetaVariable>(new MetaVariableList(subElems["element"]));
                }
                else
                {
                    std::stringstream ss;
                    ss << "Failed to parse list member '" << szMemberName
                        << "' element in object '"
                        << szName << "'";

                    mError = ss.str();
                }
            }
            else if(memberType == "array")
            {
                if(nullptr != subElems["element"])
                {
                    var = std::shared_ptr<MetaVariable>(new MetaVariableArray(subElems["element"]));
                }
                else
                {
                    std::stringstream ss;
                    ss << "Failed to parse array member '" << szMemberName
                        << "' element in object '"
                        << szName << "'";

                    mError = ss.str();
                }
            }
            else if(memberType == "map")
            {
                if(nullptr != subElems["key"] && nullptr != subElems["value"])
                {
                    auto key = subElems["key"];
                    auto value = subElems["value"];

                    auto keyMetaType = key->GetMetaType();
                    auto valueMetaType = value->GetMetaType();

                    if(keyMetaType == MetaVariable::MetaVariableType_t::TYPE_ARRAY ||
                        keyMetaType == MetaVariable::MetaVariableType_t::TYPE_LIST ||
                        keyMetaType == MetaVariable::MetaVariableType_t::TYPE_MAP ||
                        keyMetaType == MetaVariable::MetaVariableType_t::TYPE_REF)
                    {
                        std::stringstream ss;
                        ss << "Invalid map key type of '" << key->GetType()
                            << "' specified for member '" << szMemberName
                            << "' on object '"
                            << szName << "'";

                        mError = ss.str();
                    }
                    else if(valueMetaType == MetaVariable::MetaVariableType_t::TYPE_ARRAY ||
                        valueMetaType == MetaVariable::MetaVariableType_t::TYPE_LIST ||
                        valueMetaType == MetaVariable::MetaVariableType_t::TYPE_MAP)
                    {
                        std::stringstream ss;
                        ss << "Invalid map key type of '" << value->GetType()
                            << "' specified for member '" << szMemberName
                            << "' on object '"
                            << szName << "'";

                        mError = ss.str();
                    }
                    else
                    {
                        var = std::shared_ptr<MetaVariable>(
                            new MetaVariableMap(subElems["key"], subElems["value"]));
                    }
                }
                else
                {
                    std::stringstream ss;
                    ss << "Failed to parse map member '" << szMemberName
                        << "' key and value on object '"
                        << szName << "'";

                    mError = ss.str();
                }
            }
        }
        else if(var && var->GetMetaType() == MetaVariable::MetaVariableType_t::TYPE_REF)
        {
            auto ref = std::dynamic_pointer_cast<MetaVariableReference>(var);

            const tinyxml2::XMLElement *cMember = pMember->FirstChildElement();
            while (nullptr != cMember)
            {
                std::string cMemberName = cMember->Name();
                if("member" == cMemberName)
                {
                    std::string cName = cMember->Attribute("name");
                    if(!DefaultsSpecified(cMember))
                    {
                        std::stringstream ss;
                        ss << "Non-defaulted member in reference '" << szMemberName
                            << "' in object '" << szName << "'.";

                        mError = ss.str();
                    }
                    else if(cName.length() == 0)
                    {
                        std::stringstream ss;
                        ss << "Non-defaulted member in reference '" << szMemberName
                            << "' in object '" << szName << "' does not have a name specified.";

                        mError = ss.str();
                    }
                    else
                    {
                        auto subVariable = GetVariable(doc, szName, szMemberName, cMember);
                        subVariable->SetName(cName);
                        if(subVariable && subVariable->Load(doc, *cMember))
                        {
                            ref->AddDefaultedVariable(subVariable);
                        }
                        else
                        {
                            std::stringstream ss;
                            ss << "Failed to parse defaulted member in reference '" << szMemberName
                                << "' in object '" << szName << "': " << var->GetError();

                            mError = ss.str();
                        }
                    }
                }
                cMember = cMember->NextSiblingElement();
            }
        }

        if(var && var->Load(doc, *pMember))
        {
            retval = var;
        }
        else if(var)
        {
            std::stringstream ss;
            ss << "Failed to parse member '" << szMemberName
                << "' of type '" << szMemberType << "' in object '"
                << szName << "': " << var->GetError();

            mError = ss.str();
        }
        else
        {
            std::stringstream ss;
            ss << "Unknown member type '" << szMemberType
                << "' for object '" << szName << "'.";

            mError = ss.str();
        }
    }

    return retval;
}

bool MetaObject::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& root) const
{
    tinyxml2::XMLElement *pObjectElement = doc.NewElement("object");
    pObjectElement->SetAttribute("name", mName.c_str());

    root.InsertEndChild(pObjectElement);

    for(auto var : mVariables)
    {
        if(!var->Save(doc, *pObjectElement, "member"))
        {
            return false;
        }
    }

    return true;
}

std::shared_ptr<MetaVariable> MetaObject::CreateType(
    const std::string& typeName)
{
    std::shared_ptr<MetaVariable> var;

    static std::regex re("^([a-zA-Z_](?:[a-zA-Z0-9][a-zA-Z0-9_]*)?)[*]$");

    static std::unordered_map<std::string,
        std::shared_ptr<MetaVariable> (*)()> objectCreatorFunctions;

    if(objectCreatorFunctions.empty())
    {
        objectCreatorFunctions["u8"]  = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<uint8_t>()); };
        objectCreatorFunctions["u16"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<uint16_t>()); };
        objectCreatorFunctions["u32"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<uint32_t>()); };
        objectCreatorFunctions["u64"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<uint64_t>()); };

        objectCreatorFunctions["s8"]  = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<int8_t>()); };
        objectCreatorFunctions["s16"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<int16_t>()); };
        objectCreatorFunctions["s32"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<int32_t>()); };
        objectCreatorFunctions["s64"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<int64_t>()); };

        objectCreatorFunctions["f32"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<float>()); };
        objectCreatorFunctions["float"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<float>()); };
        objectCreatorFunctions["single"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<float>()); };

        objectCreatorFunctions["f64"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<double>()); };
        objectCreatorFunctions["double"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableInt<double>()); };

        objectCreatorFunctions["enum"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableEnum()); };

        objectCreatorFunctions["string"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableString()); };
    }

    auto creatorFunctionPair = objectCreatorFunctions.find(typeName);

    std::smatch match;

    if(std::regex_match(typeName, match, re))
    {
        var = std::shared_ptr<MetaVariable>(new MetaVariableReference());

        if(!var || !std::dynamic_pointer_cast<MetaVariableReference>(
            var)->SetReferenceType(match[1]))
        {
            // The type isn't valid, free the object.
            var.reset();
        }
    }
    else if(objectCreatorFunctions.end() != creatorFunctionPair)
    {
        // Create the object of the desired type.
        var = (*creatorFunctionPair->second)();
    }

    return var;
}

bool MetaObject::HasCircularReference() const
{
    return HasCircularReference(std::set<std::string>());
}

bool MetaObject::HasCircularReference(
    const std::set<std::string>& references) const
{
    bool status = false;

    if(references.end() != references.find(mName))
    {
        status = true;
    }
    else
    {
        std::set<std::string> referencesCopy;
        referencesCopy.insert(mName);

        for(auto var : mVariables)
        {
            std::shared_ptr<MetaVariableReference> ref =
                std::dynamic_pointer_cast<MetaVariableReference>(var);

            // @todo Lookup the type for the next reference type and call
            // HasCircularReference(referencesCopy) on it.
            //status = Lookup(ref->GetReferenceType())->HasCircularReference(
                //referencesCopy))

            if(status)
            {
                break;
            }
        }
    }

    return status;
}

const tinyxml2::XMLElement *MetaObject::GetChild(const tinyxml2::XMLElement *pMember,
    const std::string name) const
{
    const tinyxml2::XMLElement *cMember = pMember->FirstChildElement();
    while(nullptr != cMember)
    {
        if(name == cMember->Name())
        {
            return cMember;
        }

        cMember = cMember->NextSiblingElement();
    }
    return nullptr;
}

bool MetaObject::DefaultsSpecified(const tinyxml2::XMLElement *pMember) const
{
    const char *szMemberType = pMember->Attribute("type");
    if(nullptr == szMemberType)
    {
        return false;
    }

    const std::string memberType(szMemberType);
    auto subVar = CreateType(memberType);
    if(subVar && subVar->GetMetaType() != MetaVariable::MetaVariableType_t::TYPE_REF)
    {
        return nullptr != pMember->Attribute("default");
    }

    std::set<std::string> subMembers;
    if(subVar)
    {
        //Ref
        subMembers.insert("member");
    }
    else if(memberType == "array" || memberType == "list")
    {
        subMembers.insert("entry");
    }
    else if(memberType == "map")
    {
        subMembers.insert("key");
        subMembers.insert("value");
    }
    else
    {
        return false;
    }
    
    const tinyxml2::XMLElement *cMember = pMember->FirstChildElement();
    while(nullptr != cMember)
    {
        if(subMembers.find(cMember->Name()) != subMembers.end() && !DefaultsSpecified(cMember))
        {
            return false;
        }

        cMember = cMember->NextSiblingElement();
    }

    return true;
}

std::set<std::string> MetaObject::GetReferences() const
{
    std::set<std::string> references;

    for(auto var : mVariables)
    {
        GetReferences(var, references);
    }

    return references;
}

void MetaObject::GetReferences(std::shared_ptr<MetaVariable>& var,
    std::set<std::string>& references) const
{
    std::shared_ptr<MetaVariableReference> ref =
        std::dynamic_pointer_cast<MetaVariableReference>(var);

    if(ref)
    {
        references.insert(ref->GetReferenceType());
    }
    else
    {
        switch(var->GetMetaType())
        {
            case MetaVariable::MetaVariableType_t::TYPE_ARRAY:
                {
                    std::shared_ptr<MetaVariableArray> array =
                        std::dynamic_pointer_cast<MetaVariableArray>(var);

                    auto elementType = array->GetElementType();
                    GetReferences(elementType, references);
                }
                break;
            case MetaVariable::MetaVariableType_t::TYPE_LIST:
                {
                    std::shared_ptr<MetaVariableList> list =
                        std::dynamic_pointer_cast<MetaVariableList>(var);

                    auto elementType = list->GetElementType();
                    GetReferences(elementType, references);
                }
                break;
            case MetaVariable::MetaVariableType_t::TYPE_MAP:
                {
                    std::shared_ptr<MetaVariableMap> map =
                        std::dynamic_pointer_cast<MetaVariableMap>(var);

                    auto elementType = map->GetKeyElementType();
                    GetReferences(elementType, references);
                    elementType = map->GetValueElementType();
                    GetReferences(elementType, references);
                }
                break;
            default:
                break;
        }
    }
}
