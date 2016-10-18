/**
 * @file libcomp/src/PEFile.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Portable Executable (PE) format wrapper class implementation.
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

#ifndef LIBCOMP_SRC_PEFILE_H
#define LIBCOMP_SRC_PEFILE_H

#include "PEFormat.h"

namespace libcomp
{

/**
 * Wrapper class to aid in parsing a PE file. The only part of the Windows
 * Portable Executable format that we care about is the section information.
 * This class provides a way to determine where in the file different sections
 * of the executable are stored and what the virtual address of that data will
 * be when the executable is loaded into memory.
 */
class PEFile
{
public:
    /**
     * Wrap an PE executable. Base is a pointer to the executable loaded into
     * memory. Note that the pointer is not managed by this clas. The pointer
     * must remain valid as long as you use this object and must be freed by
     * hand. Deleting this object will not free the executable passed to it.
     * @param base Pointer to the PE executable data.
     */
    PEFile(uint8_t *base);

    /**
     * Convert a file offset to the virtual address of the data when loaded
     * into memory. If a section name is specified, the address will only be
     * returned if the offset is in the desired section.
     * @param offset Byte offset in the file.
     * @param nameReq Section to search for the offset in.
     * @returns Virtual address of data when loaded into memory or 0xFFFFFFFF
     * if the offset is invalid, not mapped, or not in the requested section.
     */
    uint32_t offsetToAddress(uint32_t offset, const char *nameReq = 0) const;

    /**
     * Convert a virtual address to the file offset of the data. If a section
     * name is specified, the file offset will only be returned if the virtual
     * address is in the desired section.
     * @param address Virtual address of the data.
     * @param nameReq Section to search for the virtual address in.
     * @returns File offset of the requested virtual address or 0xFFFFFFFF
     * if the virtual address is invalid or not in the requested section.
     */
    uint32_t addressToOffset(uint32_t address, const char *nameReq = 0) const;

    /**
     * Convert the absolute address to an address without the image base.
     * @param address Virtual addresss to subtract the image base from.
     * @param nameReq Section to search for the virtual address in.
     * @returns Converted address or 0xFFFFFFFF if the virtual address is
     * invalid or not in the requested section.
     */
    uint32_t absoluteToOffset(uint32_t address, const char *nameReq = 0) const;

    /**
     * Number of sections defined in the PE file.
     * @returns Number of sections.
     */
    uint16_t sectionCount() const;

    /**
     * Determine the index in the section array of the section with the
     * specified name.
     * @param name Name of the section to find.
     * @returns Index of the section or -1 if the section was not found.
     */
    int sectionByName(const char *name) const;

    /**
     * Access the raw DOS header of the PE file.
     * @returns Structure describing the DOS header.
     */
    IMAGE_DOS_HEADER* dosHeader() const;

    /**
     * Return the 32-bit NT headers of the PE file.
     * @returns Structure describing the 32-bit NT headers or null if the
     * headers are 64-bit.
     */
    IMAGE_NT_HEADERS32* peHeader32() const;

    /**
     * Return the 64-bit NT headers of the PE file.
     * @returns Structure describing the 64-bit NT headers or null if the
     * headers are 32-bit.
     */
    IMAGE_NT_HEADERS64* peHeader64() const;

    /**
     * Return the 32-bit optional headers of the PE file.
     * @returns Structure describing the 32-bit optional headers or null if the
     * headers are 64-bit.
     */
    IMAGE_OPTIONAL_HEADER32* optHeader32() const;

    /**
     * Return the 64-bit optional headers of the PE file.
     * @returns Structure describing the 64-bit optional headers or null if the
     * headers are 32-bit.
     */
    IMAGE_OPTIONAL_HEADER64* optHeader64() const;

    /**
     * Return the section header of the requested section.
     * @param i Index of the section.
     * @returns Structure describing the section or null if the section at the
     * specified index does not exist.
     */
    IMAGE_SECTION_HEADER* section(int i) const;

protected:
    /// Pointer to the 32-bit PE header data.
    IMAGE_NT_HEADERS32 *mPEHeader32;

    /// Pointer to the 64-bit PE header data.
    IMAGE_NT_HEADERS64 *mPEHeader64;

    /// Pointer to the 32-bit optional header data.
    IMAGE_OPTIONAL_HEADER32 *mOptHeader32;

    /// Pointer to the 64-bit optional header data.
    IMAGE_OPTIONAL_HEADER64 *mOptHeader64;

    /// Pointer to the section headers.
    IMAGE_SECTION_HEADER *mSectionHeaders;

    /// Pointer to DOS header.
    IMAGE_DOS_HEADER *mDOSHeader;

    /// Pointer to the PE file.
    uint8_t *mBase;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_PEFILE_H
