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

/**
 * Base class for all servers that run workers to handle
 * incoming messages in the message queue.  Each of these
 * servers is instantiated via a dedicated config file and
 * are responsible for choosing which of the workers it
 * manages will be assigned to each incoming connection.
 */
class BaseServer : public TcpServer
{
public:
    /**
     * Create a new base server.
     * @param config Pointer to a config container that will hold properties
     *   every server has in common.
     * @param configPath File path to the location of the config to be loaded.
     */
    BaseServer(std::shared_ptr<objects::ServerConfig> config, const String& configPath);

    /**
     * Clean up the server workers and send out the the server shutdown message.
     */
    virtual ~BaseServer();

    /**
     * Initialize the database connection and do anything else that can fail
     * to execute that needs to be handled outside of a constructor.
     * @param self Pointer to this server to be used as a reference in
     *   packet handling code.
     * @return true on success, false on failure
     */
    virtual bool Initialize(std::weak_ptr<BaseServer>& self);

    /**
     * Call the Shutdown function on each worker.  This should be called
     * only before preparing to stop the application.
     */
    virtual void Shutdown();

    /**
     * Get the OS specific default path to look for config files.
     * @return The default path to a config folder
     */
    static std::string GetDefaultConfigPath();
    
    /**
     * Read the config file from the path and parse into an XML document
     * to be read in the virtual ReadConfig function.  Messages will print
     * to the console on both success and failure.
     * @param config Pointer to a config file that will contain properties
     *   every server has in common.
     * @param configPath File path to the location of the config to be loaded.
     * @return true on success, false on failure
     */
    bool ReadConfig(std::shared_ptr<objects::ServerConfig> config, libcomp::String filePath);

    /**
     * Read the config file values from an XML document and populates the
     * config passed in.  Server specific configs should override this and
     * implement their own reading logic but also call this base function
     * to get the shared ServerConfig values.
     * @param config Pointer to a config file that will contain properties
     *   every server has in common.
     * @param doc XML file containing the config values.
     * @return true on success, false on failure
     */
    virtual bool ReadConfig(std::shared_ptr<objects::ServerConfig> config, tinyxml2::XMLDocument& doc);
    
    /**
     * Get the server config file read during the constructor steps.
     * @return Pointer to the server config
     */
    std::shared_ptr<objects::ServerConfig> GetConfig() const;

protected:
    /**
     * Runs the server until a shutdown message is received or the program
     * is forcefully closed.
     * @return 0 on success, 1 on failure
     */
    virtual int Run();
    
    /**
     * Create one or many workers to handle connection requests based upon
     * the server config allowing mutliple workers as well as how many cores
     * are available on the executing machine's CPU.
     */
    void CreateWorkers();
    
    /**
     * Retrieve and assign a message queue to use for a new connection.
     * The method of deciding which worker to use is not contained in this
     * function but the actual assignment and starting of workers not yet
     * running is handled.
     * @return true on success, false on failure
     */
    bool AssignMessageQueue(std::shared_ptr<libcomp::EncryptedConnection>& connection);
    
    /**
     * Get the next worker to use for new connections.  This implementation
     * uses a "least busy" method to decide which worker to assign but it is
     * meant to be overridden by anything with a different method of assignment.
     * @return Pointer to the worker whose message queue should be assigned
     *  to a new connection
     */
    virtual std::shared_ptr<libcomp::Worker> GetNextConnectionWorker();

    /// A shared pointer to the server.
    std::weak_ptr<BaseServer> mSelf;

    /// A shared pointer to the config used to set up the server.
    std::shared_ptr<objects::ServerConfig> mConfig;

    /// A shared pointer to the database used by the server.
    std::shared_ptr<libcomp::Database> mDatabase;

    /// Worker that blocks and runs in the main thread.
    libcomp::Worker mMainWorker;

    /// List of workers to handle incoming connection packet based work.
    std::list<std::shared_ptr<libcomp::Worker>> mWorkers;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_BASESERVER_H
