/**
 * @file tools/patcher/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Client application patcher.
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

// Standard C++11 Includes
#include <cstdlib>
#include <fstream>
#include <iostream>

// libcomp Includes
#include <Config.h>
#include <Decrypt.h>
#include <Endian.h>

// SHA-1 of the original unmodified client (1.666).
static const libcomp::String CLIENT_SHA1 =
    "45d8e66293ff289791aa85c0738f43e003328488";

// SHA-1 of the modified client (1.666).
static const libcomp::String CLIENT_PATCHED_SHA1 =
    "8b889ead13308ef562bc2b1fef59333db6923b6d";

/// Diffie-Hellman prime used by the server.
static const char *DH_PRIME =
    "9c4169bbe8f535f7a7404d4eb3ae22cf63c0450fc2c7b2a5a03794d4cfa9f290"
    "ff5774267885e60b848280e3a07468366e62f040dac3cb67e95e8f3dc4d97f94"
    "ad1d3d98f0b066f72b65cb391643a95bb96cf048ed5d60fb7af7a969f38abd23"
    "01f6a7ec4db7dafc2cfd1f417e0b634033fee8b102d62a28ec03d95266e2b0b3";

/*
/// 4-byte magic at the beginning of an encrypted file.
const char *ENCRYPTED_FILE_MAGIC = "CHED"; // COMP_hack Encrypted Data

/// Blowfish key used by the file encryption.
const char *ENCRYPTED_FILE_KEY = "}]#Su?Y}q!^f*S5O";

/// Blowfish initialization vector used by the file encryption.
const char *ENCRYPTED_FILE_IV = "P[?jd6c4";
*/

void Usage(const char *szAppName)
{
    std::cerr << "USAGE: " << szAppName << " IN OUT" << std::endl;

    exit(EXIT_FAILURE);
}

bool NopRegion(std::vector<char>& data, size_t offset, size_t size)
{
    if(offset >= data.size() || (offset + size) > data.size())
    {
        return false;
    }

    memset(&data[offset], 0x90 /* NOP */, size);

    return true;
}

bool PatchBlowfish(std::vector<char>& data)
{
    //
    // Magic
    //
    data[0xFA4E7] = libcomp::Config::ENCRYPTED_FILE_MAGIC[0];
    data[0xFA5E1] = libcomp::Config::ENCRYPTED_FILE_MAGIC[0];
    data[0xFA637] = libcomp::Config::ENCRYPTED_FILE_MAGIC[0];

    memcpy(&data[0x1537938], &libcomp::Config::ENCRYPTED_FILE_MAGIC[1], 3);

    //
    // Key
    //
    memcpy(&data[0x1537984], libcomp::Config::ENCRYPTED_FILE_KEY, 16);

    //
    // IV
    //
    uint32_t iv[2];

    memcpy(iv, libcomp::Config::ENCRYPTED_FILE_IV, sizeof(iv));

    iv[0] = be32toh(iv[0]);
    iv[1] = be32toh(iv[1]);

    memcpy(&data[0xFA8CB], &iv[0], sizeof(iv[0]));
    memcpy(&data[0x12F131], &iv[0], sizeof(iv[0]));

    memcpy(&data[0xFA8C6], &iv[1], sizeof(iv[1]));
    memcpy(&data[0x12F12C], &iv[1], sizeof(iv[1]));

    return true;
}

bool PatchClient(std::vector<char>& data)
{
    // Allow login after official server shutdown.
    if(!NopRegion(data, 0x00B35F17, 2))
    {
        return false;
    }

    // Infinite zoom.
    if(!NopRegion(data, 0x0021ADA4, 15))
    {
        return false;
    }

    // Patch the client data encryption key.
    if(!PatchBlowfish(data))
    {
        return false;
    }

    // Patch the DH prime.
    memcpy(&data[0x150AD68], DH_PRIME, 256);

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
    std::vector<char> data = libcomp::Decrypt::LoadFile(szInPath);

    if(data.empty())
    {
        std::cerr << "Failed to open input file '" << szInPath
            << "'." << std::endl;

        return EXIT_FAILURE;
    }

    // Make sure the client was not modified.
    libcomp::String hash = libcomp::Decrypt::SHA1(data);

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
