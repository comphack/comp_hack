/**
 * @file tools/translator/src/translator.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to build translated files (BinaryData).
 *
 * This tool will build translated files (BinaryData).
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
#include <BinaryData.h>

// Standard C Includes
#include <cstdlib>

// Standard C++11 Includes
#include <fstream>
#include <iostream>
#include <list>
#include <map>

// libcomp Includes
#include <Constants.h>
#include <Crypto.h>
#include <DataStore.h>
#include <Log.h>
#include <Object.h>
#include <ScriptEngine.h>

// Ignore warnings
#include <PushIgnore.h>

// tinyxml2 Includes
#include <tinyxml2.h>

// Stop ignoring warnings
#include <PopIgnore.h>

libcomp::String gLintPath = "xmllint";

struct Translator {
  Translator(const char* szProgram);

  libcomp::DataStore store;
  libcomp::ScriptEngine engine;
  bool didError;
  std::map<std::string,
           std::pair<std::string, std::function<libcomp::BinaryDataSet*(void)>>>
      binaryTypes;
};

std::unique_ptr<Translator> gTranslator;

static bool Exists(const libcomp::String& path) {
  return gTranslator->store.Exists(path);
}

static bool DeleteFile(const libcomp::String& path) {
  if (!gTranslator->store.Delete(path)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to delete file: %1\n").Arg(path));

    return false;
  }

  return true;
}

static bool DeleteDirectory(const libcomp::String& path) {
  if (!gTranslator->store.Delete(path, true)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to delete directory: %1\n").Arg(path));

    return false;
  }

  return true;
}

static bool CreateDirectory(const libcomp::String& path) {
  if (!gTranslator->store.CreateDirectory(path)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to create directory: %1\n").Arg(path));

    return false;
  }

  return true;
}

static bool CompileFile(const libcomp::String& bdType,
                        const libcomp::String& inPath,
                        const libcomp::String& outPath) {
  libcomp::BinaryDataSet* pSet = nullptr;

  auto match = gTranslator->binaryTypes.find(bdType.ToUtf8());

  if (gTranslator->binaryTypes.end() != match) {
    pSet = (match->second.second)();
  }

  if (!pSet) {
    LogGeneralErrorMsg(
        libcomp::String("Unknown binary data type: %1\n").Arg(bdType));

    return false;
  }

  tinyxml2::XMLDocument doc;

  if (tinyxml2::XML_SUCCESS != doc.LoadFile(inPath.C())) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to parse file: %1\n").Arg(inPath));

    return false;
  }

  if (!pSet->LoadXml(doc)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to load file: %1\n").Arg(inPath));

    return false;
  }

  std::ofstream out;
  out.open(outPath.C(), std::ofstream::binary);

  if (!pSet->Save(out)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to save file: %1\n").Arg(outPath));

    return false;
  }

  return true;
}

static bool DecompileFile(const libcomp::String& bdType,
                          const libcomp::String& inPath,
                          const libcomp::String& outPath) {
  libcomp::BinaryDataSet* pSet = nullptr;

  auto match = gTranslator->binaryTypes.find(bdType.ToUtf8());

  if (gTranslator->binaryTypes.end() != match) {
    pSet = (match->second.second)();
  }

  if (!pSet) {
    LogGeneralErrorMsg(
        libcomp::String("Unknown binary data type: %1\n").Arg(bdType));

    return false;
  }

  std::ifstream file;
  file.open(inPath.C(), std::ifstream::binary);

  if (!pSet->Load(file)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to load file: %1\n").Arg(inPath));

    return false;
  }

  std::ofstream out;
  out.open(outPath.C(), std::ofstream::binary);

  out << pSet->GetXml().c_str();

  if (!out.good()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to save file: %1\n").Arg(outPath));

    return false;
  }

  return true;
}

static bool EncryptFile(const libcomp::String& inPath,
                        const libcomp::String& outPath) {
  std::vector<char> data = libcomp::Crypto::LoadFile(inPath.ToUtf8());

  if (data.empty()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to read file: %1\n").Arg(inPath));

    return false;
  }

  if (!libcomp::Crypto::EncryptFile(outPath.ToUtf8(), data)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to write file: %1\n").Arg(outPath));

    return false;
  }

  return true;
}

static bool DecryptFile(const libcomp::String& inPath,
                        const libcomp::String& outPath) {
  std::vector<char> data = libcomp::Crypto::LoadFile(inPath.ToUtf8());

  if (data.empty()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to read file: %1\n").Arg(inPath));

    return false;
  }

  if (!libcomp::Crypto::DecryptFile(data)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to decrypt file: %1\n").Arg(outPath));

    return false;
  }

  std::ofstream out;
  out.open(outPath.C(), std::ofstream::out | std::ofstream::binary);
  out.write(&data[0], static_cast<std::streamsize>(data.size()));

  if (!out.good()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to write file: %1\n").Arg(outPath));

    return false;
  }

  return true;
}

static bool CopyFile(const libcomp::String& inPath,
                     const libcomp::String& outPath) {
  std::vector<char> data = libcomp::Crypto::LoadFile(inPath.ToUtf8());

  if (data.empty()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to read file: %1\n").Arg(inPath));

    return false;
  }

  std::ofstream out;
  out.open(outPath.C(), std::ofstream::out | std::ofstream::binary);
  out.write(&data[0], static_cast<std::streamsize>(data.size()));

  if (!out.good()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to write file: %1\n").Arg(outPath));

    return false;
  }

  return true;
}

static Sqrat::Object GetRecursiveFiles(const libcomp::String& path) {
  std::list<libcomp::String> files, dirs, symLinks;

  if (gTranslator->store.GetListing(path, files, dirs, symLinks, true)) {
    Sqrat::Array arr(gTranslator->engine.GetVM(), (SQInteger)files.size());

    int i = 0;

    for (auto file : files) {
      arr.SetValue(i++, file);
    }

    return arr;
  } else {
    LogGeneralErrorMsg(
        libcomp::String("Failed to get directory listing: %1\n").Arg(path));

    return {};
  }
}

static Sqrat::Object AvailableEncodings() {
  auto encodings = libcomp::Convert::AvailableEncodings();

  Sqrat::Array arr(gTranslator->engine.GetVM(), (SQInteger)encodings.size());

  int i = 0;

  for (auto enc : encodings) {
    arr.SetValue(i++, enc);
  }

  return arr;
}

static libcomp::String GetEncoding() {
  return libcomp::Convert::EncodingToString(
      libcomp::Convert::GetDefaultEncoding());
}

static bool SetEncoding(const libcomp::String& enc) {
  libcomp::Convert::Encoding_t encoding =
      libcomp::Convert::EncodingFromString(enc);
  if (libcomp::Convert::Encoding_t::ENCODING_DEFAULT != encoding) {
    libcomp::Convert::SetDefaultEncoding(encoding);

    return true;
  }

  return false;
}

static void LogInfo(const libcomp::String& msg) { LogGeneralInfoMsg(msg); }

static void LogError(const libcomp::String& msg) {
  LogGeneralErrorMsg(msg);
  gTranslator->didError = true;
}

static bool CompileSplitFiles(const libcomp::String& bdType,
                              Sqrat::Array filesArray,
                              const libcomp::String& outPath) {
  if (filesArray.IsNull()) {
    LogGeneralErrorMsg("Invalid arguments\n");

    return false;
  }

  std::list<libcomp::String> files;

  for (int i = 0; i < (int)filesArray.GetSize(); ++i) {
    auto pFile = filesArray.GetValue<libcomp::String>(i);

    if (!pFile) {
      LogGeneralErrorMsg("Invalid arguments\n");

      return false;
    }

    files.push_back(*pFile);
  }

  if (files.empty()) {
    LogGeneralErrorMsg("Invalid arguments\n");

    return false;
  }

  libcomp::BinaryDataSet* pSet = nullptr;

  auto match = gTranslator->binaryTypes.find(bdType.ToUtf8());

  if (gTranslator->binaryTypes.end() != match) {
    pSet = (match->second.second)();
  }

  if (!pSet) {
    LogGeneralErrorMsg(
        libcomp::String("Unknown binary data type: %1\n").Arg(bdType));

    return false;
  }

  for (auto inPath : files) {
    tinyxml2::XMLDocument doc;

    if (tinyxml2::XML_SUCCESS != doc.LoadFile(inPath.C())) {
      LogGeneralErrorMsg(
          libcomp::String("Failed to parse file: %1\n").Arg(inPath));

      return false;
    }

    if (!pSet->LoadXml(doc, true)) {
      LogGeneralErrorMsg(
          libcomp::String("Failed to load file: %1\n").Arg(inPath));

      return false;
    }
  }

  std::ofstream out;
  out.open(outPath.C(), std::ofstream::binary);

  if (!pSet->Save(out)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to save file: %1\n").Arg(outPath));

    return false;
  }

  return true;
}

static bool Include(const libcomp::String& path) {
  std::vector<char> file = libcomp::Crypto::LoadFile(path.ToUtf8());

  if (file.empty()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to include script: %1\n").Arg(path));

    return false;
  }

  file.push_back(0);

  if (!gTranslator->engine.Eval(&file[0], path)) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to evaluate script: %1\n").Arg(path));

    return false;
  }

  return true;
}

static bool ReplaceText(Sqrat::Table replacementsTable,
                        const libcomp::String& inPath,
                        const libcomp::String& outPath) {
  std::list<std::pair<libcomp::String, libcomp::String>> replacements;

  if (replacementsTable.IsNull()) {
    LogGeneralErrorMsg("Invalid arguments\n");

    return false;
  }

  Sqrat::Object::iterator it;

  while (replacementsTable.Next(it)) {
    Sqrat::Object key(it.getKey(), gTranslator->engine.GetVM());
    Sqrat::Object value(it.getValue(), gTranslator->engine.GetVM());

    auto pKey = key.Cast<Sqrat::SharedPtr<libcomp::String>>();
    auto pValue = value.Cast<Sqrat::SharedPtr<libcomp::String>>();

    if (!pKey || !pValue) {
      LogGeneralErrorMsg("Invalid arguments\n");

      return false;
    }

    replacements.push_back(std::make_pair(*pKey, *pValue));
  }

  std::vector<char> data = libcomp::Crypto::LoadFile(inPath.ToUtf8());

  if (data.empty()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to read file: %1\n").Arg(inPath));

    return false;
  }

  data.push_back(0);
  libcomp::String s(&data[0]);
  data.clear();

  for (auto replacement : replacements) {
    s = s.Replace(replacement.first, replacement.second);
  }

  std::ofstream out;
  out.open(outPath.C(), std::ofstream::out | std::ofstream::binary);
  out.write(s.C(), static_cast<std::streamsize>(s.Size()));

  if (!out.good()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to write file: %1\n").Arg(outPath));

    return false;
  }

  return true;
}

static void SetLintPath(const libcomp::String& path) { gLintPath = path; }

static bool HaveLint() {
  char szBuffer[10 * 1024];

  auto cmd = libcomp::String("%1 --version 2>&1").Arg(gLintPath);

#ifdef _WIN32
  FILE* pPipe = _popen(cmd.C(), "rt");
#else
  FILE* pPipe = popen(cmd.C(), "r");
#endif

  if (!pPipe) {
    return false;
  }

  while (fgets(szBuffer, sizeof(szBuffer), pPipe)) {
    // do nothing with it
  }

  if (feof(pPipe)) {
#ifdef _WIN32
    int ret = _pclose(pPipe);
#else
    int ret = pclose(pPipe);
#endif

    return 0 == ret;
  } else {
    // Broken pipe.
#ifdef _WIN32
    _pclose(pPipe);
#else
    pclose(pPipe);
#endif

    return false;
  }
}

static int LintXml(const libcomp::String& schema, const libcomp::String& xml) {
  char szBuffer[10 * 1024];
  std::list<libcomp::String> output;

  auto cmd = libcomp::String("%1 -schema %2 %3 --noout 2>&1")
                 .Arg(gLintPath)
                 .Arg(schema)
                 .Arg(xml);

#ifdef _WIN32
  FILE* pPipe = _popen(cmd.C(), "rt");
#else
  FILE* pPipe = popen(cmd.C(), "r");
#endif

  if (!pPipe) {
    return -1;
  }

  while (fgets(szBuffer, sizeof(szBuffer), pPipe)) {
    output.push_back(szBuffer);
  }

  if (feof(pPipe)) {
#ifdef _WIN32
    int ret = _pclose(pPipe);
#else
    int ret = pclose(pPipe);
#endif

    if (0 != ret) {
      for (auto line : output) {
        LogError(line);
      }
    }

    return ret;
  } else {
    // Broken pipe.
#ifdef _WIN32
    _pclose(pPipe);
#else
    pclose(pPipe);
#endif

    return -1;
  }
}

Translator::Translator(const char* szProgram)
    : store(szProgram), engine(true), didError(false) {
  Sqrat::RootTable(engine.GetVM()).Func("LogInfo", LogInfo);
  Sqrat::RootTable(engine.GetVM()).Func("LogError", LogError);
  Sqrat::RootTable(engine.GetVM()).Func("Exists", Exists);
  Sqrat::RootTable(engine.GetVM())
      .Func("_GetRecursiveFiles", GetRecursiveFiles);
  Sqrat::RootTable(engine.GetVM()).Func("_CopyFile", CopyFile);
  Sqrat::RootTable(engine.GetVM()).Func("_DeleteFile", DeleteFile);
  Sqrat::RootTable(engine.GetVM()).Func("_DeleteDirectory", DeleteDirectory);
  Sqrat::RootTable(engine.GetVM()).Func("_CreateDirectory", CreateDirectory);
  Sqrat::RootTable(engine.GetVM()).Func("_CompileFile", CompileFile);
  Sqrat::RootTable(engine.GetVM()).Func("_DecompileFile", DecompileFile);
  Sqrat::RootTable(engine.GetVM())
      .Func("_CompileSplitFiles", CompileSplitFiles);
  Sqrat::RootTable(engine.GetVM()).Func("_EncryptFile", EncryptFile);
  Sqrat::RootTable(engine.GetVM()).Func("_DecryptFile", DecryptFile);
  Sqrat::RootTable(engine.GetVM()).Func("_Include", Include);
  Sqrat::RootTable(engine.GetVM()).Func("_ReplaceText", ReplaceText);
  Sqrat::RootTable(engine.GetVM())
      .Func("AvailableEncodings", AvailableEncodings);
  Sqrat::RootTable(engine.GetVM()).Func("GetEncoding", GetEncoding);
  Sqrat::RootTable(engine.GetVM()).Func("_SetEncoding", SetEncoding);
  Sqrat::RootTable(engine.GetVM()).Func("_LintXml", LintXml);
  Sqrat::RootTable(engine.GetVM()).Func("SetLintPath", SetLintPath);
  Sqrat::RootTable(engine.GetVM()).Func("HaveLint", HaveLint);

  binaryTypes = EnumerateBinaryDataTypes();
}

static bool LoadAndRunScriptFile(const char* szScriptFile) {
  std::vector<char> scriptData = libcomp::Crypto::LoadFile(szScriptFile);
  if (scriptData.empty()) {
    LogGeneralErrorMsg(
        libcomp::String("Failed to read script file: %1\n").Arg(szScriptFile));

    return false;
  }

  scriptData.push_back(0);
  auto script = libcomp::String(&scriptData[0]);
  scriptData.clear();

  if (!gTranslator->engine.Eval(script, szScriptFile)) {
    LogGeneralErrorMsg("Build script failed\n");

    return false;
  }

  return !gTranslator->didError;
}

int main(int argc, char* argv[]) {
  libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();
  std::unique_ptr<libcomp::Log> log(libcomp::Log::GetSingletonPtr());
  log->SetLogLevel(libcomp::LogComponent_t::ScriptEngine,
                   libcomp::Log::Level_t::LOG_LEVEL_INFO);
  log->SetLogPath("build.log", true);

  gTranslator.reset(new Translator(argv[0]));

  if (!gTranslator->store.AddSearchPaths({"."})) {
    LogGeneralErrorMsg("Failed to initialize PhysFS\n");

    return EXIT_FAILURE;
  }

  if (!LoadAndRunScriptFile(2 <= argc ? argv[1] : "build.nut")) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
