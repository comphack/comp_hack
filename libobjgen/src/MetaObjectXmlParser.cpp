/**
 * @file libobjgen/src/MetaObjectXmlParser.cpp
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief XML parser for one (or many dependent) metadata object(s).
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

#include "MetaObjectXmlParser.h"

// MetaVariable Types
#include "MetaVariable.h"
#include "MetaVariableArray.h"
#include "MetaVariableEnum.h"
#include "MetaVariableInt.h"
#include "MetaVariableList.h"
#include "MetaVariableMap.h"
#include "MetaVariableSet.h"

using namespace libobjgen;

MetaObjectXmlParser::MetaObjectXmlParser()
{
}

MetaObjectXmlParser::~MetaObjectXmlParser()
{
}

std::string MetaObjectXmlParser::GetError() const
{
    return mError;
}

bool MetaObjectXmlParser::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    if(!LoadTypeInformation(doc, root))
    {
        return false;
    }

    auto objectName = mObject->GetName();
    return LoadMembers(objectName, doc, root);
}

bool MetaObjectXmlParser::LoadTypeInformation(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    mObject = std::shared_ptr<MetaObject>(new MetaObject());

    mError.clear();

    const char *szTagName = root.Name();

    if(nullptr != szTagName && "object" == std::string(szTagName))
    {
        const char *szName = root.Attribute("name");
        const char *szNamespace = root.Attribute("namespace");
        const char *szBaseObject = root.Attribute("baseobject");
        const char *szPersistent = root.Attribute("persistent");
        const char *szInheritedConstruction = root.Attribute(
            "inherited-construction");
        const char *szScriptEnabled = root.Attribute("scriptenabled");

        if(nullptr != szName && mObject->SetName(szName))
        {
            if(nullptr != szNamespace)
            {
                mObject->mNamespace = szNamespace;
            }

            if(nullptr != szPersistent)
            {
                mObject->mPersistent = Generator::GetXmlAttributeBoolean(
                    szPersistent);
            }
            else
            {
                mObject->mPersistent = nullptr == szBaseObject;
            }

            if(nullptr != szInheritedConstruction)
            {
                mObject->mInheritedConstruction = Generator::GetXmlAttributeBoolean(
                    szInheritedConstruction);
            }

            const char *szLocation = root.Attribute("location");
            if(nullptr != szLocation)
            {
                mObject->SetSourceLocation(szLocation);
            }

            if(nullptr != szBaseObject)
            {
                mObject->SetBaseObject(szBaseObject);
            }

            if(nullptr != szScriptEnabled)
            {
                mObject->mScriptEnabled = Generator::GetXmlAttributeBoolean(
                    szScriptEnabled);
            }
            else
            {
                mObject->mScriptEnabled = false;
            }

            std::stringstream ss;
            if(mObject->mPersistent && !mObject->mBaseObject.empty())
            {
                ss << "Persistent object has a base object set: " +mObject->mName;
            }
            else if(!mObject->mPersistent && !mObject->mSourceLocation.empty())
            {
                ss << "Non-persistent object has a source location"
                    " set: " + mObject->mName;
            }

            mError = ss.str();
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

    bool error = !mError.empty();

    if(error)
    {
        mObject = nullptr;
    }
    else
    {
        mError = "Member variables not parsed";
        mKnownObjects[mObject->GetName()] = mObject;
        SetXMLDefinition(root);
    }

    return !error;
}

bool MetaObjectXmlParser::LoadMembers(const std::string& object,
    const tinyxml2::XMLDocument& doc, const tinyxml2::XMLElement& root)
{
    mObject = GetKnownObject(object);
    if(nullptr == mObject)
    {
        std::stringstream ss;
        ss << "Unkown object '" << object
            << "' could not have its members loaded.";

        mError = ss.str();

        return false;
    }

    if(mMemberLoadedObjects.find(object) != mMemberLoadedObjects.end())
    {
        std::stringstream ss;
        ss << "Object '" << object
            << "' has already had its members loaded.";

        mError = ss.str();

        return false;
    }

    mError.clear();
    mMemberLoadedObjects.insert(object);

    //Base objects override the need for member variables
    bool result = mObject->mBaseObject.length() > 0;
    bool error = false;
            
    const tinyxml2::XMLElement *pMember = root.FirstChildElement();

    while(!error && nullptr != pMember)
    {
        if(std::string("member") == pMember->Name())
        {
            error = error || LoadMember(doc, mObject->mName.c_str(), pMember, result);
        }

        pMember = pMember->NextSiblingElement();
    }

    if(!error && mObject->mVariables.empty() && mObject->mBaseObject.empty())
    {
        std::stringstream ss;
        ss << "Object '" << mObject->GetName() << "' has no member variables.";

        mError = ss.str();
    }
    else if(error)
    {
        // Make sure this list is clear if an error occurred.
        mObject->mVariables.clear();
    }

    error |= mError.length() > 0 || !mObject->IsValid();
    if(!result || error)
    {
        return false;
    }

    mError.clear();
    return true;
}

bool MetaObjectXmlParser::FinalizeObjectAndReferences(const std::string& object)
{
    if(mFinalizedObjects.find(object) != mFinalizedObjects.end())
    {
        return true;
    }

    mObject = GetKnownObject(object);
    if(nullptr == mObject)
    {
        std::stringstream ss;
        ss << "Unkown object '" << object
            << "' could not be finalized.";

        mError = ss.str();

        return false;
    }

    std::set<std::string> requiresLoad = { object };
    std::list<std::shared_ptr<libobjgen::MetaVariableReference>> refs;
    while(requiresLoad.size() > 0)
    {
        auto objectName = *requiresLoad.begin();
        if(mObjectXml.find(objectName) == mObjectXml.end())
        {
            std::stringstream ss;
            ss << "Object '" << objectName
                << "' is not defined.";

            mError = ss.str();

            return false;
        }

        if(mMemberLoadedObjects.find(objectName) == mMemberLoadedObjects.end())
        {
            tinyxml2::XMLDocument doc;
            auto xml = mObjectXml[objectName];
            auto err = doc.Parse(xml.c_str(), xml.length());
            if(err != tinyxml2::XML_NO_ERROR)
            {
                std::stringstream ss;
                ss << "Object '" << objectName
                    << "' XML parsing failed.";

                mError = ss.str();

                return false;
            }

            if(!LoadMembers(objectName, doc, *doc.FirstChildElement()))
            {
                return false;
            }
        }

        //Check if the base object needs to be loaded
        auto obj = GetKnownObject(objectName);
        auto baseObject = Generator::GetObjectName(obj->GetBaseObject());
        if(!baseObject.empty() &&
            mFinalizedObjects.find(baseObject) == mFinalizedObjects.end() &&
            requiresLoad.find(baseObject) == requiresLoad.end())
        {
            requiresLoad.insert(baseObject);
        }

        //Check if the referenced objects need to be loaded
        for(auto var : obj->GetReferences())
        {
            auto ref = std::dynamic_pointer_cast<libobjgen::MetaVariableReference>(var);

            if(ref->IsGeneric()) continue;

            auto refType = ref->GetReferenceType();
            refs.push_back(ref);
            if(mFinalizedObjects.find(refType) == mFinalizedObjects.end() &&
                requiresLoad.find(refType) == requiresLoad.end())
            {
                requiresLoad.insert(refType);
            }
        }
        requiresLoad.erase(objectName);
        mFinalizedObjects.insert(objectName);
    }

    mObject = GetKnownObject(object);
    if(HasCircularReference(mObject, std::set<std::string>()))
    {
        std::stringstream ss;
        ss << "Object contains circular reference: "
            << object;

        mError = ss.str();

        return false;
    }

    if(mObject->IsScriptEnabled())
    {
        auto baseObject = Generator::GetObjectName(mObject->GetBaseObject());
        if(!baseObject.empty() &&
            !GetKnownObject(baseObject)->IsScriptEnabled())
        {
            std::stringstream ss;
            ss << "Script enabled object is derived"
                " from an object that is not script enabled: "
                << object;

            mError = ss.str();

            return false;
        }

        for(auto ref : refs)
        {
            auto refType = ref->GetReferenceType();
            if(!GetKnownObject(refType)->IsScriptEnabled())
            {
                std::stringstream ss;
                ss << "Script enabled object references"
                    " an object that is not script enabled: "
                    << object;

                mError = ss.str();

                return false;
            }
        }
    }

    // Now that everything in the chain is loaded up and we know there are no
    // circular refs, set the reference field dynamic sizes
    if(!SetReferenceFieldDynamicSizes(refs))
    {
        std::stringstream ss;
        ss << "Failed to calculate reference field dynamic sizes on object: "
            << object;

        mError = ss.str();

        return false;
    }

    return true;
}

bool MetaObjectXmlParser::SetReferenceFieldDynamicSizes(
    const std::list<std::shared_ptr<libobjgen::MetaVariableReference>>& refs)
{
    if(refs.size() == 0)
    {
        return true;
    }

    std::vector<std::shared_ptr<libobjgen::MetaVariableReference>> remaining
        { std::begin(refs), std::end(refs) };

    // Loop through and keep updating until remaining is empty or nothing
    // more can be updated.
    int updated;
    do
    {
        updated = 0;
        for(int i = (int)remaining.size() - 1; i >= 0; i--)
        {
            auto ref = remaining[(size_t)i];

            if(ref->GetDynamicSizeCount() > 0)
            {
                remaining.erase(remaining.begin() + i);
                updated++;
                continue;
            }

            auto refType = ref->GetReferenceType();
            auto refObject = GetKnownObject(refType);

            bool allRefSizesSet = true;
            for(auto var : refObject->GetReferences())
            {
                auto objRef = std::dynamic_pointer_cast<
                    libobjgen::MetaVariableReference>(var);

                if(objRef->IsGeneric()) continue;

                auto objRefObject = GetKnownObject(objRef->GetReferenceType());
                if(!objRefObject->IsPersistent() &&
                    objRef->GetDynamicSizeCount() == 0)
                {
                    /// @todo This is broken for objects that are not
                    /// persistent and have no dynamic variables.
                    //allRefSizesSet = false;
                }
            }

            if(allRefSizesSet)
            {
                ref->SetDynamicSizeCount(refObject->GetDynamicSizeCount());
                remaining.erase(remaining.begin() + i);
                updated++;
                continue;
            }
        }
    } while(updated > 0);

    return remaining.size() == 0;
}

void MetaObjectXmlParser::SetXMLDefinition(const tinyxml2::XMLElement& root)
{
    tinyxml2::XMLPrinter printer;
    root.Accept(&printer);

    std::stringstream ss;
    ss << printer.CStr();
    mObjectXml[mObject->GetName()] = ss.str();
}

bool MetaObjectXmlParser::LoadMember(const tinyxml2::XMLDocument& doc,
    const char *szName, const tinyxml2::XMLElement *pMember, bool& result)
{
    const char *szMemberName = pMember->Attribute("name");

    if(nullptr != szMemberName && mObject->IsValidIdentifier(szMemberName))
    {
        std::shared_ptr<MetaVariable> var = GetVariable(doc, szName, szMemberName, pMember);
        if(var)
        {
            var->SetName(szMemberName);

            if(var->IsLookupKey() && !mObject->IsPersistent())
            {
                std::stringstream ss;
                ss << "Non-persistent object member variable '"
                    << szMemberName << "' on object '" << szName
                    << "' marked as a lookup key.";

                mError = ss.str();
            }
            else if(mObject->AddVariable(var))
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

std::shared_ptr<MetaVariable> MetaObjectXmlParser::GetVariable(const tinyxml2::XMLDocument& doc,
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
        std::shared_ptr<MetaVariable> var = MetaVariable::CreateType(memberType);

        if(!var && (memberType == "list" || memberType == "array" || memberType == "set" ||
            memberType == "map"))
        {
            std::map<std::string, std::shared_ptr<MetaVariable>> subElems;
            if(memberType == "list" || memberType == "array" || memberType == "set")
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
            else if(memberType == "set")
            {
                if(nullptr != subElems["element"])
                {
                    var = std::shared_ptr<MetaVariable>(new MetaVariableSet(subElems["element"]));
                }
                else
                {
                    std::stringstream ss;
                    ss << "Failed to parse set member '" << szMemberName
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
                        keyMetaType == MetaVariable::MetaVariableType_t::TYPE_SET ||
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
                        valueMetaType == MetaVariable::MetaVariableType_t::TYPE_SET ||
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
        else if(var && var->GetMetaType() == MetaVariable::MetaVariableType_t::TYPE_ENUM)
        {
            std::dynamic_pointer_cast<MetaVariableEnum>(var)->SetTypePrefix(mObject->GetName());
        }
        else if(var && var->GetMetaType() == MetaVariable::MetaVariableType_t::TYPE_REF)
        {
            auto ref = std::dynamic_pointer_cast<MetaVariableReference>(var);

            auto refType = ref->GetReferenceType();
            bool persistentRefType = false;

            bool genericReference = refType.empty();
            if(genericReference)
            {
                persistentRefType = mObject->IsPersistent();
                ref->SetPersistentReference(persistentRefType);
                ref->SetGeneric();
            }
            else
            {
                auto ns = pMember->Attribute("namespace");
                if(ns)
                {
                    ref->SetNamespace(ns);
                }

                if(mKnownObjects.find(refType) == mKnownObjects.end())
                {
                    std::stringstream ss;
                    ss << "Unknown reference type '" << refType << "' encountered"
                        " on field '" << szMemberName << "' in object '" << szName << "'.";

                    mError = ss.str();
                }
                else if(mKnownObjects[refType]->GetNamespace() != ref->GetNamespace())
                {
                    std::stringstream ss;
                    ss << "Reference type '" << refType << "' with invalid namespace "
                        " encountered on field '" << szMemberName << "' in object '"
                        << szName << "'.";

                    mError = ss.str();
                }
                else
                {
                    persistentRefType = mKnownObjects[refType]->IsPersistent();
                    ref->SetPersistentReference(persistentRefType);
                    if(!persistentRefType && mKnownObjects[szName]->IsPersistent())
                    {
                        std::stringstream ss;
                        ss << "Non-peristent reference type '" << refType << "' encountered"
                            " on field '" << szMemberName << "' in persistent object '"
                            << szName << "'.";

                        mError = ss.str();
                    }
                }
            }

            if(mError.length() == 0)
            {
                const tinyxml2::XMLElement *cMember = pMember->FirstChildElement();
                while (nullptr != cMember)
                {
                    std::string cMemberName = cMember->Name();
                    if("member" == cMemberName)
                    {
                        std::string cName = cMember->Attribute("name");
                        if(persistentRefType && cName != "UID")
                        {
                            std::stringstream ss;
                            ss << "Persistent reference type '" << refType
                                << "' on field '" << szMemberName
                                << "' in object '" << szName
                                << "' defaulted with a field other than UID.";

                            mError = ss.str();
                            break;
                        }
                        else if(!persistentRefType && ref->IsGeneric())
                        {
                            std::stringstream ss;
                            ss << "Non-persistent generic reference type"
                                " on field '" << szMemberName << "' in object '" << szName
                                << "' has a default field value set.";

                            mError = ss.str();
                            break;
                        }

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

const tinyxml2::XMLElement *MetaObjectXmlParser::GetChild(
    const tinyxml2::XMLElement *pMember, const std::string name) const
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

bool MetaObjectXmlParser::DefaultsSpecified(const tinyxml2::XMLElement *pMember) const
{
    const char *szMemberType = pMember->Attribute("type");
    if(nullptr == szMemberType)
    {
        return false;
    }

    const std::string memberType(szMemberType);
    auto subVar = MetaVariable::CreateType(memberType);
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

bool MetaObjectXmlParser::HasCircularReference(const std::shared_ptr<MetaObject> obj,
    const std::set<std::string>& references) const
{
    bool status = false;

    if(references.end() != references.find(obj->mName))
    {
        status = true;
    }
    else
    {
        std::set<std::string> referencesCopy = references;
        referencesCopy.insert(obj->mName);

        for(auto var : obj->GetReferences())
        {
            std::shared_ptr<MetaVariableReference> ref =
                std::dynamic_pointer_cast<MetaVariableReference>(var);

            if(ref->IsGeneric()) continue;

            auto refType = ref->GetReferenceType();
            auto refObject = mKnownObjects.find(refType);
            status = refObject != mKnownObjects.end() &&
                !refObject->second->IsPersistent() &&
                HasCircularReference(refObject->second, referencesCopy);

            if(status)
            {
                break;
            }
        }
    }

    return status;
}

std::shared_ptr<MetaObject> MetaObjectXmlParser::GetCurrentObject() const
{
    return mObject;
}

std::shared_ptr<MetaObject> MetaObjectXmlParser::GetKnownObject(const std::string& object) const
{
    auto obj = mKnownObjects.find(object);
    return obj != mKnownObjects.end() ? obj->second : nullptr;
}

std::unordered_map<std::string,
    std::shared_ptr<MetaObject>> MetaObjectXmlParser::GetKnownObjects() const
{
    return mKnownObjects;
}

bool MetaObjectXmlParser::FinalizeClassHierarchy()
{
    for(auto objPair : mKnownObjects)
    {
        auto obj = objPair.second;

        if(!obj->GetBaseObject().empty())
        {
            auto baseName = obj->GetBaseObject();
            auto it = mKnownObjects.find(Generator::GetObjectName(baseName));

            if(mKnownObjects.end() == it)
            {
                std::stringstream ss;
                ss << "Failed to find base object " << baseName
                    << ".";

                mError = ss.str();

                return false;
            }

            auto baseObj = it->second;
            baseObj->AddInheritedObject(obj);
        }
    }

    return true;
}
