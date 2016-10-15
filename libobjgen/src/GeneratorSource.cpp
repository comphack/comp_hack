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

    std::set<std::string> references = obj.GetReferences();

    if(!references.empty())
    {
        ss << "// Referenced Objects" << std::endl;

        for(auto ref : references)
        {
            ss << "#include <" << ref << ".h>" << std::endl;
        }

        ss << std::endl;
    }

    ss << "using namespace objects;" << std::endl;
    ss << std::endl;

    // Constructor
    ss << obj.GetName() << "::" << obj.GetName() << "() : " + (!obj.GetBaseObject().empty() ? ("objects::" + obj.GetBaseObject() + "()") : "libcomp::Object()")
        << std::endl;
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

    if(references.empty())
    {
        ss << Tab() << "(void)recursive;" << std::endl;
        ss << std::endl;
    }

    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "IsValid(recursive)") + ";" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string validator = var->GetValidCondition(
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

        std::string code = var->GetLoadCode(GetMemberName(var), "stream");

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
    ss << Tab() << "(void)stream;" << std::endl; /// @todo fix
    ss << std::endl;
    ss << Tab() << "bool status = " + GetBaseBooleanReturnValue(obj, "Save(stream)") + "; " << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetSaveCode(GetMemberName(var), "stream");

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
    ss << Tab() << "std::unordered_map<std::string, const "
        "tinyxml2::XMLElement*> members = GetXmlMembers(root);" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        std::string code = var->GetXmlLoadCode(*this, GetMemberName(var),
            "doc", "root", "members");

        if(!code.empty())
        {
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
    ss << Tab() << "tinyxml2::XMLElement& root) const" << std::endl;
    ss << "{" << std::endl;
    ss << Tab() << "bool status = true;" << std::endl;
    ss << std::endl;
    ss << Tab() << "tinyxml2::XMLElement *pElement = doc.NewElement("
        "\"object\");" << std::endl;
    ss << Tab() << "pElement->SetAttribute(\"name\", " << Escape(obj.GetName())
        << ");" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        std::string code = var->GetXmlSaveCode(*this, GetMemberName(var),
            "doc", "(*pElement)");

        if(!code.empty())
        {
            ss << std::endl;
            ss << code;
        }
    }

    ss << std::endl;
    ss << Tab() << "if(status)" << std::endl;
    ss << Tab() << "{" << std::endl;
    ss << Tab(2) << "root.InsertEndChild(pElement);" << std::endl;
    ss << Tab() << "}" << std::endl;
    ss << Tab() << "else" << std::endl;
    ss << Tab() << "{" << std::endl;
    ss << Tab(2) << "doc.DeleteNode(pElement);" << std::endl;
    ss << Tab() << "}" << std::endl;
    ss << std::endl;
    ss << Tab() << "return status;" << std::endl;
    ss << "}" << std::endl;
    ss << std::endl;

    // Accessor Functions
    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        ss << var->GetAccessFunctions(*this, obj, GetMemberName(var));
        ss << std::endl;
    }

    return ss.str();
}

std::string GeneratorSource::GetBaseBooleanReturnValue(const MetaObject& obj, std::string function, std::string defaultValue)
{
    return !obj.GetBaseObject().empty() ? ("objects::" + obj.GetBaseObject() + "::" + function) : defaultValue;
}