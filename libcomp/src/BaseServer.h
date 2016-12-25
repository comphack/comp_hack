/**
 * @file libcomp/src/BaseServer.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base server class.
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

#ifndef LIBCOMP_SRC_BASESERVER_H
#define LIBCOMP_SRC_BASESERVER_H

// libcomp Includes
#include "Database.h"
#include "EncryptedConnection.h"
#include "ServerConfig.h"
#include "TcpServer.h"
#include "Worker.h"

namespace libcomp
{

class BaseServer : public TcpServer
{
public:
    BaseServer(std::shared_ptr<objects::ServerConfig> config, const String& configPath);
    virtual ~BaseServer();

    virtual bool Initialize(std::weak_ptr<BaseServer>& self);

    virtual void Shutdown();

    static std::string GetDefaultConfigPath();
    bool ReadConfig(std::shared_ptr<objects::ServerConfig> config, libcomp::String filePath);
    virtual bool ReadConfig(std::shared_ptr<objects::ServerConfig> config, tinyxml2::XMLDocument& doc);

protected:
    virtual int Run();

    void CreateWorkers();

    bool AssignMessageQueue(std::shared_ptr<libcomp::EncryptedConnection>& connection);

    virtual std::shared_ptr<libcomp::Worker> GetNextConnectionWorker();

    std::weak_ptr<BaseServer> mSelf;

    std::shared_ptr<objects::ServerConfig> mConfig;

    std::shared_ptr<libcomp::Database> mDatabase;

    /// Worker that blocks and runs in the main thread.
    libcomp::Worker mMainWorker;

    std::list<std::shared_ptr<libcomp::Worker>> mWorkers;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_BASESERVER_H
