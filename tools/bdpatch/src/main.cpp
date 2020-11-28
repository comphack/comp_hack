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
#include <ArgumentParser.h>
#include <Constants.h>
#include <Log.h>
#include <Object.h>

// Ignore warnings
#include <PushIgnore.h>

// tinyxml2 Includes
#include <tinyxml2.h>

// Stop ignoring warnings
#include <PopIgnore.h>

/**
 * Class to handle parsing command line arguments for a server.
 */
class CommandLineParser : public libcomp::ArgumentParser {
 public:
  CommandLineParser();
  virtual ~CommandLineParser();
};

CommandLineParser::CommandLineParser() : libcomp::ArgumentParser() {
  RegisterArgument(
      'e', "encoding", ArgumentType::REQUIRED,
      std::bind(
          [](CommandLineParser* pParser, ArgumentParser::Argument* pArg,
             const libcomp::String& arg) -> bool {
            (void)pParser;
            (void)pArg;

            libcomp::Convert::Encoding_t encoding =
                libcomp::Convert::EncodingFromString(arg);

            bool ok =
                libcomp::Convert::Encoding_t::ENCODING_DEFAULT != encoding;

            if (ok) {
              libcomp::Convert::SetDefaultEncoding(encoding);
            } else {
              std::cerr << "Unknown character encoding: " << arg << std::endl;
              std::cerr << "Valid encodings: " << arg << std::endl;

              for (auto enc : libcomp::Convert::AvailableEncodings()) {
                std::cerr << "- " << enc << std::endl;
              }

              std::cerr << std::endl;
            }

            return ok;
          },
          this, std::placeholders::_1, std::placeholders::_2));
}

CommandLineParser::~CommandLineParser() {}

int Usage(
    const char* szAppName,
    const std::map<
        std::string,
        std::pair<std::string, std::function<libcomp::BinaryDataSet*(void)>>>&
        binaryTypes) {
  std::cerr << "USAGE: " << szAppName << " [OPTION]... load TYPE IN OUT"
            << std::endl;
  std::cerr << "USAGE: " << szAppName << " [OPTION]... save TYPE IN OUT"
            << std::endl;
  std::cerr << "USAGE: " << szAppName << " [OPTION]... flatten TYPE IN OUT"
            << std::endl;
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

  std::cerr << std::endl;
  std::cerr << "Mandatory arguments to long options are mandatory for short "
               "options too."
            << std::endl;
  std::cerr << "  -e, --encoding=ENC          set encoding used for conversion "
               "(default=cp932)"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << "Valid encodings:" << std::endl;

  for (auto enc : libcomp::Convert::AvailableEncodings()) {
    std::cerr << "- " << enc << std::endl;
  }

  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  CommandLineParser args;

  auto binaryTypes = EnumerateBinaryDataTypes();

  if (!args.Parse(argc, argv) || (4 != args.GetStandardArguments().size())) {
    return Usage(argv[0], binaryTypes);
  }

  libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

  auto standardArgs = args.GetStandardArguments();
  libcomp::String mode = standardArgs[0];
  libcomp::String bdType = standardArgs[1];
  libcomp::String inPath = standardArgs[2];
  libcomp::String outPath = standardArgs[3];

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
    file.open(inPath.C(), std::ifstream::binary);

    // Read and discard magic
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if (magic != QMP_FORMAT_MAGIC) {
      std::cerr << "File magic invlalid for Qmp file: " << inPath << std::endl;

      return EXIT_FAILURE;
    }

    auto qmp = std::make_shared<objects::QmpFile>();
    if (!qmp->Load(file)) {
      std::cerr << "Failed to load Qmp file: " << inPath << std::endl;

      return EXIT_FAILURE;
    }

    ((ManualBinaryDataSet*)pSet)->AddRecord(qmp);
  }

  if ("load" == mode) {
    if ("qmp" != bdType) {
      std::ifstream file;
      file.open(inPath.C(), std::ifstream::binary);

      if (!pSet->Load(file)) {
        std::cerr << "Failed to load file: " << inPath << std::endl;

        return EXIT_FAILURE;
      }
    }

    std::ofstream out;
    out.open(outPath.C());

    out << pSet->GetXml().c_str();

    if (!out.good()) {
      std::cerr << "Failed to save file: " << outPath << std::endl;

      return EXIT_FAILURE;
    }
  } else if ("flatten" == mode) {
    if ("qmp" != bdType) {
      std::ifstream file;
      file.open(inPath.C(), std::ifstream::binary);

      if (!pSet->Load(file)) {
        std::cerr << "Failed to load file: " << inPath << std::endl;

        return EXIT_FAILURE;
      }
    }

    std::ofstream out;
    out.open(outPath.C());

    out << pSet->GetTabular().c_str();

    if (!out.good()) {
      std::cerr << "Failed to save file: " << outPath << std::endl;

      return EXIT_FAILURE;
    }
  } else if ("save" == mode) {
    tinyxml2::XMLDocument doc;

    if (tinyxml2::XML_SUCCESS != doc.LoadFile(inPath.C())) {
      std::cerr << "Failed to parse file: " << inPath << std::endl;

      return EXIT_FAILURE;
    }

    if (!pSet->LoadXml(doc)) {
      std::cerr << "Failed to load file: " << inPath << std::endl;

      return EXIT_FAILURE;
    }

    std::ofstream out;
    out.open(outPath.C(), std::ofstream::binary);

    if ("qmp" == bdType) {
      // Write magic
      uint32_t magic = QMP_FORMAT_MAGIC;
      out.write(reinterpret_cast<char*>(&magic), sizeof(uint32_t));

      // Write (single) entry manually
      for (auto obj : pSet->GetObjects()) {
        if (!obj->Save(out) || !out.good()) {
          std::cerr << "Failed to save QMP file: " << outPath << std::endl;

          return EXIT_FAILURE;
        }
      }
    } else {
      if (!pSet->Save(out)) {
        std::cerr << "Failed to save file: " << outPath << std::endl;

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
