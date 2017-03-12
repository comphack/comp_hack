/**
 * @file libcomp/src/DataFile.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage a data store file.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

// libcomp Includes
#include "DataFile.h"
#include "Log.h"

// PhysFS Includes
#include <physfs.h>

using namespace libcomp;

DataFile::DataFile(const libcomp::String& path, DataStore::FileMode_t mode) :
    mFile(nullptr)
{
    SetPath(path);

    (void)Open(mode);
}

DataFile::~DataFile()
{
    (void)Close();
}

bool DataFile::IsOpen() const
{
    return nullptr != mFile;
}

bool DataFile::Open(DataStore::FileMode_t mode)
{
    if(IsOpen())
    {
        return false;
    }

    switch(mode)
    {
        case DataStore::FileMode_t::READ:
            mFile = PHYSFS_openRead(mPath.C());
            break;
        case DataStore::FileMode_t::WRITE:
            mFile = PHYSFS_openWrite(mPath.C());
            break;
        case DataStore::FileMode_t::APPEND:
            mFile = PHYSFS_openAppend(mPath.C());
            break;
        default:
            mFile = nullptr;
            break;
    }

    return IsOpen();
}

bool DataFile::Close()
{
    if(nullptr == mFile)
    {
        return false;
    }

    if(0 == PHYSFS_close(mFile))
    {
        return false;
    }

    mFile = nullptr;

    return true;
}

bool DataFile::Flush()
{
    if(nullptr == mFile)
    {
        return false;
    }

    return 0 != PHYSFS_flush(mFile);
}

libcomp::String DataFile::GetPath() const
{
    return mPath;
}

void DataFile::SetPath(const libcomp::String& path)
{
    mPath = path;
}

int64_t DataFile::GetSize() const
{
    if(nullptr == mFile)
    {
        return -1;
    }

    return PHYSFS_fileLength(mFile);
}

bool DataFile::AtEOF() const
{
    if(nullptr == mFile)
    {
        return true;
    }

    return 0 != PHYSFS_eof(mFile);
}

int64_t DataFile::GetPosition() const
{
    if(nullptr == mFile)
    {
        return -1;
    }

    return PHYSFS_tell(mFile);
}

bool DataFile::SetPosition(int64_t pos, Whence_t whence)
{
    if(nullptr == mFile)
    {
        return false;
    }

    switch(whence)
    {
        case Whence_t::BEGIN:
            break;
        case Whence_t::CURRENT:
            pos += GetPosition();
            break;
        case Whence_t::END:
            pos += GetSize();
            break;
        default:
            return false;
    }

    if(0 > pos || pos > GetSize())
    {
        return false;
    }

    return 0 != PHYSFS_seek(mFile, static_cast<uint64_t>(pos));
}

bool DataFile::Read(void *pBuffer, uint32_t bufferSize)
{
    if(nullptr == mFile)
    {
        return false;
    }

    return 1 == PHYSFS_read(mFile, pBuffer, bufferSize, 1);
}

std::vector<char> DataFile::Read(uint32_t size)
{
    std::vector<char> buffer;
    buffer.resize(size);

    if(static_cast<uint32_t>(buffer.size()) == size && Read(&buffer[0], size))
    {
        return buffer;
    }

    return {};
}

bool DataFile::Write(const void *pBuffer, uint32_t bufferSize)
{
    if(nullptr == mFile)
    {
        return false;
    }

    return 1 == PHYSFS_write(mFile, pBuffer, bufferSize, 1);
}

bool DataFile::Write(const std::vector<char>& buffer)
{
    return Write(&buffer[0], static_cast<uint32_t>(buffer.size()));
}
