/**
 * @file tools/patcher/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Client application patcher.
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

// Standard C++11 Includes
#include <cstdlib>
#include <fstream>
#include <iostream>

// libcomp Includes
#include <Config.h>
#include <Crypto.h>
#include <Endian.h>

// SHA-1 of the original unmodified client (1.666).
static const libcomp::String CLIENT_SHA1 =
    "45d8e66293ff289791aa85c0738f43e003328488";

// SHA-1 of the modified client (1.666).
static const libcomp::String CLIENT_PATCHED_SHA1 =
    "b438a4d921af881153adbb0b5ba2e26f29dc84ae";

static const uint8_t DLL_INJECTION[] = {
    0x68, 0x8C, 0xA0, 0x8A, 0x08, 0xFF, 0x15, 0xCC, 0xA5, 0x7E, 0x08, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x83, 0xC4, 0x08
};

void Usage(const char *szAppName)
{
    std::cerr << "USAGE: " << szAppName << " IN OUT" << std::endl;

    exit(EXIT_FAILURE);
}

bool PatchClient(std::vector<char>& data)
{
    // Patch the client to inject the DLL.
    memcpy(&data[0xE396DA], DLL_INJECTION, sizeof(DLL_INJECTION));
    strcpy(&data[0x15E908C], "comp_client.dll");

    return true;
}

bool SaveClient(const char *szOutPath, std::vector<char>& data)
{
    std::ofstream file;

    // Open the file.
    file.open(szOutPath, std::ofstream::binary);

    // Write to the file.
    file.write(&data[0], static_cast<std::streamsize>(data.size()));

    // Make sure it wrote.
    return file.good();
}

int main(int argc, char *argv[])
{
    if(3 != argc)
    {
        Usage(argv[0]);
    }

    const char *szInPath = argv[1];
    const char *szOutPath = argv[2];

    // Load the original client.
    std::vector<char> data = libcomp::Crypto::LoadFile(szInPath);

    if(data.empty())
    {
        std::cerr << "Failed to open input file '" << szInPath
            << "'." << std::endl;

        return EXIT_FAILURE;
    }

    // Make sure the client was not modified.
    libcomp::String hash = libcomp::Crypto::SHA1(data);

    if(CLIENT_SHA1 != hash)
    {
        if(CLIENT_PATCHED_SHA1 == hash)
        {
            std::cerr << "Input file has already been patched." << std::endl;
        }
        else
        {
            std::cerr << "Input file has been modified. "
                << "Cowardly refusing to patch it." << std::endl;
        }

        return EXIT_FAILURE;
    }

    // Patch the client.
    if(!PatchClient(data) || !SaveClient(szOutPath, data))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
