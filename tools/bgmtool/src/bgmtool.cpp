/**
 * @file tools/bgmtool/src/bgmtool.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to encrypt and decrypt background music files.
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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <string>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Edit these to match if you need to work with the original magic and key!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// This is not the magic and encryption key used by the client because it
// might be considered a copyrighted sequence. Replace them if you want to
// get this to work with the original client files.

// These are inside ImagineClient.exe (version 1.666) so look there for the
// value to replace them with. You could use something like HxD to do this.

// 32-bit little endian value @ 0x93AC11
#define BGM_XOR_KEY (0x1337C0DE)
// Value @ 0x93ABDE
#define BGM_MAGIC1 (0xEF)
// Value @ 0x93ABE3
#define BGM_MAGIC2 (0xBE)
// Value @ 0x93ABEA
#define BGM_MAGIC3 (0xAD)
// Value @ 0x93ABF1
#define BGM_MAGIC4 (0xDE)

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DO NOT EDIT BELOW THIS LINE!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Combine the magic into a single DWORD.
#define BGM_MAGIC (((uint32_t)BGM_MAGIC4 << 24) | \
    ((uint32_t)BGM_MAGIC3 << 16) | \
    ((uint32_t)BGM_MAGIC2 << 8) | \
     (uint32_t)BGM_MAGIC1)

/**
 * Encrypt a background music file.
 * @param szIn Path to the input file to encrypt.
 * @param szOut Path to the output file to write.
 * @returns Standard exit code.
 */
int EncryptFile(const char *szIn, const char *szOut)
{
    FILE *pIn = fopen(szIn, "rb");
    FILE *pOut = fopen(szOut, "wb");

    if(!pIn)
    {
        fprintf(stderr, "Failed to open input file.\n");

        return EXIT_FAILURE;
    }

    if(!pOut)
    {
        fprintf(stderr, "Failed to open input file.\n");

        return EXIT_FAILURE;
    }

    if(0 != fseek(pIn, 0, SEEK_END))
    {
        fprintf(stderr, "Seek error in input file.\n");

        return EXIT_FAILURE;
    }

    // Get the size of the file and calculate the XOR key.
    uint32_t sz = (uint32_t)ftell(pIn);
    uint32_t key = sz ^ BGM_XOR_KEY;

    // Start over now that we have the key
    if(0 != fseek(pIn, 0, SEEK_SET))
    {
        fprintf(stderr, "Seek error in input file.\n");

        return EXIT_FAILURE;
    }

    // Write the magic.
    uint32_t magic = BGM_MAGIC;

    if(1 != fwrite(&magic, 4, 1, pOut))
    {
        fprintf(stderr, "Failed to write output file.\n");

        return EXIT_FAILURE;
    }

    // Write the output file contents.
    while(sz)
    {
        uint32_t buffer = 0;
        uint32_t chunk = std::min((uint32_t)4, sz);

        if(1 != fread(&buffer, chunk, 1, pIn))
        {
            fprintf(stderr, "Failed to read input file.\n");

            return EXIT_FAILURE;
        }

        // Encrypt the dword.
        buffer ^= key;

        if(1 != fwrite(&buffer, chunk, 1, pOut))
        {
            fprintf(stderr, "Failed to write output file.\n");

            return EXIT_FAILURE;
        }

        sz -= chunk;
    }

    // Close the files and cleanup.
    fclose(pIn);
    fclose(pOut);

    return EXIT_SUCCESS;
}

/**
 * Decrypt a background music file.
 * @param szIn Path to the input file to decrypt.
 * @param szOut Path to the output file to write.
 * @returns Standard exit code.
 */
int DecryptFile(const char *szIn, const char *szOut)
{
    FILE *pIn = fopen(szIn, "rb");
    FILE *pOut = fopen(szOut, "wb");

    if(!pIn)
    {
        fprintf(stderr, "Failed to open input file.\n");

        return EXIT_FAILURE;
    }

    if(!pOut)
    {
        fprintf(stderr, "Failed to open input file.\n");

        return EXIT_FAILURE;
    }

    // Read and check the magic is correct.
    uint32_t magic = 0;

    if(1 != fread(&magic, 4, 1, pIn))
    {
        fprintf(stderr, "Failed to read input file.\n");
    }

    if(BGM_MAGIC != magic)
    {
        fprintf(stderr, "ERROR: File is not encrypted!\n");

        return EXIT_FAILURE;
    }

    if(0 != fseek(pIn, 0, SEEK_END))
    {
        fprintf(stderr, "Seek error in input file.\n");

        return EXIT_FAILURE;
    }

    // Get the size of the file and calculate the XOR key.
    uint32_t sz = (uint32_t)ftell(pIn);
    uint32_t key = (sz - 4) ^ BGM_XOR_KEY;

    // Start over now that we have the key
    if(0 != fseek(pIn, 4, SEEK_SET))
    {
        fprintf(stderr, "Seek error in input file.\n");

        return EXIT_FAILURE;
    }

    // We don't read the magic as part of the file.
    sz -= 4;

    // Write the output file contents.
    while(sz)
    {
        uint32_t buffer = 0;
        uint32_t chunk = std::min((uint32_t)4, sz);

        if(1 != fread(&buffer, chunk, 1, pIn))
        {
            fprintf(stderr, "Failed to read input file.\n");

            return EXIT_FAILURE;
        }

        // Encrypt the dword.
        buffer ^= key;

        if(1 != fwrite(&buffer, chunk, 1, pOut))
        {
            fprintf(stderr, "Failed to write output file.\n");

            return EXIT_FAILURE;
        }

        sz -= chunk;
    }

    // Close the files and cleanup.
    fclose(pIn);
    fclose(pOut);

    return EXIT_SUCCESS;
}

/**
 * Encrypt or decrypt a background music file.
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @returns Standard exit code.
 */
int main(int argc, char *argv[])
{
    // Detect encrypt or decrypt mode or print usage.
    if(argc == 4 && std::string("-d") == argv[1])
    {
        return DecryptFile(argv[2], argv[3]);
    }
    else if(argc == 3)
    {
        return EncryptFile(argv[1], argv[2]);
    }
    else
    {
        fprintf(stderr, "USAGE: %s [-d] IN OUT\n", argv[0]);
    }

    return EXIT_FAILURE;
}
