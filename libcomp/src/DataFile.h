/**
 * @file libcomp/src/DataFile.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage a data store file.
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

#ifndef LIBCOMP_SRC_DATAFILE_H
#define LIBCOMP_SRC_DATAFILE_H

// Standard C++11 Includes
#include <stdint.h>

// libcomp Includes
#include "DataStore.h"

struct PHYSFS_File;

namespace libcomp
{

class DataFile
{
    friend class DataStore;

public:
    bool IsOpen() const;

    bool Open(DataStore::FileMode_t mode = DataStore::FileMode_t::READ);
    bool Close();
    bool Flush();

    libcomp::String GetPath() const;
    void SetPath(const libcomp::String& path);

    int64_t GetSize() const;
    bool AtEOF() const;

    enum class Whence_t
    {
        BEGIN,
        CURRENT,
        END
    };

    bool Read(void *pBuffer, uint32_t bufferSize);
    std::vector<char> Read(uint32_t size);

    bool Write(const void *pBuffer, uint32_t bufferSize);
    bool Write(const std::vector<char>& buffer);

    int64_t GetPosition() const;
    bool SetPosition(int64_t pos, Whence_t whence = Whence_t::BEGIN);

protected:
    DataFile(const libcomp::String& path, DataStore::FileMode_t mode);
    ~DataFile();

private:
    libcomp::String mPath;
    PHYSFS_File *mFile;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATAFILE_H
