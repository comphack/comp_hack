/**
 * @file libobjgen/src/Generator.h
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

#ifndef LIBOBJGEN_SRC_GENERATOR_H
#define LIBOBJGEN_SRC_GENERATOR_H

// Standard C++11 Includes
#include <memory>
#include <string>

// VFS Includes
#include "PushIgnore.h"
#include <ttvfs/ttvfs.h>
#include <ttvfs/ttvfs_zip.h>
#include "PopIgnore.h"

namespace libobjgen
{

class MetaObject;
class MetaVariable;

class Generator
{
public:
    Generator();

    virtual std::string Generate(const MetaObject& obj) = 0;

    std::vector<char> GetTemplate(const std::string& name) const;

    std::string ParseTemplate(size_t tabLevel, const std::string& name,
        const std::map<std::string, std::string>& replacements) const;

    static std::string Tab(size_t count = 1);

    virtual std::string GetMemberName(
        const MetaVariable& var) const;
    virtual std::string GetMemberName(
        const std::shared_ptr<MetaVariable>& var) const;

    static std::string GetCapitalName(
        const MetaVariable& var);
    static std::string GetCapitalName(
        const std::shared_ptr<MetaVariable>& var);

    static std::string GetObjectName(
        const std::string& fullName);
    static std::string GetObjectName(
        const std::string& fullName, std::string& outNamespace);

    static bool GetXmlAttributeBoolean(const std::string& attr);

    static std::string GetPersistentRefCopyCode(
        const std::shared_ptr<MetaVariable>& var, const std::string& name);

    static std::string Escape(const std::string& str);

    static bool LoadString(std::istream& stream, std::string& s);
    static bool SaveString(std::ostream& stream, const std::string& s);

private:
    mutable ttvfs::Root mVfs;
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_GENERATOR_H
