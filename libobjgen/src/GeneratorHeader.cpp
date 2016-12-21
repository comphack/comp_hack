/**
 * @file libobjgen/src/GeneratorHeader.cpp
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

#include "GeneratorHeader.h"

// libobjgen Includes
#include "MetaObject.h"
#include "MetaVariable.h"
#include "MetaVariableEnum.h"

// Standard C++11 Includes
#include <algorithm>
#include <sstream>

using namespace libobjgen;

std::string GeneratorHeader::GenerateClass(const MetaObject& obj)
{
    std::stringstream ss;
    ss << "class " << obj.GetName() << " : public ";
    if(!obj.GetBaseObject().empty())
    {
        ss << "objects::" + obj.GetBaseObject() << std::endl;
    }
    else if(obj.GetPersistent())
    {
        ss << "libcomp::PersistentObject" << std::endl;
    }
    else
    {
        ss << "libcomp::Object" << std::endl;
    }
    ss << "{" << std::endl;
    ss << "public:" << std::endl;
    
    // Print defitions for any enums we have
    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        if(var->GetMetaType() == MetaVariable::MetaVariableType_t::TYPE_ENUM)
        {
            ss << Tab() << "enum class " << var->GetName() << "_t" << std::endl;
            ss << Tab() << "{" << std::endl;
            auto values = std::dynamic_pointer_cast<MetaVariableEnum>(var)->GetValues();
            for(auto value : values)
            {
                ss << Tab(2) << value << "," << std::endl;
            }
            ss << Tab() << "};" << std::endl << std::endl;
        }
    }

    ss << Tab() << obj.GetName() << "();" << std::endl;
    ss << Tab() << "virtual ~" << obj.GetName() << "();" << std::endl;
    ss << std::endl;

    ss << Tab() << "virtual bool IsValid(bool recursive = true) const;"
        << std::endl;
    ss << std::endl;

    ss << Tab() << "virtual bool Load(libcomp::ObjectInStream& stream);"
        << std::endl << std::endl;
    ss << Tab() << "virtual bool Save(libcomp::ObjectOutStream& stream) const;"
        << std::endl << std::endl;

    ss << Tab() << "virtual bool Load(std::istream& stream, bool flat = false);"
        << std::endl << std::endl;
    ss << Tab() << "virtual bool Save(std::ostream& stream, bool flat = false) const;"
        << std::endl << std::endl;

    ss << Tab() << "virtual bool Load("
        "const tinyxml2::XMLDocument& doc, " << std::endl;
    ss << Tab(2) << "const tinyxml2::XMLElement& root);"
        << std::endl << std::endl;
    ss << Tab() << "virtual bool Save("
        "tinyxml2::XMLDocument& doc, " << std::endl;
    ss << Tab(2) << "tinyxml2::XMLElement& root) const;"
        << std::endl << std::endl;

    ss << Tab() << "virtual uint16_t GetDynamicSizeCount() const;" << std::endl;
    ss << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        ss << var->GetAccessDeclarations(*this, obj, var->GetName());
        ss << std::endl;
    }

    if(obj.GetPersistent())
    {
        std::map<std::string, std::string> replacements;
        ss << ParseTemplate(1, "VariablePersistentDeclarations", replacements);
    }

    std::stringstream utilStream;
    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        auto util = var->GetUtilityDeclarations(*this, var->GetName());
        if(util.length() > 0)
        {
            utilStream << util;
        }
    }

    std::string utilDeclarations = utilStream.str();
    if(utilDeclarations.length() > 0)
    {
        ss << "protected:" << utilDeclarations << std::endl;
    }

    ss << "private:" << std::endl;

    for(auto it = obj.VariablesBegin(); it != obj.VariablesEnd(); ++it)
    {
        auto var = *it;

        if(var->IsInherited()) continue;

        ss << Tab() << var->GetDeclaration(GetMemberName(var)) << std::endl;
    }

    ss << "};" << std::endl;

    return ss.str();
}

std::string GeneratorHeader::GenerateHeaderDefine(const std::string& objName)
{
    std::stringstream ss;
    ss << "OBJGEN_OBJECT_" << objName << "_H";

    std::string headerDefine = ss.str();
    std::transform(headerDefine.begin(), headerDefine.end(),
        headerDefine.begin(), ::toupper);

    return headerDefine;
}

std::string GeneratorHeader::Generate(const MetaObject& obj)
{
    std::stringstream ss;
    ss << "// THIS FILE IS GENERATED" << std::endl;
    ss << "// DO NOT MODIFY THE CONTENTS" << std::endl;
    ss << "// DO NOT COMMIT TO VERSION CONTROL" << std::endl;
    ss << std::endl;

    std::string headerDefine = GenerateHeaderDefine(obj.GetName());

    ss << "#ifndef " << headerDefine << std::endl;
    ss << "#define " << headerDefine << std::endl;
    ss << std::endl;

    ss << "// libcomp Includes" << std::endl;
    ss << "#include <Convert.h>" << std::endl;
    ss << "#include <CString.h>" << std::endl;

    std::set<std::string> references = obj.GetReferencesTypes();
    if(references.size() > 0)
    {
        ss << "#include <ObjectReference.h>" << std::endl;
    }

    if(!obj.GetBaseObject().empty())
    {
        ss << "#include <" + obj.GetBaseObject() + ".h>" << std::endl;
    }
    else if(obj.GetPersistent())
    {
        ss << "#include <PersistentObject.h>" << std::endl;

        ss << std::endl;
        ss << "// libobjgen Includes" << std::endl;
        ss << "#include <MetaObject.h>" << std::endl;
    }
    else
    {
        ss << "#include <Object.h>" << std::endl;
    }
    ss << std::endl;

    ss << "// Standard C++11 Includes" << std::endl;
    ss << "#include <array>" << std::endl;
    ss << std::endl;

    ss << "// tinyxml2 Includes" << std::endl;
    ss << "#include <PushIgnore.h>" << std::endl;
    ss << "#include <tinyxml2.h>" << std::endl;
    ss << "#include <PopIgnore.h>" << std::endl;
    ss << std::endl;

    if(!references.empty())
    {
        ss << "namespace objects" << std::endl;
        ss << "{" << std::endl;
        ss << std::endl;

        ss << "// Forward Declare the Object" << std::endl;
        ss << "class " << obj.GetName() << ";" << std::endl;
        ss << std::endl;

        ss << "// Referenced Objects" << std::endl;

        for(auto ref : references)
        {
            ss << "class " << ref << ";" << std::endl;
        }

        ss << std::endl;
        ss << "} // namespace objects" << std::endl;
        ss << std::endl;
    }

    ss << "namespace objects" << std::endl;
    ss << "{" << std::endl;
    ss << std::endl;

    ss << GenerateClass(obj) << std::endl;

    ss << "} // namespace objects" << std::endl;
    ss << std::endl;

    ss << "#endif // " << headerDefine << std::endl;

    return ss.str();
}
