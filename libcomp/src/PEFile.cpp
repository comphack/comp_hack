/**
 * @file libcomp/src/PEFile.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Portable Executable (PE) format wrapper class implementation.
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

#include "PEFile.h"

#include <cstdio>
#include <cstring>
#include <iostream>

using namespace libcomp;

/// Macro to access either the 32-bit or 64-bit PE header.
#define FileHdr (mPEHeader64 ? mPEHeader64->FileHeader : \
    mPEHeader32->FileHeader)

/**
 * Macro to access the optional header item 'val' from the appropriate header.
 * @param val Variable in the optional header to access.
 */
#define OptHdr(val) (mOptHeader64 ? mOptHeader64->val : mOptHeader32->val)

PEFile::PEFile(uint8_t *base) : mBase(base)
{
    // Set the DOS header (first item in a PE file).
    mDOSHeader = (IMAGE_DOS_HEADER*)mBase;

    // Ensure the DOS header has the proper magic or signature.
    if(mDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::cerr << "ERROR: Failed to find DOS header." << std::endl;

        return;
    }

    // Assume 32-bit until we can verify that the PE file is 64-bit
    mPEHeader32 = (IMAGE_NT_HEADERS32*)(mBase + mDOSHeader->e_lfanew);

    // Check the size of the header is valid.
    {
        uint16_t size = mPEHeader32->FileHeader.SizeOfOptionalHeader;

        if(size < 224)
        {
            std::cerr << "ERROR: The size of IMAGE_NT_HEADERS "
                "is too small." << std::endl;

            return;
        }

        if(size != 224 && size != 240)
        {
            std::cerr << "Warning: The size of IMAGE_NT_HEADERS "
                "is larger then expected." << std::endl;
        }
    }

    // Determine if the PE file is 32-bit or 64-bit and set the appropriate
    // pointers.
    if(mPEHeader32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        mPEHeader64 = 0;

        mOptHeader32 = (IMAGE_OPTIONAL_HEADER32*)&mPEHeader32->OptionalHeader;
        mOptHeader64 = 0;
    }
    else if(mPEHeader32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        mPEHeader64 = (IMAGE_NT_HEADERS64*)mPEHeader32;
        mPEHeader32 = 0;

        mOptHeader64 = (IMAGE_OPTIONAL_HEADER64*)&mPEHeader64->OptionalHeader;
        mOptHeader32 = 0;
    }
    else
    {
        std::cerr << "ERROR: Invalid optional header magic." << std::endl;

        return;
    }

    // From now on we use mPEHeader32 or mPEHeader64 depending on the
    // file format used for this file (or the macros).

    {
        // Calculate the location of the section headers and set the pointer.
        uint8_t *s = mOptHeader64 ? (uint8_t*)mOptHeader64 :
            (uint8_t*)mOptHeader32;
        uint16_t sz = FileHdr.SizeOfOptionalHeader;

        mSectionHeaders = (IMAGE_SECTION_HEADER*)(s + sz);
    }
}

uint32_t PEFile::offsetToAddress(uint32_t offset, const char *nameReq) const
{
    // Iterate over the sections headers. If nameReq is set, check if the
    // current section matches the required section name. If not, move on to
    // the next section. Calculate the file offset range of the section and
    // compare the offset argument to this range. If the offset is in this
    // range, calculate the address and return it; otherwise, test the next
    // section. If all sections have been checked and there is no match,
    // return an error.
    for(int i = 0; i < FileHdr.NumberOfSections; i++)
    {
        IMAGE_SECTION_HEADER *sec = &mSectionHeaders[i];

        if(nameReq)
        {
            char name[IMAGE_SIZEOF_SHORT_NAME + 1];
            name[IMAGE_SIZEOF_SHORT_NAME] = 0;

            memcpy(name, sec->Name, IMAGE_SIZEOF_SHORT_NAME);
            if(strcmp(nameReq, name) != 0)
                continue;
        }

        uint32_t start = sec->PointerToRawData;
        uint32_t stop = start + sec->SizeOfRawData;

        if(offset < start || offset >= stop)
            continue;

        uint32_t address = (uint32_t)((offset - start) +
            sec->VirtualAddress + OptHdr(ImageBase));

        return address;
    }

    return 0xFFFFFFFF;
}

uint32_t PEFile::addressToOffset(uint32_t address, const char *nameReq) const
{
    // Iterate over the sections headers. If nameReq is set, check if the
    // current section matches the required section name. If not, move on to
    // the next section. Calculate the file offset range of the section and
    // use it to convert the address to a file offset. If the offset is not in
    // the range of section offsets, continue to the next section to check. If
    // the calculated offset is in the range, return it. If all sections have
    // been tested, return an error.
    for(int i = 0; i < FileHdr.NumberOfSections; i++)
    {
        IMAGE_SECTION_HEADER *sec = &mSectionHeaders[i];

        if(nameReq)
        {
            char name[IMAGE_SIZEOF_SHORT_NAME + 1];
            name[IMAGE_SIZEOF_SHORT_NAME] = 0;

            memcpy(name, sec->Name, IMAGE_SIZEOF_SHORT_NAME);
            if(strcmp(nameReq, name) != 0)
                continue;
        }

        uint32_t start = sec->PointerToRawData;
        uint32_t stop = start + sec->SizeOfRawData;

        uint32_t offset = (uint32_t)((address - sec->VirtualAddress -
            OptHdr(ImageBase)) + start);

        if(offset < start || offset >= stop)
            continue;

        return offset;
    }

    return 0xFFFFFFFF;
}

uint32_t PEFile::absoluteToOffset(uint32_t address, const char *nameReq) const
{
    // Iterate over the sections headers. If nameReq is set, check if the
    // current section matches the required section name. If not, move on to
    // the next section. Calculate the offset range of the section and
    // compare the offset argument to this range. If the offset is in this
    // range, return the offset; otherwise, test the next section. If all
    // sections have been checked and there is no match, return an error.
    for(int i = 0; i < FileHdr.NumberOfSections; i++)
    {
        IMAGE_SECTION_HEADER *sec = &mSectionHeaders[i];

        if(nameReq)
        {
            char name[IMAGE_SIZEOF_SHORT_NAME + 1];
            name[IMAGE_SIZEOF_SHORT_NAME] = 0;

            memcpy(name, sec->Name, IMAGE_SIZEOF_SHORT_NAME);
            if(strcmp(nameReq, name) != 0)
                continue;
        }

        uint32_t start = sec->PointerToRawData;
        uint32_t stop = start + sec->SizeOfRawData;

        uint32_t offset = (uint32_t)(address - OptHdr(ImageBase));

        if(offset < start || offset >= stop)
            continue;

        return offset;
    }

    return 0xFFFFFFFF;
}

uint16_t PEFile::sectionCount() const
{
    // Return the number of sections.
    return FileHdr.NumberOfSections;
}

int PEFile::sectionByName(const char *name) const
{
    // Iterate over each section and compare the name of the section to the
    // search name. If the names match (case sensitive), return the section
    // index. If all sections have been searched, return an error.
    for(int i = 0; i < FileHdr.NumberOfSections; i++)
    {
        if(strcmp(name, mSectionHeaders[i].Name) == 0)
            return i;
    }

    return -1;
}

IMAGE_SECTION_HEADER* PEFile::section(int i) const
{
    // Return the requested section or null if the index is out of bounds.
    if(i < 0 || i >= sectionCount())
        return 0;

    return &mSectionHeaders[i];
}

IMAGE_DOS_HEADER* PEFile::dosHeader() const
{
    // Return the DOS header.
    return mDOSHeader;
}

IMAGE_NT_HEADERS32* PEFile::peHeader32() const
{
    // Return the 32-bit PE header.
    return mPEHeader32;
}

IMAGE_NT_HEADERS64* PEFile::peHeader64() const
{
    // Return the 64-bit PE header.
    return mPEHeader64;
}

IMAGE_OPTIONAL_HEADER32* PEFile::optHeader32() const
{
    // Return the 32-bit optional header.
    return mOptHeader32;
}

IMAGE_OPTIONAL_HEADER64* PEFile::optHeader64() const
{
    // Return the 64-bit optional header.
    return mOptHeader64;
}
