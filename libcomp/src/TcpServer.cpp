/**
 * @file libcomp/src/TcpServer.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base TCP/IP server class.
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

#include "TcpServer.h"

#include "Constants.h"
#include "Log.h"
#include "TcpConnection.h"

using namespace libcomp;

TcpServer::TcpServer(const String& listenAddress, uint16_t port) :
    mAcceptor(mService), mDiffieHellman(nullptr),
    mListenAddress(listenAddress), mPort(port)
{
}

TcpServer::~TcpServer()
{
    if(nullptr != mDiffieHellman)
    {
        DH_free(mDiffieHellman);
    }
}

int TcpServer::Start()
{
    // Check for a DH key pair.
    if(nullptr == mDiffieHellman)
    {
        LOG_WARNING("Generating a DH key pair. "
            "This could take several minutes.\n");

        // Generate it since we don't have one yet.
        mDiffieHellman = GenerateDiffieHellman();

        // Check if the key was made.
        if(nullptr == mDiffieHellman)
        {
            LOG_CRITICAL("Failed to generate Diffie-Hellman prime!\n");
        }
        else
        {
            LOG_WARNING(String("Please add the following to your "
                "configuration XML: <prime>%1</prime>\n").Arg(
                    TcpConnection::GetDiffieHellmanPrime(mDiffieHellman)
                )
            );
        }
    }

    asio::ip::tcp::endpoint endpoint;

    if(mListenAddress.IsEmpty() || "any" == mListenAddress.ToLower())
    {
        endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), mPort);
    }
    else
    {
        endpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(
            mListenAddress.ToUtf8()), mPort);
    }

    mAcceptor.open(endpoint.protocol());
    mAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    mAcceptor.bind(endpoint);
    mAcceptor.listen();

    asio::ip::tcp::socket socket(mService);

    mAcceptor.async_accept(socket,
        [this, &socket](asio::error_code errorCode)
        {
            AcceptHandler(errorCode, socket);
        });

    mServiceThread = std::thread([this]()
    {
        mService.run();
    });

    ServerReady();

    int returnCode = Run();

    mServiceThread.join();

    return returnCode;
}

void TcpServer::RemoveConnection(std::shared_ptr<TcpConnection>& connection)
{
    auto iter = std::find(mConnections.begin(), mConnections.end(), connection);
    if(iter != mConnections.end())
    {
        mConnections.remove(connection);
    }
}

int TcpServer::Run()
{
    return 0;
}

void TcpServer::ServerReady()
{
    LOG_INFO("Server ready!\n");
}

std::shared_ptr<TcpConnection> TcpServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    return std::make_shared<TcpConnection>(socket,
        CopyDiffieHellman(mDiffieHellman));
}

void TcpServer::AcceptHandler(asio::error_code errorCode,
    asio::ip::tcp::socket& socket)
{
    if(errorCode)
    {
        LOG_ERROR(String("async_accept error: %1\n").Arg(errorCode.message()));
    }
    else
    {
        // Make sure the DH key pair is valid.
        if(nullptr != mDiffieHellman)
        {
            LOG_DEBUG(String("New connection from %1\n").Arg(
                socket.remote_endpoint().address().to_string()));

            auto connection = CreateConnection(socket);
            if(nullptr == connection)
            {
                LOG_CRITICAL("The connection could not be created\n");
                return;
            }

            mConnections.push_back(connection);

            // This is actually using a different socket because the
            // CreateConnection() call will use std::move on the socket which
            // will reset this socket (which is a reference to the local
            // variable in the Start() function).
            mAcceptor.async_accept(socket,
                [this, &socket](asio::error_code acceptErrorCode)
                {
                    AcceptHandler(acceptErrorCode, socket);
                });
        }
        else
        {
            LOG_CRITICAL("Somehow you got this far without a DH key pair!\n");
        }
    }
}

const DH* TcpServer::GetDiffieHellman() const
{
    return mDiffieHellman;
}

void TcpServer::SetDiffieHellman(DH *pDiffieHellman)
{
    if(nullptr != mDiffieHellman)
    {
        DH_free(pDiffieHellman);
    }

    mDiffieHellman = pDiffieHellman;
}

DH* TcpServer::GenerateDiffieHellman()
{
    int codes;

    DH *pDiffieHellman = DH_new();

    if(nullptr != pDiffieHellman)
    {
        if(1 != DH_generate_parameters_ex(pDiffieHellman, DH_KEY_BIT_SIZE,
            DH_BASE_INT, NULL) || nullptr == pDiffieHellman->p ||
            nullptr == pDiffieHellman->g ||
            1 != DH_check(pDiffieHellman, &codes) ||
            DH_SHARED_DATA_SIZE != DH_size(pDiffieHellman))
        {
            DH_free(pDiffieHellman);
            pDiffieHellman = nullptr;
        }
    }

    return pDiffieHellman;
}

DH* TcpServer::LoadDiffieHellman(const String& prime)
{
    DH *pDiffieHellman = nullptr;

    if(DH_KEY_HEX_SIZE == prime.Length())
    {
        pDiffieHellman = DH_new();

        if(nullptr != pDiffieHellman)
        {
            if(0 >= BN_hex2bn(&pDiffieHellman->p, prime.C()) ||
                0 >= BN_hex2bn(&pDiffieHellman->g, DH_BASE_STRING) ||
                nullptr == pDiffieHellman->p || nullptr == pDiffieHellman->g ||
                DH_SHARED_DATA_SIZE != DH_size(pDiffieHellman))
            {
                DH_free(pDiffieHellman);
                pDiffieHellman = nullptr;
            }
        }
    }

    return pDiffieHellman;
}

DH* TcpServer::LoadDiffieHellman(const std::vector<char>& data)
{
    return LoadDiffieHellman(&data[0], data.size());
}

DH* TcpServer::LoadDiffieHellman(const void *pData, size_t dataSize)
{
    DH *pDiffieHellman = nullptr;

    if(nullptr != pData && DH_SHARED_DATA_SIZE == dataSize)
    {
        pDiffieHellman = DH_new();

        if(nullptr != pDiffieHellman)
        {
            pDiffieHellman->p = BN_bin2bn(reinterpret_cast<
                const unsigned char*>(pData),
                static_cast<int>(dataSize), NULL);

            if(0 >= BN_hex2bn(&pDiffieHellman->g, DH_BASE_STRING) ||
                nullptr == pDiffieHellman->p || nullptr == pDiffieHellman->g ||
                DH_SHARED_DATA_SIZE != DH_size(pDiffieHellman))
            {
                DH_free(pDiffieHellman);
                pDiffieHellman = nullptr;
            }
        }
    }

    return pDiffieHellman;
}

std::vector<char> TcpServer::SaveDiffieHellman(const DH *pDiffieHellman)
{
    std::vector<char> data;

    unsigned char primeData[DH_SHARED_DATA_SIZE];

    if(nullptr != pDiffieHellman && nullptr != pDiffieHellman->p &&
        DH_SHARED_DATA_SIZE == BN_num_bytes(pDiffieHellman->p) &&
        DH_SHARED_DATA_SIZE == BN_bn2bin(pDiffieHellman->p, primeData))
    {
        data.insert(data.begin(), reinterpret_cast<const char*>(primeData),
            reinterpret_cast<const char*>(primeData) + DH_SHARED_DATA_SIZE);
    }

    return data;
}

DH* TcpServer::CopyDiffieHellman(const DH *pDiffieHellman)
{
    DH *pCopy = nullptr;

    if(nullptr != pDiffieHellman && nullptr != pDiffieHellman->p &&
        nullptr != pDiffieHellman->g)
    {
        pCopy = DH_new();

        if(nullptr != pCopy)
        {
            pCopy->p = BN_dup(pDiffieHellman->p);
            pCopy->g = BN_dup(pDiffieHellman->g);

            if(nullptr == pCopy->p || nullptr == pCopy->g)
            {
                DH_free(pCopy);
                pCopy = nullptr;
            }
        }
    }

    return pCopy;
}
