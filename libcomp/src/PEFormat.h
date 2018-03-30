/**
 * @file libcomp/src/PEFormat.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Portable Executable (PE) file format definitions.
 * There is no point commenting anything in this file. If you really want to
 * read about the PE file format, google it.
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

#ifndef LIBCOMP_SRC_PEFORMAT_H
#define LIBCOMP_SRC_PEFORMAT_H

#include <stdint.h>

namespace libcomp
{

/// Proper signature of a DOS header.
#define IMAGE_DOS_SIGNATURE     0x5A4D

/// Proper signature of a PE image file header.
#define IMAGE_NT_SIGNATURE      0x00004550

/// Proper magic for a 32-bit COFF optional header.
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b

/// Proper magic for a 64-bit COFF optional header.
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b

/// Proper magic for a ROM COFF optional header.
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC  0x107

/// Machine type for an i386 (x86) system.
#define IMAGE_FILE_MACHINE_I386 0x014c

/// Machine type for an Intel Itanium system.
#define IMAGE_FILE_MACHINE_IA64 0x0200

/// Machine type for a AMD 64-bit (x86_64) system.
#define IMAGE_FILE_MACHINE_AMD64 0x8664

/// Generic default subsystem.
#define IMAGE_SUBSYSTEM_NATIVE      1

/// Subsystem for a Windows GUI application.
#define IMAGE_SUBSYSTEM_WINDOWS_GUI 2

/// Subsystem for a Windows CLI application.
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3

/// File header format for COFF image files.
typedef struct _IMAGE_FILE_HEADER
{
    /// Architecture of the computer.
    uint16_t Machine;

    /// Number of sections in the image file.
    uint16_t NumberOfSections;

    /// Data and time the image was created in seconds since the UNIX epoch.
    uint32_t TimeDateStamp;

    /// Offset of the symbol table in bytes (or 0 if none exists).
    uint32_t PointerToSymbolTable;

    /// Number of symbols in the symbol table.
    uint32_t NumberOfSymbols;

    /// Size of the optional header.
    uint16_t SizeOfOptionalHeader;

    /// Characteristics of the image.
    uint16_t Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

/// Representation of a COFF data directory.
typedef struct _IMAGE_DATA_DIRECTORY
{
    /// Relative virtual address of the table.
    uint32_t VirtualAddress;

    /// Size of the table in bytes.
    uint32_t Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

/// Max number of directory entries.
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

/// Optional header format (32-bit version) for COFF image files.
typedef struct _IMAGE_OPTIONAL_HEADER
{
    /// Magic to indicate the type of image.
    uint16_t Magic;

    /// Major version number of the linker.
    uint8_t  MajorLinkerVersion;

    /// Minor version number of the linker.
    uint8_t  MinorLinkerVersion;

    /// Sum of all the code sections in bytes.
    uint32_t SizeOfCode;

    /// Sum of all the initialized data sections in bytes.
    uint32_t SizeOfInitializedData;

    /// Sum of all the uninitialized data sections in bytes.
    uint32_t SizeOfUninitializedData;

    /// Pointer to entry point function, relative to image base address.
    uint32_t AddressOfEntryPoint;

    /// Pointer to beginning of the code section, relative to image base.
    uint32_t BaseOfCode;

    /// Pointer to beginning of the data section, relative to image base.
    uint32_t BaseOfData;

    /**
     * Preferred address of the first byte of the image when loaded into
     * memory. This value is a multiple of 64KiB and defaults to 0x00400000
     * for applications.
     */
    uint32_t ImageBase;

    /**
     * Alignment of sections when loaded into memory (must be greater than or
     * equal to FileAlignment).
     */
    uint32_t SectionAlignment;

    /**
     * Alignment of the raw data sections in the image file (in bytes). This
     * value is a power of 2 between 512 and 64KiB.
     */
    uint32_t FileAlignment;

    /// Major version of the required OS.
    uint16_t MajorOperatingSystemVersion;

    /// Minor version of the required OS.
    uint16_t MinorOperatingSystemVersion;

    /// Major version of the image.
    uint16_t MajorImageVersion;

    /// Minor version of the image.
    uint16_t MinorImageVersion;

    /// Major version of the subsystem.
    uint16_t MajorSubsystemVersion;

    /// Minor version of the subsystem.
    uint16_t MinorSubsystemVersion;

    /// Reserved value set to 0.
    uint32_t Win32VersionValue;

    /// Size of the image (including headers - multiple of SectionAlignment).
    uint32_t SizeOfImage;

    /// Combined size of all headers rounded to a multiple of FileAlignment.
    uint32_t SizeOfHeaders;

    /// Image file checksum.
    uint32_t CheckSum;

    /// Subsystem required to run the image.
    uint16_t Subsystem;

    /// DLL characteristics of the image.
    uint16_t DllCharacteristics;

    /// Number of bytes to reserve for the stack.
    uint32_t SizeOfStackReserve;

    /// Number of bytes to commit for the stack.
    uint32_t SizeOfStackCommit;

    /// Number of bytes to reserve for the heap.
    uint32_t SizeOfHeapReserve;

    /// Number of bytes to commit for the heap.
    uint32_t SizeOfHeapCommit;

    /// Deprecated value.
    uint32_t LoaderFlags;

    /// Number of directory entries listed in DataDirectory.
    uint32_t NumberOfRvaAndSizes;

    /// Array of directory entries.
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

/// Optional header format (64-bit version) for COFF image files.
typedef struct _IMAGE_OPTIONAL_HEADER64
{
    /// Magic to indicate the type of image.
    uint16_t Magic;

    /// Major version number of the linker.
    uint8_t  MajorLinkerVersion;

    /// Minor version number of the linker.
    uint8_t  MinorLinkerVersion;

    /// Sum of all the code sections in bytes.
    uint32_t SizeOfCode;

    /// Sum of all the initialized data sections in bytes.
    uint32_t SizeOfInitializedData;

    /// Sum of all the uninitialized data sections in bytes.
    uint32_t SizeOfUninitializedData;

    /// Pointer to entry point function, relative to image base address.
    uint32_t AddressOfEntryPoint;

    /// Pointer to beginning of the code section, relative to image base.
    uint32_t BaseOfCode;

    /**
     * Preferred address of the first byte of the image when loaded into
     * memory. This value is a multiple of 64KiB and defaults to 0x00400000
     * for applications.
     */
    uint64_t ImageBase;

    /**
     * Alignment of sections when loaded into memory (must be greater than or
     * equal to FileAlignment).
     */
    uint32_t SectionAlignment;

    /**
     * Alignment of the raw data sections in the image file (in bytes). This
     * value is a power of 2 between 512 and 64KiB.
     */
    uint32_t FileAlignment;

    /// Major version of the required OS.
    uint16_t MajorOperatingSystemVersion;

    /// Minor version of the required OS.
    uint16_t MinorOperatingSystemVersion;

    /// Major version of the image.
    uint16_t MajorImageVersion;

    /// Minor version of the image.
    uint16_t MinorImageVersion;

    /// Major version of the subsystem.
    uint16_t MajorSubsystemVersion;

    /// Minor version of the subsystem.
    uint16_t MinorSubsystemVersion;

    /// Reserved value set to 0.
    uint32_t Win32VersionValue;

    /// Size of the image (including headers - multiple of SectionAlignment).
    uint32_t SizeOfImage;

    /// Combined size of all headers rounded to a multiple of FileAlignment.
    uint32_t SizeOfHeaders;

    /// Image file checksum.
    uint32_t CheckSum;

    /// Subsystem required to run the image.
    uint16_t Subsystem;

    /// DLL characteristics of the image.
    uint16_t DllCharacteristics;

    /// Number of bytes to reserve for the stack.
    uint64_t SizeOfStackReserve;

    /// Number of bytes to commit for the stack.
    uint64_t SizeOfStackCommit;

    /// Number of bytes to reserve for the heap.
    uint64_t SizeOfHeapReserve;

    /// Number of bytes to commit for the heap.
    uint64_t SizeOfHeapCommit;

    /// Deprecated value.
    uint32_t LoaderFlags;

    /// Number of directory entries listed in DataDirectory.
    uint32_t NumberOfRvaAndSizes;

    /// Array of directory entries.
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

/// Format of the main hader of a PE image file (32-bit version).
typedef struct _IMAGE_NT_HEADERS
{
    /// 4-byte signature indicating the file is a PE image ("PE\0\0").
    uint32_t Signature;

    /// The file header.
    IMAGE_FILE_HEADER FileHeader;

    /// The optional file header.
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

/// Format of the main hader of a PE image file (64-bit version).
typedef struct _IMAGE_NT_HEADERS64
{
    /// 4-byte signature indicating the file is a PE image ("PE\0\0").
    uint32_t Signature;

    /// The file header.
    IMAGE_FILE_HEADER FileHeader;

    /// The optional file header.
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

/// Format of the DOS header that appears at the beginning of a PE file.
typedef struct _IMAGE_DOS_HEADER
{
    /// MZ header signature.
    uint16_t e_magic;

    /// Bytes on last page of file.
    uint16_t e_cblp;

    /// Pages in file.
    uint16_t e_cp;

    /// Relocations.
    uint16_t e_crlc;

    /// Size of the header in paeagraphs.
    uint16_t e_cparhdr;

    /// Minimum extra paragraphs needed.
    uint16_t e_minalloc;

    /// Maximum extra paragraphs needed.
    uint16_t e_maxalloc;

    /// Initial (relative) SS value.
    uint16_t e_ss;

    /// Initial SP value.
    uint16_t e_sp;

    /// Checksum.
    uint16_t e_csum;

    /// Initial IP value.
    uint16_t e_ip;

    /// Initial (relative) CS value.
    uint16_t e_cs;

    /// File address of relocation table.
    uint16_t e_lfarlc;

    /// Overlay number.
    uint16_t e_ovno;

    /// Reserved words.
    uint16_t e_res[4];
    /// OEM identifier (for e_oeminfo).
    uint16_t e_oemid;

    /// OEM information; e_oemid specific.
    uint16_t e_oeminfo;

    /// Reserved words.
    uint16_t e_res2[10];

    /// Offset to extended header.
    uint32_t e_lfanew;
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

/// Max length of a short section name.
#define IMAGE_SIZEOF_SHORT_NAME 8

/// Structure of a section header in a COFF image file.
typedef struct _IMAGE_SECTION_HEADER
{
    /// Name of the section (encoded as UTF-8).
    char Name[IMAGE_SIZEOF_SHORT_NAME];

    /**
     * @var IMAGE_SECTION_HEADER::Misc
     * @brief PhysicalAddress or VirtualSize.
     */
    union {
        /// The file address.
        uint32_t PhysicalAddress;

        /// Total size of the section when loaded into memory.
        uint32_t VirtualSize;
    } Misc;

    /// Address of the first byte of the section when loaded into memory.
    uint32_t VirtualAddress;

    /// Size of the initialized data on the disk (in bytes).
    uint32_t SizeOfRawData;

    /// Pointer to the first page of initialized data within the COFF file.
    uint32_t PointerToRawData;

    /// Pointer to the beginning of the relocation entries for the section.
    uint32_t PointerToRelocations;

    /// Pointer to the beginning of line number entries for the section.
    uint32_t PointerToLinenumbers;

    /// Number of relocation entries for the section.
    uint16_t NumberOfRelocations;

    /// Number of line number entries for the section.
    uint16_t NumberOfLinenumbers;

    /// Characteristics of the section.
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

} // namespace libcomp

#endif // LIBCOMP_SRC_PEFORMAT_H
