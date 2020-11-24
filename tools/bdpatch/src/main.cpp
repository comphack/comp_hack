/**
 * @file tools/bdpatch/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to read and write BinaryData files.
 *
 * This tool will read and write BinaryData files.
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

// bdpatch Includes
#include "BinaryData.h"

// Standard C++11 Includes
#include <fstream>
#include <iostream>
#include <map>

// libcomp Includes
#include <Constants.h>
#include <Log.h>
#include <Object.h>

// Ignore warnings
#include <PushIgnore.h>

// tinyxml2 Includes
#include <tinyxml2.h>

// Stop ignoring warnings
#include <PopIgnore.h>

int Usage(
    const char* szAppName,
    const std::map<
        std::string,
        std::pair<std::string, std::function<libcomp::BinaryDataSet*(void)>>>&
        binaryTypes) {
  std::cerr << "USAGE: " << szAppName << " load TYPE IN OUT" << std::endl;
  std::cerr << "USAGE: " << szAppName << " save TYPE IN OUT" << std::endl;
  std::cerr << "USAGE: " << szAppName << " flatten TYPE IN OUT" << std::endl;
  std::cerr << std::endl;
  std::cerr << "TYPE indicates the format of the BinaryData and can "
            << "be one of:" << std::endl;

  for (auto& typ : binaryTypes) {
    std::cerr << typ.second.first << std::endl;
  }

  std::cerr << std::endl;
  std::cerr << "Mode 'load' will take the input BinaryData file and "
            << "write the output XML file." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Mode 'save' will take the input XML file and "
            << "write the output BinaryData file." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Mode 'flatten' will take the input BinaryData file and "
            << "write the output text file." << std::endl;

  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  auto binaryTypes = EnumerateBinaryDataTypes();

  if (5 != argc) {
    return Usage(argv[0], binaryTypes);
  }

  libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

  libcomp::String mode = argv[1];
  libcomp::String bdType = argv[2];
  const char* szInPath = argv[3];
  const char* szOutPath = argv[4];

  if ("load" != mode && "save" != mode && "flatten" != mode) {
    return Usage(argv[0], binaryTypes);
  }

  libcomp::BinaryDataSet* pSet = nullptr;

  auto match = binaryTypes.find(bdType.ToUtf8());

  if (binaryTypes.end() != match) {
    pSet = (match->second.second)();
  }

  if (!pSet) {
    return Usage(argv[0], binaryTypes);
  }

  if ("qmp" == bdType && ("load" == mode || "flatten" == mode)) {
    // Manually load single record
    std::ifstream file;
    file.open(szInPath, std::ifstream::binary);

    // Read and discard magic
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if (magic != QMP_FORMAT_MAGIC) {
      std::cerr << "File magic invlalid for Qmp file: " << szInPath
                << std::endl;

      return EXIT_FAILURE;
    }

    auto qmp = std::make_shared<objects::QmpFile>();
    if (!qmp->Load(file)) {
      std::cerr << "Failed to load Qmp file: " << szInPath << std::endl;

      return EXIT_FAILURE;
    }

    ((ManualBinaryDataSet*)pSet)->AddRecord(qmp);
  }

  if ("load" == mode) {
    if ("qmp" != bdType) {
      std::ifstream file;
      file.open(szInPath, std::ifstream::binary);

      if (!pSet->Load(file)) {
        std::cerr << "Failed to load file: " << szInPath << std::endl;

        return EXIT_FAILURE;
      }
    }

    std::ofstream out;
    out.open(szOutPath);

    out << pSet->GetXml().c_str();

    if (!out.good()) {
      std::cerr << "Failed to save file: " << szOutPath << std::endl;

      return EXIT_FAILURE;
    }
  } else if ("flatten" == mode) {
    if ("qmp" != bdType) {
      std::ifstream file;
      file.open(szInPath, std::ifstream::binary);

      if (!pSet->Load(file)) {
        std::cerr << "Failed to load file: " << szInPath << std::endl;

        return EXIT_FAILURE;
      }
    }

    std::ofstream out;
    out.open(szOutPath);

    out << pSet->GetTabular().c_str();

    if (!out.good()) {
      std::cerr << "Failed to save file: " << szOutPath << std::endl;

      return EXIT_FAILURE;
    }
  } else if ("save" == mode) {
    tinyxml2::XMLDocument doc;

    if (tinyxml2::XML_SUCCESS != doc.LoadFile(szInPath)) {
      std::cerr << "Failed to parse file: " << szInPath << std::endl;

      return EXIT_FAILURE;
    }

    if (!pSet->LoadXml(doc)) {
      std::cerr << "Failed to load file: " << szInPath << std::endl;

      return EXIT_FAILURE;
    }

    std::ofstream out;
    out.open(szOutPath, std::ofstream::binary);

    if ("qmp" == bdType) {
      // Write magic
      uint32_t magic = QMP_FORMAT_MAGIC;
      out.write(reinterpret_cast<char*>(&magic), sizeof(uint32_t));

      // Write (single) entry manually
      for (auto obj : pSet->GetObjects()) {
        if (!obj->Save(out) || !out.good()) {
          std::cerr << "Failed to save QMP file: " << szOutPath << std::endl;

          return EXIT_FAILURE;
        }
      }
    } else {
      if (!pSet->Save(out)) {
        std::cerr << "Failed to save file: " << szOutPath << std::endl;

        return EXIT_FAILURE;
      }
    }
  }

#ifndef EXOTIC_PLATFORM
  // Stop the logger
  delete libcomp::Log::GetSingletonPtr();
#endif  // !EXOTIC_PLATFORM

  return EXIT_SUCCESS;
}
