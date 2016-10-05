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
#include "MetaVariableInt.h"
#include "MetaVariableList.h"
#include "MetaVariableReference.h"
#include "MetaVariableString.h"

// Standard C++11 Includes
#include <regex>
#include <set>
#include <sstream>

using namespace libobjgen;

MetaObject::MetaObject()
{
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

std::string MetaObject::GetError() const
{
    return mError;
}

bool MetaObject::SetName(const std::string& name)
{
    bool result = true;

    /// @todo Validate the name.
    mName = name;

    return result;
}

bool MetaObject::SetBaseObject(const std::string& baseObject)
{
    bool result = true;

    mBaseObject = baseObject;

    return result;
}

bool MetaObject::AddVariable(const std::string& name,
    const std::shared_ptr<MetaVariable>& var)
{
    bool result = false;

    if(IsValidIdentifier(name) && mVariableMapping.end() == mVariableMapping.find(name))
    {
        mVariables.push_back(var);
        mVariableMapping[name] = var;
        var->SetName(name);

        result = true;
    }

    return result;
}

bool MetaObject::RemoveVariable(const std::string& name)
{
    bool result = false;

    auto entry = mVariableMapping.find(name);

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

MetaObject::VariableList::const_iterator MetaObject::VariablesBegin() const
{
    return mVariables.begin();
}

MetaObject::VariableList::const_iterator MetaObject::VariablesEnd() const
{
    return mVariables.end();
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

    const char *szTagName = root.Name();

    if(nullptr != szTagName && "object" == std::string(szTagName))
    {
        const char *szName = root.Attribute("name");
        const char *szBaseObject = root.Attribute("baseobject");

        if(nullptr != szName && SetName(szName))
        {
            if (szBaseObject != nullptr)
            {
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
                else if(std::string("array") == pMember->Name())
                {
                    error = error || LoadArray(doc, szName, pMember, result);
                }
                else if(std::string("list") == pMember->Name())
                {
                    error = error || LoadList(doc, szName, pMember, result);
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
    bool error = false;

    const char *szMemberType = pMember->Attribute("type");
    const char *szMemberName = pMember->Attribute("name");

    if(nullptr != szMemberType && nullptr != szMemberName)
    {
        std::shared_ptr<MetaVariable> var = CreateType(
            szMemberType);

        if(var)
        {
            var->SetName(szMemberName);
        }

        if(var && var->Load(doc, *pMember))
        {
            if(AddVariable(szMemberName, var))
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
                error = true;
            }
        }
        else if(var)
        {
            std::stringstream ss;
            ss << "Failed to parse member '" << szMemberName
                << "' of type '" << szMemberType << "' in object '"
                << szName << "': " << var->GetError();

            mError = ss.str();
            error = true;
        }
        else
        {
            std::stringstream ss;
            ss << "Unknown member type '" << szMemberType
                << "' for object '" << szName << "'.";

            mError = ss.str();
            error = true;
        }
    }
    else if(nullptr == szMemberName)
    {
        std::stringstream ss;
        ss << "Member variable for object '" << szName
            << "' does not have a name attribute.";

        mError = ss.str();
        error = true;
    }
    else
    {
        std::stringstream ss;
        ss << "Member variable '" << szMemberName
            << "' for object '" << szName
            << "' does not have a type attribute.";

        mError = ss.str();
        error = true;
    }

    return error;
}

bool MetaObject::LoadArray(const tinyxml2::XMLDocument& doc,
    const char *szName, const tinyxml2::XMLElement *pMember, bool& result)
{
    bool error = false;

    const char *szMemberType = pMember->Attribute("type");
    const char *szMemberName = pMember->Attribute("name");

    if(nullptr != szMemberType && nullptr != szMemberName)
    {
        std::shared_ptr<MetaVariable> elementType = CreateType(
            szMemberType);
        std::shared_ptr<MetaVariable> var(new MetaVariableArray(elementType));
        
        if(var)
        {
            var->SetName(szMemberName);
        }

        if(elementType)
        {
            elementType->SetName("none");
        }

        if(var && var->Load(doc, *pMember) && elementType &&
            elementType->Load(doc, *pMember))
        {
            if(AddVariable(szMemberName, var))
            {
                // At least one variable is added now. The result
                // is OK unless an error causes a problem.
                result = true;
            }
            else
            {
                std::stringstream ss;
                ss << "Failed to add array variable '"
                    << szMemberName << "' to object '" << szName
                    << "'. A variable by the same name may "
                    << "already exist.";

                mError = ss.str();
                error = true;
            }
        }
        else if(var)
        {
            std::stringstream ss;
            ss << "Failed to parse array '" << szMemberName
                << "' of type '" << szMemberType << "' in object '"
                << szName << "': " << var->GetError();

            mError = ss.str();
            error = true;
        }
        else
        {
            std::stringstream ss;
            ss << "Unknown array type '" << szMemberType
                << "' for object '" << szName << "'.";

            mError = ss.str();
            error = true;
        }
    }
    else if(nullptr == szMemberName)
    {
        std::stringstream ss;
        ss << "Array variable for object '" << szName
            << "' does not have a name attribute.";

        mError = ss.str();
        error = true;
    }
    else
    {
        std::stringstream ss;
        ss << "Array variable '" << szMemberName
            << "' for object '" << szName
            << "' does not have a type attribute.";

        mError = ss.str();
        error = true;
    }

    return error;
}

bool MetaObject::LoadList(const tinyxml2::XMLDocument& doc,
    const char *szName, const tinyxml2::XMLElement *pMember, bool& result)
{
    bool error = false;

    const char *szMemberType = pMember->Attribute("type");
    const char *szMemberName = pMember->Attribute("name");

    if(nullptr != szMemberType && nullptr != szMemberName)
    {
        std::shared_ptr<MetaVariable> elementType = CreateType(
            szMemberType);
        std::shared_ptr<MetaVariable> var(new MetaVariableList(elementType));
        
        if(var)
        {
            var->SetName(szMemberName);
        }

        if(elementType)
        {
            elementType->SetName("none");
        }

        if(var && var->Load(doc, *pMember) && elementType &&
            elementType->Load(doc, *pMember))
        {
            if(AddVariable(szMemberName, var))
            {
                // At least one variable is added now. The result
                // is OK unless an error causes a problem.
                result = true;
            }
            else
            {
                std::stringstream ss;
                ss << "Failed to add list variable '"
                    << szMemberName << "' to object '" << szName
                    << "'. A variable by the same name may "
                    << "already exist.";

                mError = ss.str();
                error = true;
            }
        }
        else if(var)
        {
            std::stringstream ss;
            ss << "Failed to parse list '" << szMemberName
                << "' of type '" << szMemberType << "' in object '"
                << szName << "': " << var->GetError();

            mError = ss.str();
            error = true;
        }
        else
        {
            std::stringstream ss;
            ss << "Unknown list type '" << szMemberType
                << "' for object '" << szName << "'.";

            mError = ss.str();
            error = true;
        }
    }
    else if(nullptr == szMemberName)
    {
        std::stringstream ss;
        ss << "List variable for object '" << szName
            << "' does not have a name attribute.";

        mError = ss.str();
        error = true;
    }
    else
    {
        std::stringstream ss;
        ss << "List variable '" << szMemberName
            << "' for object '" << szName
            << "' does not have a type attribute.";

        mError = ss.str();
        error = true;
    }

    return error;
}

bool MetaObject::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& root) const
{
    tinyxml2::XMLElement *pObjectElement = doc.NewElement("object");
    pObjectElement->SetAttribute("name", mName.c_str());

    root.InsertEndChild(pObjectElement);

    for(auto var : mVariables)
    {
        if(!var->Save(doc, *pObjectElement))
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

        objectCreatorFunctions["string"] = []() { return std::shared_ptr<
            MetaVariable>(new MetaVariableString()); };
    }

    auto creatorFunctionPair = objectCreatorFunctions.find(typeName);

    std::smatch match;

    if(std::regex_match(typeName, match, re))
    {
        var = std::shared_ptr<MetaVariable>(new MetaVariableReference());

        if(var && std::dynamic_pointer_cast<MetaVariableReference>(
            var)->SetReferenceType(match[1]))
        {
            /// @todo Add this type to the MetaObject cache that must be
            /// resolved by the time the load completes.
        }
        else
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

std::set<std::string> MetaObject::GetReferences() const
{
    std::set<std::string> references;

    for(auto var : mVariables)
    {
        std::shared_ptr<MetaVariableReference> ref =
            std::dynamic_pointer_cast<MetaVariableReference>(var);

        if(ref)
        {
            references.insert(ref->GetReferenceType());
        }
        else
        {
            std::shared_ptr<MetaVariableArray> array =
                std::dynamic_pointer_cast<MetaVariableArray>(var);

            if(array)
            {
                ref = std::dynamic_pointer_cast<MetaVariableReference>(
                    array->GetElementType());

                if(ref)
                {
                    references.insert(ref->GetReferenceType());
                }
            }
        }
    }

    return references;
}
