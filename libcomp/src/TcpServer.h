/**
 * @file libcomp/src/TcpServer.h
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

#ifndef LIBCOMP_SRC_TCPSERVER_H
#define LIBCOMP_SRC_TCPSERVER_H

// libcomp Includes
#include "CString.h"
#include "ServerConfig.h"

// Boost ASIO Includes
#include "PushIgnore.h"
#include <asio.hpp>
#include "PopIgnore.h"

// Standard C++ Includes
#include <memory>
#include <thread>

// OpenSSL Includes
#include <openssl/dh.h>

namespace libcomp
{

class TcpConnection;

class TcpServer
{
public:
    TcpServer(const String& listenAddress, uint16_t port);
    virtual ~TcpServer();

    virtual int Start();

    bool ReadConfig(objects::ServerConfig* config, std::string filename);
    virtual bool ReadConfig(objects::ServerConfig* config, tinyxml2::XMLDocument& doc);

    static DH* GenerateDiffieHellman();
    static DH* LoadDiffieHellman(const String& prime);
    static DH* LoadDiffieHellman(const std::vector<char>& data);
    static DH* LoadDiffieHellman(const void *pData, size_t dataSize);
    static std::vector<char> SaveDiffieHellman(const DH *pDiffieHellman);
    static DH* CopyDiffieHellman(const DH *pDiffieHellman);

protected:
    virtual int Run();

    virtual std::shared_ptr<TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    const DH* GetDiffieHellman() const;
    void SetDiffieHellman(DH *pDiffieHellman);

    void AcceptHandler(asio::error_code errorCode,
        asio::ip::tcp::socket& socket);

    std::list<std::shared_ptr<TcpConnection>> mConnections;

    asio::io_service mService;

private:
    asio::ip::tcp::acceptor mAcceptor;

    std::thread mServiceThread;

    DH *mDiffieHellman;

    String mListenAddress;
    uint16_t mPort;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_TCPSERVER_H
