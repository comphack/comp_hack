/**
 * @file tools/exports/src/exports.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to patch the client with some export symbols.
 *
 * This tool will patch the client with some export symbols.
 * These are used to notify the GPU driver that it should use the
 * high performance GPU instead (for laptops).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <Crypto.h>
#include <PEFile.h>
#include <PEFormat.h>

// Standard C Includes
#include <cstdlib>
#include <ctime>

// Standard C++ Includes
#include <array>
#include <iostream>
#include <string>

/// Macro to access either the 32-bit or 64-bit PE header.
#define FileHdr \
  (mPEHeader64 ? mPEHeader64->FileHeader : mPEHeader32->FileHeader)

class MyPEFile : public libcomp::PEFile {
 public:
  MyPEFile(uint8_t *base) : libcomp::PEFile(base) {}
  bool WriteExports(std::vector<char> &file);
  bool HaveExports() const;
};

struct ExportDirectoryTable {
  uint32_t Characteristics;
  uint32_t TimeDateStamp;
  uint16_t MajorVersion;
  uint16_t MinorVersion;
  uint32_t Name;
  uint32_t Base;
  uint32_t NumberOfFunctions;
  uint32_t NumberOfNames;
  uint32_t AddressOfFunctions;
  uint32_t AddressOfNames;
  uint32_t AddressOfNameOrdinals;
};

struct ExportAddressTable {
  uint32_t Export[2];
};

struct ExportNamesTable {
  uint32_t Name[2];
};

struct ExportNameOrdinalsTable {
  uint16_t Ordinal[2];
};

bool MyPEFile::WriteExports(std::vector<char> &file) {
  if (HaveExports()) {
    fprintf(stderr, "ERROR: Refusing to replace the existing exports.\n");

    return false;
  }

  if (1 > mOptHeader32->NumberOfRvaAndSizes) {
    fprintf(stderr, "No RVA for the exports!\n");

    return false;
  }

  // uint32_t virtAddr = 0x601000;
  uint32_t virtAddr = 0x8916000;
  uint32_t expBase = virtAddr - mOptHeader32->ImageBase;  // RVA
  uint32_t strBase = (uint32_t)(
      expBase + sizeof(ExportDirectoryTable) + sizeof(ExportAddressTable) +
      sizeof(ExportNamesTable) + sizeof(ExportNameOrdinalsTable));

  // Fix the size of the image
  mOptHeader32->SizeOfImage = expBase + 4096;

  std::vector<char> strData;

  std::array<std::string, 3> strings = {{
      // "ImagineOption.exe",
      "ImagineClient.exe",
      "AmdPowerXpressRequestHighPerformance",
      "NvOptimusEnablement",
  }};

  uint32_t strRVAs[] = {
      0,
      0,
      0,
  };

  for (std::array<std::string, 3>::size_type i = 0; i < strings.size(); ++i) {
    strRVAs[i] = (uint32_t)(strBase + strData.size());
    std::vector<char> sData(strings[i].c_str(),
                            strings[i].c_str() + strings[i].size());
    strData.insert(strData.end(), sData.begin(), sData.end());
    strData.push_back(0);
  }

  while (strData.size() % 4) {
    strData.push_back(0);
  }

  // Push back the two variables.
  uint32_t varRVAs[2] = {
      0,
      0,
  };

  for (int i = 0; i < 2; ++i) {
    varRVAs[i] = (uint32_t)(strBase + strData.size());
    strData.push_back(1);
    strData.push_back(0);
    strData.push_back(0);
    strData.push_back(0);
  }

  uint16_t sectionIdx = FileHdr.NumberOfSections++;
  auto &sec = mSectionHeaders[sectionIdx];
  memset(&sec, 0, sizeof(libcomp::IMAGE_SECTION_HEADER));
  strcpy(sec.Name, ".edata");
  sec.Misc.VirtualSize = 4096;
  sec.VirtualAddress = expBase;
  sec.SizeOfRawData = 4096;
  sec.PointerToRawData = (uint32_t)file.size();
  sec.Characteristics =
      0x40000040;  // IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ

  auto &pDataDirectory = mOptHeader32->DataDirectory[0];

  /// @todo Check this does not overlap with anything?
  pDataDirectory.VirtualAddress = expBase;
  pDataDirectory.Size = (uint32_t)(strBase + strData.size() - 8 - expBase);

  ExportDirectoryTable expDir;
  expDir.Characteristics = 0;
  expDir.TimeDateStamp = (uint32_t)time(0);
  expDir.MajorVersion = 0;
  expDir.MinorVersion = 0;
  expDir.Name = strRVAs[0];
  expDir.Base = 1;
  expDir.NumberOfFunctions = 2;
  expDir.NumberOfNames = 2;
  expDir.AddressOfFunctions =
      (uint32_t)(expBase + sizeof(ExportDirectoryTable));
  expDir.AddressOfNames = (uint32_t)(expBase + sizeof(ExportDirectoryTable) +
                                     sizeof(ExportAddressTable));
  expDir.AddressOfNameOrdinals =
      (uint32_t)(expBase + sizeof(ExportDirectoryTable) +
                 sizeof(ExportAddressTable) + sizeof(ExportNamesTable));

  ExportAddressTable expAddr;
  expAddr.Export[0] = varRVAs[0];
  expAddr.Export[1] = varRVAs[1];

  ExportNamesTable expNames;
  expNames.Name[0] = strRVAs[1];
  expNames.Name[1] = strRVAs[2];

  ExportNameOrdinalsTable expNameOrdinals;
  expNameOrdinals.Ordinal[0] = 0;
  expNameOrdinals.Ordinal[1] = 1;

  char edata[4096];
  char *szNext = edata;

  memset(edata, 0, sizeof(edata));
  memcpy(szNext, &expDir, sizeof(expDir));
  szNext += sizeof(expDir);
  memcpy(szNext, &expAddr, sizeof(expAddr));
  szNext += sizeof(expAddr);
  memcpy(szNext, &expNames, sizeof(expNames));
  szNext += sizeof(expNames);
  memcpy(szNext, &expNameOrdinals, sizeof(expNameOrdinals));
  szNext += sizeof(expNameOrdinals);
  memcpy(szNext, &strData[0], strData.size());
  szNext += strData.size();

  file.insert(file.end(), edata, edata + sizeof(edata));

  return true;
}

bool MyPEFile::HaveExports() const {
  if (1 <= mOptHeader32->NumberOfRvaAndSizes) {
    auto &pDataDirectory = mOptHeader32->DataDirectory[0];
    return 0 < pDataDirectory.Size;
  }

  return false;
}

int main(int argc, char *argv[]) {
  if (3 != argc) {
    fprintf(stderr, "USAGE: %s IN OUT\n", argv[0]);

    return EXIT_FAILURE;
  }

  std::vector<char> file = libcomp::Crypto::LoadFile(argv[1]);

  // Fix the file alignment before we read the pointer from the vector.
  while (file.size() % 4096) {
    file.push_back(0);
  }

  MyPEFile pe((uint8_t *)&file[0]);

  if (!pe.WriteExports(file)) {
    fprintf(stderr, "ERROR: Failed to generate exports!\n");

    return EXIT_FAILURE;
  }

  FILE *out = fopen(argv[2], "wb");
  fwrite(&file[0], file.size(), 1, out);
  fclose(out);

  return EXIT_SUCCESS;
}
