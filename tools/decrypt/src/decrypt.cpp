/**
 * @file tools/decrypt/src/decrypt.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Built-in configuration settings.
 *
 * This tool will decrypt a Blowfish encrypted file.
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

#include <Crypto.h>

#include <fstream>
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
    if(3 != argc)
    {
        std::cerr << "USAGE: " << argv[0] << " IN OUT" << std::endl;

        return EXIT_FAILURE;
    }

    std::vector<char> data = libcomp::Crypto::LoadFile(argv[1]);

    if(data.empty())
    {
        std::cerr << "Failed to load input file." << std::endl;

        return EXIT_FAILURE;
    }

    if(!libcomp::Crypto::DecryptFile(data))
    {
        std::cerr << "Failed to decrypt file." << std::endl;

        return EXIT_FAILURE;
    }

    std::ofstream out;
    out.open(argv[2], std::ofstream::out | std::ofstream::binary);
    out.write(&data[0], static_cast<std::streamsize>(data.size()));

    if(!out.good())
    {
        std::cerr << "Failed to write output file." << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
