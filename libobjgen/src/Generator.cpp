/**
 * @file libobjgen/src/Generator.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class for a generator to write source code for an object.
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

#include "Generator.h"

// libobjgen Includes
#include "MetaVariable.h"
#include "ResourceTemplate.h"

// Standard C++11 Includes
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cassert>

using namespace libobjgen;

Generator::Generator()
{
    mVfs.AddArchiveLoader(new ttvfs::VFSZipArchiveLoader);

    ttvfs::CountedPtr<ttvfs::MemFile> pMemoryFile = new ttvfs::MemFile(
        "templates.zip", ResourceTemplate, static_cast<uint32_t>(
        ResourceTemplateSize));

    if(nullptr == mVfs.AddArchive(pMemoryFile, ""))
    {
        assert(false && "Archive did not load!");
    }
}

std::vector<char> Generator::GetTemplate(const std::string& name) const
{
    std::vector<char> data;

    ttvfs::File *vf = mVfs.GetFile(std::string(name + ".cpp").c_str());

    if(!vf)
    {
        return std::vector<char>();
    }

    size_t fileSize = static_cast<size_t>(vf->size());

    data.resize(fileSize);

    if(!vf->open("rb"))
    {
        return std::vector<char>();
    }

    if(fileSize != vf->read(&data[0], fileSize))
    {
        return std::vector<char>();
    }

    return data;
}

std::string Generator::ParseTemplate(size_t tabLevel, const std::string& name,
    const std::map<std::string, std::string>& replacements) const
{
    std::vector<char> templ = GetTemplate(name);

    if(templ.empty())
    {
        return std::string();
    }

    std::string code(templ.begin(), templ.end());

    for(auto pair : replacements)
    {
        size_t pos = 0;

        while((pos = code.find(pair.first, pos)) != std::string::npos)
        {
            code.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }

    std::string indent = Tab(tabLevel);

    if(!indent.empty())
    {
        indent = std::string("\n") + indent;

        size_t pos = 0;

        while((pos = code.find("\n", pos)) != std::string::npos)
        {
            code.replace(pos, 1, indent);
            pos += indent.length();
        }

        code = indent + code;
    }

    return code;
}

std::string Generator::Tab(size_t count) const
{
    return std::string(count * 4, ' ');
}

std::string Generator::GetMemberName(const MetaVariable& var) const
{
    return std::string("m") + GetCapitalName(var);
}

std::string Generator::GetMemberName(
    const std::shared_ptr<MetaVariable>& var) const
{
    std::string s;

    if(var)
    {
        return GetMemberName(*var.get());
    }

    return s;
}

std::string Generator::GetCapitalName(const MetaVariable& var) const
{
    std::string name = var.GetName();

    if(!name.empty())
    {
        if(var.IsCaps())
        {
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
        }
        else
        {
            name[0] = static_cast<char>(::toupper(name[0]));
        }
    }

    return name;
}

std::string Generator::GetCapitalName(
    const std::shared_ptr<MetaVariable>& var) const
{
    std::string s;

    if(var)
    {
        return GetCapitalName(*var.get());
    }

    return s;
}

std::string Generator::Escape(const std::string& str)
{
    std::string s = "\"";

    for(auto c : str)
    {
        if(::isprint(static_cast<uint8_t>(c)))
        {
            switch(c)
            {
                case '"':
                    s += "\\\"";
                    break;
                case '\t':
                    s += "\\t";
                    break;
                case '\\':
                    s += "\\\\";
                    break;
                default:
                    s += std::string(1, c);
                    break;
            }
        }
        else
        {
            switch(c)
            {
                case '\n':
                    s += "\\n";
                    break;
                case '\r':
                    s += "\\r";
                    break;
                default:
                    std::stringstream ss;
                    ss << "\\x" << std::hex << std::setw(2)
                        << std::setfill('0')
                        << (static_cast<uint16_t>(c) & 0xFF);
                    s += ss.str();
                    break;
            }
        }
    }

    s += "\"";

    return s;
}
