/**
 * @file tools/rehash/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to generate a new hashlist.dat for an updater.
 *
 * This tool will generate a new hashlist.dat for an updater.
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
#include <CString.h>
#include <Compress.h>
#include <Crypto.h>
#include <DataStore.h>

// Standard C++11 Includes
#include <fstream>
#include <iostream>
#include <regex>
#include <unordered_map>
#include <vector>

// Standard C Includes
#include <ctime>

class FileData {
 public:
  libcomp::String path;
  libcomp::String compressed_hash;
  libcomp::String uncompressed_hash;

  int compressed_size;
  int uncompressed_size;
};

std::unordered_map<libcomp::String, FileData *> ParseFileList(
    const std::vector<char> data) {
  std::unordered_map<libcomp::String, FileData *> files;

  std::list<libcomp::String> lines =
      libcomp::String(&data[0], data.size()).Split("\n");

  static const std::regex fileMatcher(
      "FILE : (.+),([0-9a-fA-F]{32}),([0-9]+),([0-9a-fA-F]{32}),([0-9]+)");

  // Parse each line of the hashlist.dat file.
  for (auto line : lines) {
    std::string cleanLine = line.Trimmed().ToUtf8();
    std::smatch match;

    if (!std::regex_match(cleanLine, match, fileMatcher)) continue;

    // Add each file entry to the map.
    FileData *info = new FileData;
    info->path = libcomp::String(match[1]).Mid(2).Replace("\\", "/");
    info->compressed_hash = libcomp::String(match[2]).ToUpper();
    info->compressed_size = libcomp::String(match[3]).ToInteger<int>();
    info->uncompressed_hash = libcomp::String(match[4]).ToUpper();
    info->uncompressed_size = libcomp::String(match[5]).ToInteger<int>();

    files[info->path] = info;
  }

  return files;
}

std::list<libcomp::String> RecursiveEntryList(const libcomp::String &dir) {
  std::list<libcomp::String> files;
  std::list<libcomp::String> dirs;
  std::list<libcomp::String> symLinks;

  libcomp::DataStore store(NULL);

  if (!store.AddSearchPath(dir)) {
    return {};
  }

  // Get all the files in the directory.
  if (!store.GetListing("/", files, dirs, symLinks, true, true)) {
    return {};
  }

  return files;
}

int main(int argc, char *argv[]) {
  // Check the arguments and print the usage.
  if (argc != 5 || libcomp::String(argv[1]) != "--base" ||
      libcomp::String(argv[3]) != "--overlay") {
    std::cerr << "SYNTAX: comp_rehash --base BASE --overlay OVERLAY"
              << std::endl;

    return -1;
  }

  libcomp::String base = argv[2];
  libcomp::String overlay = argv[4];

  // Read in the original file list
  std::unordered_map<libcomp::String, FileData *> files;

  {
    // Open and read the hashlist.dat file.
    auto hashlist = libcomp::Crypto::LoadFile(
        libcomp::String("%1/hashlist.dat").Arg(base).ToUtf8());

    if (hashlist.empty()) {
      std::cerr << "Failed to open the hashlist.dat file for reading."
                << std::endl;

      return -1;
    }

    // Parse the hashlist.dat
    files = ParseFileList(hashlist);
  }

  static const std::regex compressedRx("^.+\\.compressed$");

  // Find each file in the overlay and handle it.
  for (auto filePath : RecursiveEntryList(overlay)) {
    std::smatch match;

    auto file = filePath;
    auto fileString = file.ToUtf8();

    // See if the file is an *.compressed file and ignore it.
    if (std::regex_match(fileString, match, compressedRx)) continue;

    // Get the relative path.
    libcomp::String shortName = file.Mid(1);

    // Make the path absolute.
    file = overlay + file;

    // Relative path for the compressed file.
    libcomp::String shortComp = libcomp::String("%1.compressed").Arg(shortName);

    // Ignore the hashlist.dat and hashlist.ver files.
    if (shortName == "hashlist.dat") continue;
    if (shortName == "hashlist.ver") continue;

    // Check if the file is in the original hashlist.dat file.
    auto it = files.find(shortComp);

    // If the file is in the base, remove the entry.
    if (files.end() != it) {
      delete it->second;
    }

    it = files.find(shortName);

    if (files.end() != it) {
      delete it->second;

      files.erase(it);
    }

    // Create a new entry for the file.
    FileData *d = new FileData;
    d->path = shortComp;

    // Get the original file contents.
    std::vector<char> uncomp_data = libcomp::Crypto::LoadFile(file.ToUtf8());

    // Ignore empty files
    if (uncomp_data.empty()) {
      continue;
    }

    // Hash the original file.
    d->uncompressed_hash = libcomp::Crypto::MD5(uncomp_data).ToUpper();
    d->uncompressed_size = (int)uncomp_data.size();

    //
    // Process the compressed copy now.
    //

    // Calculate max size.
    int out_size = (int)((float)uncomp_data.size() * 0.001f + 0.5f);
    out_size += (int32_t)uncomp_data.size() + 12;

    char *out_buffer = new char[(size_t)out_size];

    // Compress the file.
    int32_t sz = libcomp::Compress::Compress(
        &uncomp_data[0], out_buffer, (int32_t)uncomp_data.size(), out_size, 9);

    // Write the compressed copy
    {
      std::ofstream file_comp;
      file_comp.open(libcomp::String(file + ".compressed").C(),
                     std::ofstream::binary);
      file_comp.write(out_buffer, (std::streamsize)sz);
      file_comp.close();
    }

    // Get the hash for the compressed copy.
    d->compressed_hash =
        libcomp::Crypto::MD5(std::vector<char>(out_buffer, out_buffer + sz))
            .ToUpper();
    d->compressed_size = sz;

    // Save the entry.
    files[shortComp] = d;

    delete[] out_buffer;
  }

  // Write the overlay hashlist.dat file now.
  std::ofstream hashlist;
  hashlist.open(libcomp::String("%1/hashlist.dat").Arg(overlay).C(),
                std::ofstream::binary);

  // Add each file in the list.
  for (auto file : files) {
    FileData *d = file.second;

    libcomp::String line = "FILE : .\\%1,%2,%3,%4,%5 ";
    line = line.Arg(d->path.Replace("/", "\\"));
    line = line.Arg(d->compressed_hash.ToUpper());
    line = line.Arg(d->compressed_size);
    line = line.Arg(d->uncompressed_hash.ToUpper());
    line = line.Arg(d->uncompressed_size);

    auto l = line.ToUtf8();

    hashlist.write(l.c_str(), (std::streamsize)l.size());
    hashlist.write("\r\n", 2);
  }

  libcomp::String exe = "EXE : .\\ImagineClient.exe ";
  libcomp::String ver = "VERSION : %1";

  // Generate a timestamp.
  char mbstr[100];
  std::time_t t = std::time(NULL);
  std::strftime(mbstr, sizeof(mbstr), "%Y%m%d%H%M%S", std::localtime(&t));

  ver = ver.Arg(mbstr);

  auto l = exe.ToUtf8();
  hashlist.write(l.c_str(), (std::streamsize)l.size());
  hashlist.write("\r\n", 2);
  l = ver.ToUtf8();
  hashlist.write(l.c_str(), (std::streamsize)l.size());
  hashlist.close();

  std::ofstream hashver;
  hashver.open(libcomp::String("%1/hashlist.ver").Arg(overlay).C(),
               std::ofstream::binary);
  hashver.write(l.c_str(), (std::streamsize)l.size());
  hashver.close();

  //
  // Compress the hashlist.dat file.
  //
  {
    // Read in the hashlist.dat file.
    auto uncomp_data = libcomp::Crypto::LoadFile(
        libcomp::String("%1/hashlist.dat").Arg(overlay).ToUtf8());

    // Close the hashlist.dat file.
    hashlist.close();

    // Calculate max size.
    int32_t out_size = (int32_t)((float)uncomp_data.size() * 0.001f + 0.5f);
    out_size += (int32_t)uncomp_data.size() + 12;

    char *out_buffer = new char[(size_t)out_size];

    // Compress the file.
    int32_t sz = libcomp::Compress::Compress(
        &uncomp_data[0], out_buffer, (int32_t)uncomp_data.size(), out_size, 9);

    // Write the compressed copy.
    {
      std::ofstream hashlist_comp;
      hashlist_comp.open(
          libcomp::String("%1/hashlist.dat.compressed").Arg(overlay).C(),
          std::ofstream::binary);
      hashlist_comp.write(out_buffer, (std::streamsize)sz);
      hashlist_comp.close();
    }

    delete[] out_buffer;
  }

  return 0;
}
