/**
 * @file libobjgen/src/MetaVariable.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for an object member variable.
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

// libobjgen Includes
#include "Generator.h"
#include "MetaVariable.h"
#include "MetaObject.h"

// MetaVariable Types
#include "MetaVariable.h"
#include "MetaVariableArray.h"
#include "MetaVariableBool.h"
#include "MetaVariableEnum.h"
#include "MetaVariableInt.h"
#include "MetaVariableList.h"
#include "MetaVariableSet.h"
#include "MetaVariableMap.h"
#include "MetaVariableReference.h"
#include "MetaVariableSet.h"
#include "MetaVariableString.h"

// Standard C++11 Includes
#include <algorithm>
#include <sstream>

using namespace libobjgen;

MetaVariable::MetaVariable() : mCaps(false),
    mInherited(false), mLookupKey(false), mUniqueKey(false)
{
}

std::string MetaVariable::GetName() const
{
    return mName;
}

void MetaVariable::SetName(const std::string& name)
{
    mName = name;
}

bool MetaVariable::IsInherited() const
{
    return mInherited;
}

void MetaVariable::SetInherited(const bool inherited)
{
    mInherited = inherited;
}

bool MetaVariable::IsLookupKey() const
{
    return mLookupKey;
}

void MetaVariable::SetLookupKey(const bool lookupKey)
{
    mLookupKey = lookupKey;

    //Always default to the same value when set
    mUniqueKey = mLookupKey;
}

bool MetaVariable::IsUniqueKey() const
{
    return mUniqueKey;
}

bool MetaVariable::SetUniqueKey(const bool uniqueKey)
{
    if(!mLookupKey)
    {
        return false;
    }

    mUniqueKey = uniqueKey;
    return true;
}

std::string MetaVariable::GetError() const
{
    return mError;
}

bool MetaVariable::IsCaps() const
{
    return mCaps;
}

void MetaVariable::SetCaps(bool caps)
{
    mCaps = caps;
}

uint16_t MetaVariable::GetDynamicSizeCount() const
{
    return 0;
}

bool MetaVariable::Load(std::istream& stream)
{
    Generator::LoadString(stream, mName);
    stream.read(reinterpret_cast<char*>(&mCaps),
        sizeof(mCaps));
    stream.read(reinterpret_cast<char*>(&mInherited),
        sizeof(mInherited));
    stream.read(reinterpret_cast<char*>(&mLookupKey),
        sizeof(mLookupKey));
    stream.read(reinterpret_cast<char*>(&mUniqueKey),
        sizeof(mUniqueKey));

    return stream.good();
}

bool MetaVariable::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid())
    {
        Generator::SaveString(stream, mName);
        stream.write(reinterpret_cast<const char*>(&mCaps),
            sizeof(mCaps));
        stream.write(reinterpret_cast<const char*>(&mInherited),
            sizeof(mInherited));
        stream.write(reinterpret_cast<const char*>(&mLookupKey),
            sizeof(mLookupKey));
        stream.write(reinterpret_cast<const char*>(&mUniqueKey),
            sizeof(mUniqueKey));

        result = stream.good();
    }

    return result;
}

bool MetaVariable::LoadVariableList(std::istream& stream,
    std::list<std::shared_ptr<MetaVariable>>& vars)
{
    size_t varCount;
    stream.read(reinterpret_cast<char*>(&varCount),
        sizeof(varCount));

    if(stream.good())
    {
        for(size_t i = 0; i < varCount; i++)
        {
            MetaVariableType_t metaType;
            stream.read(reinterpret_cast<char*>(&metaType),
                sizeof(metaType));

            short subTypeCount;
            stream.read(reinterpret_cast<char*>(&subTypeCount),
                sizeof(subTypeCount));

            if(!stream.good())
            {
                return false;
            }

            std::vector<MetaVariableType_t> subTypes;
            for(short k = 0; k < subTypeCount; k++)
            {
                MetaVariableType_t subMetaType;
                stream.read(reinterpret_cast<char*>(&subMetaType),
                    sizeof(subMetaType));
                subTypes.push_back(subMetaType);
            }

            if(!stream.good())
            {
                return false;
            }

            auto var = CreateType(metaType, subTypes);
            if(!var || !var->Load(stream))
            {
                return false;
            }
            vars.push_back(var);
        }
    }

    return stream.good() && varCount == vars.size();
}

bool MetaVariable::SaveVariableList(std::ostream& stream,
    const std::list<std::shared_ptr<MetaVariable>>& vars)
{
    size_t varCount = vars.size();
    stream.write(reinterpret_cast<const char*>(&varCount),
        sizeof(varCount));
    
    if(stream.good())
    {
        for(auto var : vars)
        {
            auto metaType = var->GetMetaType();
            stream.write(reinterpret_cast<const char*>(&metaType),
                sizeof(metaType));

            std::list<std::shared_ptr<MetaVariable>> subTypes;
            switch(metaType)
            {
                case MetaVariableType_t::TYPE_ARRAY:
                    {
                        auto arr = std::dynamic_pointer_cast<
                            MetaVariableArray>(var);

                        subTypes.push_back(arr->GetElementType());
                    }
                    break;
                case MetaVariableType_t::TYPE_LIST:
                    {
                        auto l = std::dynamic_pointer_cast<
                            MetaVariableList>(var);

                        subTypes.push_back(l->GetElementType());
                    }
                    break;
                case MetaVariableType_t::TYPE_SET:
                    {
                        auto l = std::dynamic_pointer_cast<
                            MetaVariableSet>(var);

                        subTypes.push_back(l->GetElementType());
                    }
                    break;
                case MetaVariableType_t::TYPE_MAP:
                    {
                        auto m = std::dynamic_pointer_cast<
                            MetaVariableMap>(var);

                        subTypes.push_back(m->GetKeyElementType());
                        subTypes.push_back(m->GetValueElementType());
                    }
                    break;
                default:
                    break;
            }

            short subTypeCount = (short)subTypes.size();
            stream.write(reinterpret_cast<const char*>(&subTypeCount),
                sizeof(subTypeCount));

            for(auto subType : subTypes)
            {
                auto subMetaType = subType->GetMetaType();
                stream.write(reinterpret_cast<const char*>(&subMetaType),
                    sizeof(subMetaType));
            }

            if(!var->Save(stream))
            {
                return false;
            }
        }
    }

    return stream.good();
}

std::shared_ptr<MetaVariable> MetaVariable::CreateType(
    const std::string& typeName)
{
    std::smatch match;
    static std::regex re("^([a-zA-Z_](?:[a-zA-Z0-9][a-zA-Z0-9_]*)?)[*]$");

    if(std::regex_match(typeName, match, re))
    {
        auto var = CreateType(MetaVariableType_t::TYPE_REF);

        if(!var || !std::dynamic_pointer_cast<MetaVariableReference>(
            var)->SetReferenceType(match[1]))
        {
            // The type isn't valid, free the object.
            var.reset();
        }

        return var;
    }

    static std::unordered_map<std::string, MetaVariableType_t> nameToMetaType;

    if(nameToMetaType.empty())
    {
        nameToMetaType["bit"] = MetaVariableType_t::TYPE_BOOL;
        nameToMetaType["bool"] = MetaVariableType_t::TYPE_BOOL;
        nameToMetaType["flag"] = MetaVariableType_t::TYPE_BOOL;

        nameToMetaType["u8"] = MetaVariableType_t::TYPE_U8;
        nameToMetaType["u16"] = MetaVariableType_t::TYPE_U16;
        nameToMetaType["u32"] = MetaVariableType_t::TYPE_U32;
        nameToMetaType["u64"] = MetaVariableType_t::TYPE_U64;

        nameToMetaType["s8"] = MetaVariableType_t::TYPE_S8;
        nameToMetaType["s16"] = MetaVariableType_t::TYPE_S16;
        nameToMetaType["s32"] = MetaVariableType_t::TYPE_S32;
        nameToMetaType["s64"] = MetaVariableType_t::TYPE_S64;

        nameToMetaType["f32"] = MetaVariableType_t::TYPE_FLOAT;
        nameToMetaType["float"] = MetaVariableType_t::TYPE_FLOAT;
        nameToMetaType["single"] = MetaVariableType_t::TYPE_FLOAT;

        nameToMetaType["f64"] = MetaVariableType_t::TYPE_DOUBLE;
        nameToMetaType["double"] = MetaVariableType_t::TYPE_DOUBLE;

        nameToMetaType["enum"] = MetaVariableType_t::TYPE_ENUM;

        nameToMetaType["string"] = MetaVariableType_t::TYPE_STRING;

        nameToMetaType["ref"] = MetaVariableType_t::TYPE_REF;
        nameToMetaType["pref"] = MetaVariableType_t::TYPE_REF;
    }

    auto iter = nameToMetaType.find(typeName);
    return iter != nameToMetaType.end() ? CreateType(iter->second) : nullptr;
}


std::shared_ptr<MetaVariable> MetaVariable::CreateType(
    const MetaVariableType_t type,
    std::vector<MetaVariableType_t> subtypes)
{
    switch(type)
    {
        case MetaVariableType_t::TYPE_BOOL:
            return std::shared_ptr<MetaVariable>(new MetaVariableBool());
        case MetaVariableType_t::TYPE_S8:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<int8_t>());
        case MetaVariableType_t::TYPE_U8:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<uint8_t>());
        case MetaVariableType_t::TYPE_S16:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<int16_t>());
        case MetaVariableType_t::TYPE_U16:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<uint16_t>());
        case MetaVariableType_t::TYPE_S32:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<int32_t>());
        case MetaVariableType_t::TYPE_U32:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<uint32_t>());
        case MetaVariableType_t::TYPE_S64:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<int64_t>());
        case MetaVariableType_t::TYPE_U64:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<uint64_t>());
        case MetaVariableType_t::TYPE_FLOAT:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<float>());
        case MetaVariableType_t::TYPE_DOUBLE:
            return std::shared_ptr<MetaVariable>(new MetaVariableInt<double>());
        case MetaVariableType_t::TYPE_ENUM:
            return std::shared_ptr<MetaVariable>(new MetaVariableEnum());
        case MetaVariableType_t::TYPE_STRING:
            return std::shared_ptr<MetaVariable>(new MetaVariableString());
        case MetaVariableType_t::TYPE_ARRAY:
            if(subtypes.size() == 1)
            {
                auto elementType = CreateType(subtypes[0]);
                if(nullptr != elementType)
                {
                    return std::shared_ptr<MetaVariable>(
                        new MetaVariableArray(elementType));
                }
            }
            break;
        case MetaVariableType_t::TYPE_LIST:
            if(subtypes.size() == 1)
            {
                auto elementType = CreateType(subtypes[0]);
                if(nullptr != elementType)
                {
                    return std::shared_ptr<MetaVariable>(
                        new MetaVariableList(elementType));
                }
            }
            break;
        case MetaVariableType_t::TYPE_SET:
            if(subtypes.size() == 1)
            {
                auto elementType = CreateType(subtypes[0]);
                if(nullptr != elementType)
                {
                    return std::shared_ptr<MetaVariable>(
                        new MetaVariableSet(elementType));
                }
            }
            break;
        case MetaVariableType_t::TYPE_MAP:
            if(subtypes.size() == 2)
            {
                auto keyType = CreateType(subtypes[0]);
                auto valueType = CreateType(subtypes[1]);
                if(nullptr != keyType && nullptr != valueType)
                {
                    return std::shared_ptr<MetaVariable>(
                        new MetaVariableMap(keyType, valueType));
                }
            }
            break;
        case MetaVariableType_t::TYPE_REF:
            return std::shared_ptr<MetaVariable>(new MetaVariableReference());
            break;
        default:
            break;
    }

    return nullptr;
}

std::string MetaVariable::GetDeclaration(const std::string& name) const
{
    std::string decl;

    if(MetaObject::IsValidIdentifier(name))
    {
        std::stringstream ss;

        ss << GetCodeType() << " " << name << ";";

        decl = ss.str();
    }

    return decl;
}

std::string MetaVariable::GetArgumentType() const
{
    std::stringstream ss;
    ss << "const " << GetCodeType() << "&";
    return ss.str();
}

std::string MetaVariable::GetArgument(const std::string& name) const
{
    std::string arg;

    if(MetaObject::IsValidIdentifier(name))
    {
        std::stringstream ss;

        ss << GetArgumentType() << " " << name;

        arg = ss.str();
    }

    return arg;
}

std::string MetaVariable::GetDefaultValueCode() const
{
    return GetCodeType() + "{}";
}

std::string MetaVariable::GetGetterCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;
    ss << generator.Tab(tabLevel) << "return " << name << ";" << std::endl;

    return ss.str();
}

std::string MetaVariable::GetBindValueCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@SAVE_CODE@"] = GetSaveRawCode(generator, name, "stream");

    return generator.ParseTemplate(tabLevel, "VariableGetBind",
        replacements);
}

std::string MetaVariable::GetDatabaseLoadCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@LOAD_CODE@"] = GetLoadRawCode(generator, name, "stream");

    return generator.ParseTemplate(tabLevel, "VariableDatabaseBlobLoad",
        replacements);
}

std::string MetaVariable::GetInternalGetterCode(const Generator& generator,
    const std::string& name) const
{
    return IsInherited() ? ("Get" + generator.GetCapitalName(*this) +
        "()") : name;
}

std::string MetaVariable::GetSetterCode(const Generator& generator,
    const MetaObject& object, const std::string& name, const std::string argument,
    size_t tabLevel) const
{
    std::string condition = GetValidCondition(generator, argument);

    std::stringstream ss;

    ss << generator.Tab(tabLevel) <<
        "std::lock_guard<std::mutex> lock(mFieldLock);" << std::endl;

    std::string persistentCode = object.IsPersistent() ?
        ("mDirtyFields.insert(\"" + GetName() + "\");") : "";
    if(condition.empty())
    {
        ss << generator.Tab(tabLevel) << name << " = "
            << argument << ";" << std::endl;
        ss << generator.Tab(tabLevel) << persistentCode << std::endl;
        ss << generator.Tab(tabLevel) << "return true;" << std::endl;
    }
    else
    {
        ss << generator.Tab(tabLevel) << "if(" << condition << ")" << std::endl;
        ss << generator.Tab(tabLevel) << "{" << std::endl;
        ss << generator.Tab(tabLevel + 1) << name << " = "
            << argument << ";" << std::endl;
        ss << generator.Tab(tabLevel + 1) << persistentCode << std::endl;
        ss << generator.Tab(tabLevel + 1) << "return true;" << std::endl;
        ss << generator.Tab(tabLevel) << "}" << std::endl;
        ss << std::endl;
        ss << generator.Tab(tabLevel) << "return false;" << std::endl;
    }

    return ss.str();
}

std::string MetaVariable::GetAccessDeclarations(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    std::stringstream ss;
    ss << generator.Tab(tabLevel) << GetCodeType() << " Get"
        << generator.GetCapitalName(*this) << "() const;" << std::endl;
    ss << generator.Tab(tabLevel) << "bool Set"
        << generator.GetCapitalName(*this) << "("
        << GetArgument(name) << ");" << std::endl;

    if(IsLookupKey())
    {
        auto objName = object.GetName();
        std::map<std::string, std::string> replacements;
        replacements["@LOOKUP_TYPE@"] = generator.GetCapitalName(*this);
        replacements["@ARGUMENT@"] = GetArgument(GetName());
        
        if(IsUniqueKey())
        {
            std::stringstream ss2;
            ss2 << "std::shared_ptr<" << objName << ">";

            replacements["@RETURN_TYPE@"] = ss2.str();

            replacements["@RETURN_NAME@"] = objName;
        }
        else
        {
            std::stringstream ss2;
            ss2 << "std::list<std::shared_ptr<" << objName << ">>";

            replacements["@RETURN_TYPE@"] = ss2.str();

            ss2.str("");
            ss2 << objName << "List";

            replacements["@RETURN_NAME@"] = ss2.str();
        }

        ss << generator.ParseTemplate(1, "VariableLookupKeyDeclarations",
            replacements);
    }

    return ss.str();
}

std::string MetaVariable::GetAccessFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    std::stringstream ss;

    auto objName = object.GetName();

    ss << GetCodeType() << " " << objName << "::" << "Get"
        << generator.GetCapitalName(*this) << "() const" << std::endl;
    ss << "{" << std::endl;
    ss << GetGetterCode(generator, name);
    ss << "}" << std::endl;
    ss << std::endl;

    ss << "bool " << objName << "::" << "Set"
        << generator.GetCapitalName(*this) << "(" << GetArgument(GetName())
        << ")" << std::endl;
    ss << "{" << std::endl;
    ss << GetSetterCode(generator, object, name, GetName());
    ss << "}" << std::endl;

    if(IsLookupKey())
    {
        std::map<std::string, std::string> replacements;
        replacements["@OBJECT_NAME@"] = objName;
        replacements["@RETURN_VAR@"] = "retval";
        replacements["@LOOKUP_TYPE@"] = generator.GetCapitalName(*this);
        replacements["@ARGUMENT@"] = GetArgument(GetName());
        replacements["@BINDING@"] = GetBindValueCode(generator, GetName());

        std::map<std::string, std::string> subReplacments;
        subReplacments["@OBJECT_NAME@"] = objName;
        subReplacments["@RETURN_VAR@"] = "retval";

        if(IsUniqueKey())
        {
            std::stringstream ss2;
            ss2 << "std::shared_ptr<" << objName << ">";

            replacements["@RETURN_TYPE@"] = ss2.str();

            replacements["@RETURN_NAME@"] = objName;

            ss2.str("");
            ss2 << generator.ParseTemplate(0, "VariableLookupKeySingleAssignment",
                subReplacments);
            replacements["@ASSIGNMENT_CODE@"] = ss2.str();
        }
        else
        {
            std::stringstream ss2;
            ss2 << "std::list<std::shared_ptr<" << objName << ">>";

            replacements["@RETURN_TYPE@"] = ss2.str();

            ss2.str("");
            ss2 << objName << "List";

            replacements["@RETURN_NAME@"] = ss2.str();

            ss2.str("");
            ss2 << generator.ParseTemplate(1, "VariableLookupKeyListAssignment",
                subReplacments);
            replacements["@ASSIGNMENT_CODE@"] = ss2.str();
        }

        ss << generator.ParseTemplate(0, "VariableLookupKeyFunctions",
            replacements);
    }

    return ss.str();
}

std::string MetaVariable::GetUtilityDeclarations(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    (void)generator;
    (void)name;
    (void)tabLevel;

    return "";
}

std::string MetaVariable::GetUtilityFunctions(const Generator& generator,
    const MetaObject& object, const std::string& name) const
{
    (void)generator;
    (void)object;
    (void)name;

    return "";
}

std::string MetaVariable::GetAccessScriptBindings(const Generator& generator,
    const MetaObject& object, const std::string& name,
    size_t tabLevel) const
{
    (void)name;
    (void)tabLevel;

    std::stringstream ss;

    auto objName = object.GetName();
    ss << ".Func(\"Get" << generator.GetCapitalName(*this) << "\", &"
        << objName << "::Get" << generator.GetCapitalName(*this) << ")"
        << std::endl;

    ss << ".Func(\"Set" << generator.GetCapitalName(*this) << "\", &"
        << objName << "::Set" << generator.GetCapitalName(*this)
        << ")" << std::endl;

    ss << ".Prop(" << "\"" << generator.GetCapitalName(*this) << "\", &"
        << objName << "::Get" << generator.GetCapitalName(*this)
        << ", &" << objName << "::Set" << generator.GetCapitalName(*this)
        << ")" << std::endl;

    if(IsLookupKey())
    {
        std::stringstream f;
        f << "Load" << objName << (IsUniqueKey() ? "" : "List");

        ss << ".StaticFunc(\"" << f.str() << "By"
            << generator.GetCapitalName(*this) << "\", "
            << objName << "::" << f.str() << "By"
            << generator.GetCapitalName(*this) << ")" << std::endl;
    }
    
    return ss.str();
}

std::string MetaVariable::GetConstructorCode(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    (void)object;

    std::string code = GetConstructValue();

    if(code.empty())
    {
        std::stringstream ss;
        ss << generator.Tab(tabLevel) << name << " = " << GetCodeType()
            << "{};" << std::endl;

        code = ss.str();
    }
    else
    {
        std::stringstream ss;
        if(IsInherited())
        {
            ss << generator.Tab(tabLevel) << "Set"
                << generator.GetCapitalName(*this) << "(" << code << ")"
                << ";" << std::endl;
            
        }
        else
        {
            ss << generator.Tab(tabLevel) << name << " = " << code
                << ";" << std::endl;
        }

        code = ss.str();
    }

    return code;
}

std::string MetaVariable::GetDestructorCode(const Generator& generator,
    const MetaObject& object, const std::string& name, size_t tabLevel) const
{
    (void)generator;
    (void)object;
    (void)name;
    (void)tabLevel;

    return std::string();
}

std::string MetaVariable::GetDynamicSizeCountCode(const Generator& generator,
    const std::string& name) const
{
    (void)generator;
    (void)name;

    return std::string();
}

bool MetaVariable::BaseLoad(const tinyxml2::XMLElement& element)
{
    for(const char *a : { "caps", "inherited", "key", "unique" })
    {
        std::string aName(a);
        const char *szAttr = element.Attribute(a);

        if(nullptr != szAttr)
        {
            bool value = Generator::GetXmlAttributeBoolean(szAttr);

            if(aName == "caps")
            {
                SetCaps(value);
            }
            else if(aName == "inherited")
            {
                SetInherited(value);
            }
            else if(aName == "key")
            {
                SetLookupKey(value);
            }
            else if(aName == "unique" && !SetUniqueKey(value))
            {
                return false;
            }
        }
    }

    return true;
}

bool MetaVariable::BaseSave(tinyxml2::XMLElement& element) const
{
    if(IsCaps())
    {
        element.SetAttribute("caps", "true");
    }

    if(IsInherited())
    {
        element.SetAttribute("inherited", "true");
    }

    if(IsLookupKey())
    {
        element.SetAttribute("key", "true");
    }

    if(IsUniqueKey())
    {
        element.SetAttribute("unique", "true");
    }

    return true;
}
