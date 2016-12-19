/**
 * @file libcomp/src/Decrypt.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Encryption and decryption function definitions.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#ifndef LIBCOMP_SRC_DECRYPT_H
#define LIBCOMP_SRC_DECRYPT_H

/// libcomp Includes
#include "CString.h"

#include "PushIgnore.h"
#include <openssl/blowfish.h>
#include "PopIgnore.h"

#include <stdint.h>

#include <string>
#include <vector>

namespace libcomp
{

/// Size (in bytes) of a block of Blowfish encrypted data.
const std::size_t BLOWFISH_BLOCK_SIZE = sizeof(uint64_t);

class Packet;

/**
 * Encryption and Decryption functions.
 */
namespace Decrypt
{

/**
 * @brief Decrypt a file into a buffer.
 * @param path Path to the file to be decrypted.
 * @returns Buffer of the decrypted file. Will be empty if an error occurred.
 * @sa Decrypt::EncryptFile
 * @sa Config::ENCRYPTED_FILE_MAGIC
 * @sa Config::ENCRYPTED_FILE_KEY
 * @sa Config::ENCRYPTED_FILE_IV
 */
std::vector<char> DecryptFile(const std::string& path);

/**
 * Encrypt a file from a buffer.
 * @param Path to the file to be written to.
 * @param data Data to be encrypted.
 * @retval true File was encrypted.
 * @retval false File was not encrypted.
 * @sa Decrypt::DecryptFile
 * @sa Config::ENCRYPTED_FILE_MAGIC
 * @sa Config::ENCRYPTED_FILE_KEY
 * @sa Config::ENCRYPTED_FILE_IV
 */
bool EncryptFile(const std::string& path, const std::vector<char>& data);

/**
 * Load a file into a buffer
 * @param path Path to the file to be loaded.
 * @param requestedSize Number of bytes to read from the file. If this is
 *   negative, the entire file will be read.
 * @returns The loaded file data or an empty vector if an error occured.
 */
std::vector<char> LoadFile(const std::string& path, int requestedSize = -1);

/**
 * Generates random data to be used during key exchange. The returned data will
 * be a series of hex digits.
 *
 * @param sz Number of digits of random data to generate. If -1 is used, the
 *   default number of digits will be generated (80 digits).
 * @returns The random data encoded as a base-16 string or an empty string if
 *   an error occured.
 */
String GenerateRandom(int sz = -1);

/**
 * Generates a random value. This value is used to identify a login session
 * when passing an authenticated user from the lobby server to the
 * channel server.
 *
 * @returns Random value to identify a transitioning client or 0 if an error
 *   occured.
 */
uint32_t GenerateSessionKey();

/**
 * Generate the result of the operation g^a % p
 * @param g Base number represented as a base-16 string.
 * @param p Prime number represented as a base-16 string.
 * @param a Secret number represented as a base-16 string.
 * @param outputSize Number of characters to output. Smaller values will be
 *   padded with zeros. Any value of 0 or less will disable output padding.
 * @returns A base-16 string representing the operation g^a % p or an empty
 *   string if an error occured.
 */
String GenDiffieHellman(const String& g, const String& p,
    const String& a, int outputSize = -1);

/**
 * Encrypt a data buffer with Blowfish.
 * @param key Blowfish key to encrypt with.
 * @param pData Data to be encrypted.
 * @param dataSize Size of the data to be encrypted (in bytes).
 * @note The data size should be a multiple of BLOWFISH_BLOCK_SIZE.
 */
void Encrypt(const BF_KEY& key, void *pData, uint32_t dataSize);

/**
 * Encrypt a data buffer with Blowfish.
 * @param key Blowfish key to encrypt with.
 * @param data Data to be encrypted. The size may change on return.
 */
void Encrypt(const BF_KEY& key, std::vector<char>& data);

/**
 * Encrypt a data buffer with the default Blowfish key.
 * @param data Data to be encrypted. The size may change on return.
 * @sa Config::ENCRYPTED_FILE_KEY
 */
void Encrypt(std::vector<char>& data);

/**
 * Decrypt a data buffer with Blowfish.
 * @param key Blowfish key to decrypt with.
 * @param pData Data to be decrypted.
 * @param dataSize Size of the data to be decrypted (in bytes).
 * @note The data size should be a multiple of BLOWFISH_BLOCK_SIZE.
 */
void Decrypt(const BF_KEY& key, void *pData, uint32_t dataSize);

/**
 * Decrypt a data buffer with Blowfish.
 * @param key Blowfish key to decrypt with.
 * @param data Data to be decrypted.
 * @param realSize Size to shrink the buffer to after decryption.
 */
void Decrypt(const BF_KEY& key, std::vector<char>& data,
    std::vector<char>::size_type realSize = 0);

/**
 * Decrypt a data buffer with the default Blowfish key.
 * @param data Data to be decrypted.
 * @param realSize Size to shrink the buffer to after decryption.
 * @sa Config::ENCRYPTED_FILE_KEY
 */
void Decrypt(std::vector<char>& data,
    std::vector<char>::size_type realSize = 0);

/**
 * Encrypt a data buffer with Blowfish and Cipher Block Chaining (CBC).
 * @param key Blowfish key to encrypt with.
 * @param initializationVector Initial value to feed into the CBC algorithm.
 * @param data Data to be encrypted. The size may change on return.
 */
void EncryptCbc(const BF_KEY& key, uint64_t& initializationVector,
    std::vector<char>& data);

/**
 * Encrypt a data buffer with the default Blowfish key and Cipher Block
 * Chaining (CBC) initialization vector (IV).
 *
 * @param data Data to be encrypted. The size may change on return.
 * @sa Config::ENCRYPTED_FILE_KEY
 * @sa Config::ENCRYPTED_FILE_IV
 */
void EncryptCbc(std::vector<char>& data);

/**
 * Decrypt a data buffer with Blowfish and Cipher Block Chaining (CBC).
 * @param key Blowfish key to decrypt with.
 * @param initializationVector Initial value to feed into the CBC algorithm.
 * @param data Data to be decrypted.
 * @param realSize Size to shrink the buffer to after decryption.
 */
void DecryptCbc(const BF_KEY& key, uint64_t& initializationVector,
    std::vector<char>& data, std::vector<char>::size_type realSize = 0);

/**
 * Decrypt a data buffer with the default Blowfish key and Cipher Block
 * Chaining (CBC) initialization vector (IV).
 *
 * @param data Data to be decrypted.
 * @param realSize Size to shrink the buffer to after decryption.
 * @sa Config::ENCRYPTED_FILE_KEY
 * @sa Config::ENCRYPTED_FILE_IV
 */
void DecryptCbc(std::vector<char>& data,
    std::vector<char>::size_type realSize = 0);

/**
 * Encrypt a packet.
 * @param key The blowfish key to use.
 * @param p The packet to encrypt.
 */
void EncryptPacket(const BF_KEY& key, Packet& p);

/**
 * Decrypt a packet.
 * @param key The blowfish key to use.
 * @param p The packet to decrypt.
 */
void DecryptPacket(const BF_KEY& key, Packet& p);

/**
 * Generate a password hash.
 * @param password Clear text password to hash.
 * @param salt Salt to append to the password before hashing.
 * @returns Hash for the given password.
 */
String HashPassword(const String& password, const String& salt);

} // namespace Decrypt

} // namespace libcomp

#endif // LIBCOMP_SRC_DECRYPT_H
