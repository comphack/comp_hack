/**
 * @file libobjgen/src/GeneratorSource.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief C++ source generator to write source code for an object.
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

#include "GeneratorSource.h"

// libobjgen Includes
#include "MetaObject.h"
#include "MetaVariable.h"
#include "MetaVariableEnum.h"
#include "MetaVariableReference.h"

// Standard C++ Includes
#include <sstream>

using namespace libobjgen;

std::string GeneratorSource::Generate(const MetaObject& obj)
{
    std::stringstream ss;
    ss << "// THIS FILE IS GENERATED" << std::endl;
    ss << "// DO NOT MODIFY THE CONTENTS" << std::endl;
    ss << "// DO NOT COMMIT TO VERSION CONTROL" << std::endl;
    ss << std::endl;

    ss << "#include \"" << obj.GetName() << ".h\"" << std::endl;
    ss << std::endl;

    ss << "// libcomp Includes" << std::endl;
    if(obj.IsPersistent())
    {
        ss << "#include <Database.h>" << std::endl;
    }
    ss << "#include <DatabaseBind.h>" << std::endl;
    ss << "#include <DatabaseQuery.h>" << std::endl;
    ss << "#include <Log.h>" << std::endl;
    ss << "#include <VectorStream.h>" << std::endl;

    bool scriptEnabled = obj.IsScriptEnabled();
    if(scriptEnabled)
    {
        ss << "#include <ScriptEngine.h>" << std::endl;
    }
    ss << std::endl;

    std::list<std::shared_ptr<MetaVariableReference>> references;
    for(auto var : obj.GetReferences())
    {
        references.push_back(std::dynamic_pointer_cast<
            MetaVariableReference>(var));
    }

    if(!references.empty())
    {
        ss << "// Referenced Objects" << std::endl;

        for(auto ref : references)
        {
            ss << "#include <" << Generator::GetObjectName(
                ref->GetReferenceType(true)) << ".h>" << std::endl;
        }

        ss << std::endl;
    }

    std::list<std::string> inheritedObjects;

    if(obj.IsInheritedConstruction())
    {
        std::list<std::shared_ptr<MetaObject>> objs;

        obj.GetAllInheritedObjects(objs);

        for(auto o : objs)
        {
            inheritedObjects.push_back(o->GetName());
        }
    }

    if(!inheritedObjects.empty())
    {
        ss << "// Inherited Objects" << std::endl;

        for(auto objName : inheritedObjects)
        {
            ss << "#include <" << objName << ".h>" << std::endl;
        }

        ss << std::endl;
    }

    inheritedObjects.push_front(obj.GetName());

    ss << "using namespace " << obj.GetNamespace() << ";" << std::endl;
    ss << std::endl;

    // Constructor
    ss << obj.GetName() << "::" << obj.GetName() << "() : ";

    auto baseObject = obj.GetBaseObject();
    if(!baseObject.empty())
    {
        ss << baseObject + "()" << std::endl;
    }
    else if(obj.IsPersistent())
    {
        ss << "libcomp::PersistentObject()" << std::endl;
    }
    else
    {
        ss << "libcomp::Object()" << std::endl;
    }
    ss << "{" << std::endl;

    int constructorCount = 0;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        std::string constructor = var->GetConstructorCode(*this,
            obj, GetMemberName(var));

        if(!constructor.empty())
        {
            // Add space between constructor code chunks.
            if(0 < constructorCount)
            {
                ss << std::endl;
            }

            ss << constructor;

            constructorCount++;
        }
    }

    ss << "}" << std::endl;
    ss << std::endl;

    // Destructor
    ss << obj.GetName() << "::~" << obj.GetName() << "()" << std::endl;
    ss << "{" << std::endl;

    int destructorCount = 0;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string destructor = var->GetDestructorCode(*this,
            obj, GetMemberName(var));

        if(!destructor.empty())
        {
            // Add space between destructor code chunks.
            if(0 < destructorCount)
            {
                ss << std::endl;
            }

            ss << destructor;

            destructorCount++;
        }
    }

    ss << "}" << std::endl;
    ss << std::endl;

    // IsValid
    ss << "bool " << obj.GetName() << "::IsValid(bool recursive) const"
        << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "(void)recursive;" << std::endl;
    ss << std::endl;

    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "IsValid(recursive)") + ";" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string validator = var->GetValidCondition(*this,
            GetMemberName(var), true);

        if(!validator.empty())
        {
            ss << std::endl;
            ss << Tab() << "if(status && !(" << validator << "))" << std::endl;
            ss << Tab() << "{" << std::endl;
            ss << Tab(2) << "status = false;" << std::endl;
            ss << Tab() << "}" << std::endl;
        }
    }

    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    // Load (binary)
    ss << "bool " << obj.GetName()
        << "::Load(libcomp::ObjectInStream& stream)" << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "Load(stream)") + ";" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetLoadCode(*this, GetMemberName(var),
            "stream");

        if(!code.empty())
        {
            ss << std::endl;
            ss << Tab() << "if(status && !(" << code << "))" << std::endl;
            ss << Tab() << "{" << std::endl;
            ss << Tab(2) << "status = false;" << std::endl;
            ss << Tab() << "}" << std::endl;
        }
    }

    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    // Save (binary)
    ss << "bool " << obj.GetName()
        << "::Save(libcomp::ObjectOutStream& stream) const" << std::endl;
    ss << "{" << std::endl;
    ss << std::endl;
    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "Save(stream)") + "; " << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetSaveCode(*this, GetMemberName(var),
            "stream");

        if(!code.empty())
        {
            ss << std::endl;
            ss << Tab() << "if(status && !(" << code << "))" << std::endl;
            ss << Tab() << "{" << std::endl;
            ss << Tab(2) << "status = false;" << std::endl;
            ss << Tab() << "}" << std::endl;
        }
    }

    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    // Load (raw binary)
    ss << "bool " << obj.GetName()
        << "::Load(std::istream& stream, bool flat)" << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "(void)flat;" << std::endl;
    ss << std::endl;
    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "Load(stream, flat)") + ";" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetLoadRawCode(*this, GetMemberName(var),
            "stream");

        if(!code.empty())
        {
            ss << std::endl;
            ss << Tab() << "if(status && !(" << code << "))" << std::endl;
            ss << Tab() << "{" << std::endl;
            ss << Tab(2) << "status = false;" << std::endl;
            ss << Tab() << "}" << std::endl;
        }
    }

    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    // Save (raw binary)
    ss << "bool " << obj.GetName()
        << "::Save(std::ostream& stream, bool flat) const" << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "(void)flat;" << std::endl;
    ss << std::endl;
    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "Save(stream, flat)") + "; " << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetSaveRawCode(*this, GetMemberName(var),
            "stream");

        if(!code.empty())
        {
            ss << std::endl;
            ss << Tab() << "if(status && !(" << code << "))" << std::endl;
            ss << Tab() << "{" << std::endl;
            ss << Tab(2) << "status = false;" << std::endl;
            ss << Tab() << "}" << std::endl;
        }
    }

    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    // Load (XML)
    ss << "bool " << obj.GetName()
        << "::Load(const tinyxml2::XMLDocument& doc, " << std::endl;
    ss << Tab() << "const tinyxml2::XMLElement& root)" << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "(void)doc;" << std::endl;
    ss << std::endl;
    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "Load(doc, root)") + ";" << std::endl;
    ss << std::endl;
    ss << Tab() << "auto members = GetXmlMembers(root);" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetXmlLoadCode(*this, GetMemberName(var),
            "doc", "pMember");

        if(!code.empty())
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = Escape(var->GetName());
            replacements["@VAR_CAMELCASE_NAME@"] = GetCapitalName(*var);
            replacements["@ACCESS_CODE@"] = code;
            replacements["@NODE@"] = "pMember";

            code = ParseTemplate(1, "VariableMemberXmlLoad", replacements);

            ss << std::endl;
            ss << code;
        }
    }

    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    // Save (XML)
    ss << "bool " << obj.GetName()
        << "::Save(tinyxml2::XMLDocument& doc, " << std::endl;
    ss << Tab() << "tinyxml2::XMLElement& root, bool append) const" << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "bool status = true;" << std::endl;
    ss << std::endl;
    ss << Tab() << "tinyxml2::XMLElement *pElement = nullptr;" << std::endl;
    ss << Tab() << "if(append)" << std::endl;
    ss << Tab() << "{" << std::endl;
    ss << Tab(2) << "pElement = &root;" << std::endl;
    ss << Tab() << "}" << std::endl;
    ss << Tab() << "else" << std::endl;
    ss << Tab() << "{" << std::endl;
    ss << Tab(2) << "pElement = doc.NewElement(\"object\");" << std::endl;
    ss << Tab(2) << "pElement->SetAttribute(\"name\", " << Escape(obj.GetName())
        << ");" << std::endl;
    ss << Tab() << "}" << std::endl;

    // Save the base object fields first
    if(!baseObject.empty())
    {
        ss << std::endl << Tab() << "status &= " << baseObject << "::Save(doc, *pElement, true);"
            << std::endl;
    }

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetXmlSaveCode(*this, GetMemberName(var),
            "doc", "pElement");

        if(!code.empty())
        {
            ss << code;
            ss << std::endl;
        }
    }

    ss << std::endl;
    ss << Tab() << "if(status)" << std::endl;
    ss << Tab() << "{" << std::endl;
    ss << Tab(2) << "if(!append)" << std::endl;
    ss << Tab(2) << "{" << std::endl;
    ss << Tab(3) << "root.InsertEndChild(pElement);" << std::endl;
    ss << Tab(2) << "}" << std::endl;
    ss << Tab() << "}" << std::endl;
    ss << Tab() << "else" << std::endl;
    ss << Tab() << "{" << std::endl;
    ss << Tab(2) << "doc.DeleteNode(pElement);" << std::endl;
    ss << Tab() << "}" << std::endl;
    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    ss << "uint16_t " << obj.GetName()
        << "::GetDynamicSizeCount() const" << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "return " << obj.GetDynamicSizeCount() << ";" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    ss << "std::shared_ptr<" << obj.GetName() << "> " << obj.GetName()
        << "::InheritedConstruction(const libcomp::String& name)" << std::endl;
    ss << "{" << std::endl;

    for(auto objName : inheritedObjects)
    {
        ss << std::endl;
        ss << Tab() << "if(" << Escape(objName) << " == name)" << std::endl;
        ss << Tab() << "{" << std::endl;
        ss << Tab(2) << "return std::make_shared<" << objName
            << ">();" << std::endl;
        ss << Tab() << "}" << std::endl;
        ss << std::endl;
    }

    ss << Tab() << "return {};" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    std::stringstream scriptBindings;

    // Accessor Functions
    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        ss << var->GetAccessFunctions(*this, obj, GetMemberName(var));
        ss << std::endl;

        auto util = var->GetUtilityFunctions(*this, obj, GetMemberName(var));
        if(util.length() > 0)
        {
            ss << util << std::endl;
        }

        if(scriptEnabled && var->IsScriptAccessible())
        {
            auto binding = var->GetAccessScriptBindings(*this, obj,
                GetMemberName(var));
            if(binding.length() > 0)
            {
                scriptBindings << binding;
            }
        }
    }

    if(scriptEnabled)
    {
        // The script bindings will not cover things such as lists of script
        // enabled objects, so load these separately
        std::set<std::string> scriptReferences;
        for(auto ref : references)
        {
            if(ref->IsScriptAccessible())
            {
                scriptReferences.insert(ref->GetReferenceType());
            }
        }

        scriptReferences.erase(obj.GetName());

        std::stringstream bindingType;
        std::stringstream dependencies;
        if(!baseObject.empty())
        {
            bindingType << "DerivedClass<" << obj.GetName() << ", "
                << baseObject << ">";
            dependencies << "Using<" << baseObject << ">();" << std::endl;
        }
        else
        {
            bindingType << "Class<" << obj.GetName() << ">";
        }

        if(scriptReferences.size() > 0)
        {
            dependencies << "// Include references" << std::endl;
            for(auto ref : scriptReferences)
            {
                if(ref != obj.GetName())
                {
                    dependencies << "Using<" << ref << ">();" << std::endl;
                }
            }
        }

        std::string bindingsStr = scriptBindings.str();
        if(!bindingsStr.empty())
        {
            bindingsStr = "binding" + bindingsStr + ";";
        }

        std::map<std::string, std::string> replacements;
        replacements["@BINDING_TYPE@"] = bindingType.str();
        replacements["@OBJECT_NAME@"] = obj.GetName();
        replacements["@OBJECT_STRING_NAME@"] = Escape(obj.GetName());
        replacements["@BINDINGS@"] = bindingsStr;
        replacements["@DEPENDENCIES@"] = dependencies.str();

        std::stringstream additions;

        // Register enums with the constants table
        for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
        {
            auto var = *it;

            if(var->GetMetaType() == MetaVariable::MetaVariableType_t::TYPE_ENUM)
            {
                auto eVar = std::dynamic_pointer_cast<MetaVariableEnum>(var);

                additions << "{" << std::endl
                    << Tab() << "Sqrat::Enumeration e(mVM);" << std::endl;
                for(auto enumValPair : eVar->GetValues())
                {
                    additions << Tab() << "e.Const(" << Escape(enumValPair.first)
                        << ", (" << eVar->GetUnderlyingType() << ")"
                        << eVar->GetCodeType() << "::" << enumValPair.first
                        << ");" << std::endl;
                }

                additions << std::endl << Tab() << "Sqrat::ConstTable(mVM).Enum(\""
                    << obj.GetName() << "_" << eVar->GetName() << "_t\", e);"
                    << std::endl << "}" << std::endl << std::endl;
            }
        }

        if(obj.IsPersistent())
        {
            // Need to generate ObjectReference binding
            additions << ParseTemplate(0,
                "VariablePersistentReferenceScriptBinding", replacements);
        }

        replacements["@ADDITIONS@"] = additions.str();

        ss << ParseTemplate(0, "VariableAccessScriptBindings", replacements)
            << std::endl;
    }

    if(obj.IsPersistent() && !GeneratePersistentObjectFunctions(obj, ss))
    {
        return "";
    }

    return ss.str();
}

bool GeneratorSource::GeneratePersistentObjectFunctions(const MetaObject& obj,
    std::stringstream& ss)
{
    std::stringstream binds;
    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        //Only return fields to save if the record is new or the field was updated
        binds << Tab() << "if(retrieveAll || mDirtyFields.find(\"" << var->GetName() <<
            "\") != mDirtyFields.end())" << std::endl;
        binds << Tab() << "{" << std::endl;
        binds << Tab(1) << "values.push_back((" << var->GetBindValueCode(
            *this, GetMemberName(var)) << ")());" << std::endl;
        binds << Tab() << "}" << std::endl;
        binds << std::endl;
    }

    std::stringstream dbValues;
    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        dbValues << Tab() << "if(!" << var->GetDatabaseLoadCode(
            *this, GetMemberName(var)) << ")" << std::endl;
        dbValues << Tab() << "{" << std::endl;
        dbValues << Tab(2) << "return false;" << std::endl;
        dbValues << Tab() << "}" << std::endl;
        dbValues << std::endl;
    }

    std::map<std::string, std::string> replacements;
    replacements["@OBJECT_NAME@"] = obj.GetName();
    replacements["@BINDS@"] = binds.str();
    replacements["@GET_DATABASE_VALUES@"] = dbValues.str();

    std::stringstream savedBytes;
    if(!obj.Save(savedBytes))
    {
        return false;
    }

    std::string byteStr(savedBytes.str());
    replacements["@BYTE_COUNT@"] = std::to_string(byteStr.length());

    std::stringstream outBytes;
    for(size_t i = 0; i < byteStr.length(); i++)
    {
        if(i > 0)
        {
            outBytes << ", ";
            if(i % 10 == 0)
            {
                outBytes << std::endl;
            }
        }

        outBytes << std::to_string(byteStr[i]);
    }

    replacements["@BYTES@"] = outBytes.str();

    ss << ParseTemplate(0, "VariablePersistentFunctions", replacements);

    return true;
}

std::string GeneratorSource::GetBaseBooleanReturnValue(const MetaObject& obj,
    std::string function, std::string defaultValue)
{
    std::string baseObject = obj.GetBaseObject();
    return !baseObject.empty() ? (baseObject + "::" + function) : defaultValue;
}