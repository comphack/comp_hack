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

std::unordered_map<std::string, MetaObject*> MetaObject::sKnownObjects;

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

        for(auto var : GetReferences())
        {
            std::shared_ptr<MetaVariableReference> ref =
                std::dynamic_pointer_cast<MetaVariableReference>(var);

            auto refObject = sKnownObjects.find(ref->GetReferenceType());
            status = refObject != sKnownObjects.end() &&
                !refObject->second->GetPersistent() &&
                refObject->second->HasCircularReference(referencesCopy);

            if(status)
            {
                break;
            }
        }
    }

    return status;
}

std::set<std::string> MetaObject::GetReferencesTypes() const
{
    std::set<std::string> references;

    for(auto var : GetReferences())
    {
        references.insert(
            std::dynamic_pointer_cast<MetaVariableReference>(var)->GetReferenceType());
    }

    return references;
}

std::list<std::shared_ptr<MetaVariable>> MetaObject::GetReferences() const
{
    std::list<std::shared_ptr<MetaVariable>> references;

    for (auto var : mVariables)
    {
        GetReferences(var, references);
    }

    return references;
}

void MetaObject::GetReferences(std::shared_ptr<MetaVariable>& var,
    std::list<std::shared_ptr<MetaVariable>>& references) const
{
    std::shared_ptr<MetaVariableReference> ref =
        std::dynamic_pointer_cast<MetaVariableReference>(var);

    if(ref)
    {
        references.push_back(ref);
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
