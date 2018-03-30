/**
 * @file libcomp/src/DataStore.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage the data store (for static game data).
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#ifndef LIBCOMP_SRC_DATASTORE_H
#define LIBCOMP_SRC_DATASTORE_H

// libcomp Includes
#include "CString.h"

// Standard C++11 Includes
#include <list>

namespace libcomp
{

class DataFile;

class DataStore
{
public:
    DataStore(const char *szProgram);
    ~DataStore();

    libcomp::String GetError();

    std::vector<char> ReadFile(const libcomp::String& path);
    bool WriteFile(const libcomp::String& path,
        const std::vector<char>& data);

    std::vector<char> DecryptFile(const libcomp::String& path);
    bool EncryptFile(const libcomp::String& path,
        const std::vector<char>& data);

    enum class FileMode_t
    {
        READ,
        WRITE,
        APPEND
    };

    DataFile* Open(const libcomp::String& path,
        FileMode_t mode = FileMode_t::READ);

    bool Exists(const libcomp::String& path);
    int64_t FileSize(const libcomp::String& path);

    libcomp::String GetHash(const libcomp::String& path);

    bool Delete(const libcomp::String& path, bool recursive = false);
    bool CreateDirectory(const libcomp::String& path);

    bool GetListing(const libcomp::String& path,
        std::list<libcomp::String>& files,
        std::list<libcomp::String>& dirs,
        std::list<libcomp::String>& symLinks,
        bool recursive = false, bool fullPath = false);
    bool PrintListing(const libcomp::String& path, bool recursive = false,
        bool fullPath = false);
    bool AddSearchPaths(const std::list<libcomp::String>& paths);
    bool AddSearchPath(const libcomp::String& path);
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATASTORE_H
