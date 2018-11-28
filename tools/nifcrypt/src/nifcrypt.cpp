/**
 * @file tools/nifcrypt/src/nifcrypt.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to encrypt and decrypt (and decompress) NIF files.
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
#include <cstring>
#include <stdint.h>
#include <string>

// libcomp Includes
#include <Compress.h>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Edit these to match if you need to work with the original magic and key!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// This is not the magic and encryption key used by the client because it
// might be considered a copyrighted sequence. Replace them if you want to
// get this to work with the original client files.

// These are inside ImagineClient.exe (version 1.666) so look there for the
// value to replace them with. You could use something like HxD to do this.

// 32-bit little endian value @ 0x71966A
#define NIF_XOR_KEY1 (0x1337C0DE)
// 32-bit little endian value @ 0x719681
#define NIF_XOR_KEY2 (0x8BADF00D)
// Value @ 0x71947E
#define NIF_MAGIC1 (0xEF)
// Value @ 0x719485
#define NIF_MAGIC2 (0xBE)
// Value @ 0x71948C
#define NIF_MAGIC3 (0xAD)
// Value @ 0x719493
#define NIF_MAGIC4 (0xDE)

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DO NOT EDIT BELOW THIS LINE!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Combine the magic into a single DWORD.
#define NIF_MAGIC (((uint32_t)NIF_MAGIC4 << 24) | \
    ((uint32_t)NIF_MAGIC3 << 16) | \
    ((uint32_t)NIF_MAGIC2 << 8) | \
     (uint32_t)NIF_MAGIC1)

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

    // Get the size of the file.
    uint32_t sz = (uint32_t)ftell(pIn);

    // Check the size of the file.
    if(sz > 30000000)
    {
        fprintf(stderr, "Input file is too big!\n");

        return EXIT_FAILURE;
    }

    // Start over now that we have the size.
    if(0 != fseek(pIn, 0, SEEK_SET))
    {
        fprintf(stderr, "Seek error in input file.\n");

        return EXIT_FAILURE;
    }

    // Allocate the buffer.
    uint8_t *pData = new uint8_t[sz];
    memset(pData, 0, sz);

    // Calculate the max size of the compressed data.
    int32_t maxSize = (int)((float)sz * 0.001f + 0.5f);
    maxSize += (int32_t)sz + 12;

    // Allocate the output buffer.
    uint8_t *pDataOut = new uint8_t[(uint32_t)maxSize];

    // Read the file.
    if(1 != fread(pData, sz, 1, pIn))
    {
        fprintf(stderr, "Failed to read input file.\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Attempt to compress the file.
    int32_t compSize = libcomp::Compress::Compress(pData, pDataOut,
        (int32_t)sz, maxSize, 9);

    // Check if it compressed.
    if(0 >= compSize)
    {
        fprintf(stderr, "Failed to compress NIF file!\n");
        fprintf(stderr, "Size: %d\n", compSize);

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Check the size of the file.
    if(compSize > 30000000)
    {
        fprintf(stderr, "Input file is too big!\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Sanity check here.
    if(compSize > (int32_t)sz)
    {
        fprintf(stderr, "Input file is too big!\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Encrypt the sizes.
    uint32_t cryptDecompSize = sz ^ NIF_XOR_KEY1;
    int32_t cryptCompSize = compSize ^ (int32_t)NIF_XOR_KEY2;

    // Write the magic.
    uint32_t magic = NIF_MAGIC;

    if(1 != fwrite(&magic, 4, 1, pOut))
    {
        fprintf(stderr, "Failed to write output file.\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Write the uncompressed size.
    if(1 != fwrite(&cryptDecompSize, 4, 1, pOut))
    {
        fprintf(stderr, "Failed to write output file.\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Write the compressed size.
    if(1 != fwrite(&cryptCompSize, 4, 1, pOut))
    {
        fprintf(stderr, "Failed to write output file.\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Write the compressed data.
    if(1 != fwrite(pDataOut, (size_t)compSize, 1, pOut))
    {
        fprintf(stderr, "Failed to write output file.\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Free the input and output buffers.
    delete[] pData;
    delete[] pDataOut;
    pDataOut = 0;
    pData = 0;

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

        return EXIT_FAILURE;
    }

    if(NIF_MAGIC != magic)
    {
        fprintf(stderr, "ERROR: File is not encrypted!\n");

        return EXIT_FAILURE;
    }

    // Get the decompressed size.
    uint32_t decompSize = 0;

    if(1 != fread(&decompSize, 4, 1, pIn))
    {
        fprintf(stderr, "Failed to read input file.\n");

        return EXIT_FAILURE;
    }

    // Decrypt the size and check it.
    decompSize ^= NIF_XOR_KEY1;

    if(decompSize > 30000000)
    {
        fprintf(stderr, "Input file is too big!\n");

        return EXIT_FAILURE;
    }

    // Get the compressed size.
    uint32_t compSize = 0;

    if(1 != fread(&compSize, 4, 1, pIn))
    {
        fprintf(stderr, "Failed to read input file.\n");

        return EXIT_FAILURE;
    }

    if(0 != fseek(pIn, 0, SEEK_END))
    {
        fprintf(stderr, "Seek error in input file.\n");

        return EXIT_FAILURE;
    }

    // Get the size of the file.
    uint32_t sz = (uint32_t)ftell(pIn);

    // We don't read the magic, size and key as part of the file.
    sz -= 4 * 3;

    // Check the size of the file.
    if(sz > 30000000)
    {
        fprintf(stderr, "Input file is too big!\n");

        return EXIT_FAILURE;
    }

    // Decrypt and check the compressed size.
    compSize ^= NIF_XOR_KEY2;

    if(compSize > sz)
    {
        fprintf(stderr, "Input file is too big!\n");

        return EXIT_FAILURE;
    }

    // Start over now that we have the key.
    if(0 != fseek(pIn, 4 * 3, SEEK_SET))
    {
        fprintf(stderr, "Seek error in input file.\n");

        return EXIT_FAILURE;
    }

    // Allocate the buffer.
    uint8_t *pData = new uint8_t[compSize];
    memset(pData, 0, compSize);

    // Allocate the output buffer.
    uint8_t *pDataOut = new uint8_t[decompSize];

    // Read the file.
    if(1 != fread(pData, compSize, 1, pIn))
    {
        fprintf(stderr, "Failed to read input file.\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Attempt to decompress the file.
    if((int32_t)decompSize != libcomp::Compress::Decompress(pData, pDataOut,
        (int32_t)compSize, (int32_t)decompSize))
    {
        fprintf(stderr, "Failed to decompress NIF file!\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Write the output file contents.
    if(1 != fwrite(pDataOut, decompSize, 1, pOut))
    {
        fprintf(stderr, "Failed to write output file.\n");

        // Free the input and output buffers.
        delete[] pData;
        delete[] pDataOut;
        pDataOut = 0;
        pData = 0;

        return EXIT_FAILURE;
    }

    // Free the input and output buffers.
    delete[] pData;
    delete[] pDataOut;
    pDataOut = 0;
    pData = 0;

    // Close the files and cleanup.
    fclose(pIn);
    fclose(pOut);

    return EXIT_SUCCESS;
}

/**
 * Encrypt or decrypt a NIF file.
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
