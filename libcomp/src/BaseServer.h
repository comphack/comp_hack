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

// Standard C++14 Includes
#include <gsl/gsl>

// libcomp Includes
#include "Database.h"
#include "DatabaseConfig.h"
#include "DataStore.h"
#include "EncryptedConnection.h"
#include "ServerConfig.h"
#include "TcpServer.h"
#include "Worker.h"

namespace libcomp
{

class ServerCommandLineParser;

/**
 * Base class for all servers that run workers to handle
 * incoming messages in the message queue.  Each of these
 * servers is instantiated via a dedicated config file and
 * are responsible for choosing which of the workers it
 * manages will be assigned to each incoming connection.
 */
class BaseServer : public TcpServer, public Manager,
    public std::enable_shared_from_this<BaseServer>
{
public:
    /**
     * Create a new base server.
     * @param szProgram First command line argument for the application.
     * @param config Pointer to a config container that will hold properties
     *   every server has in common.
     */
    BaseServer(const char *szProgram,
        std::shared_ptr<objects::ServerConfig> config,
        std::shared_ptr<ServerCommandLineParser> commandLine);

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
    virtual bool Initialize();

    /**
     * Do any initialize that should happen after the server is listening and
     * fully started.
     */
    virtual void FinishInitialize();

    /**
     * Get the different types of messages handled by this manager.
     * @return List of supported message types
     */
    virtual std::list<libcomp::Message::MessageType> GetSupportedTypes() const;

    /**
     * Process a message from the queue.
     * @param pMessage Message to be processed
     * @return true on success, false on failure
     */
    virtual bool ProcessMessage(const libcomp::Message::Message *pMessage);

    /**
     * Get an open database connection of the database type associated to the
     * server.
     * @param dbType Database type to use.
     * @param configMap Map of the available database configs by type
     * @return Pointer to the new database connection or nullptr on failure
     */
    static std::shared_ptr<Database> GetDatabase(
        objects::ServerConfig::DatabaseType_t dbType,
        const EnumMap<objects::ServerConfig::DatabaseType_t,
            std::shared_ptr<objects::DatabaseConfig>>& configMap);

    /**
     * Get an open database connection of the database type associated to the
     * server.
     * @param configMap Map of the available database configs by type
     * @param performSetup If true, @ref Database::Setup will be executed
     * @return Pointer to the new database connection or nullptr on failure
     */
    std::shared_ptr<Database> GetDatabase(
        const EnumMap<objects::ServerConfig::DatabaseType_t,
            std::shared_ptr<objects::DatabaseConfig>>& configMap, bool performSetup);

    /**
     * Get the data store for the server.
     * @returns Pointer to the data store. This shold never be deleted.
     */
    gsl::not_null<DataStore*> GetDataStore();

    /**
     * Call the Shutdown function on each worker.  This should be called
     * only before preparing to stop the application.
     */
    virtual void Shutdown();

    /**
     * Get the current config directory path to use. If a value is passed
     * to SetConfigPath, that will be used, if not GetDefaultConfigPath
     * will be used instead.
     * @return Current config directory path to use
     */
    static std::string GetConfigPath();

    /**
     * Set a custom config directory path
     * @param path Config directory path to use during execution
     */
    static void SetConfigPath(const std::string& path);

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
    static bool ReadConfig(std::shared_ptr<objects::ServerConfig> config, libcomp::String filePath);

    /**
     * Read the config file values from an XML document and populates the
     * config passed in.
     * @param config Pointer to a config file that will contain properties
     *   every server has in common.
     * @param doc XML file containing the config values.
     * @return true on success, false on failure
     */
    static bool ReadConfig(std::shared_ptr<objects::ServerConfig> config, tinyxml2::XMLDocument& doc);

    /**
     * Get the server config file read during the constructor steps.
     * @return Pointer to the server config
     */
    std::shared_ptr<objects::ServerConfig> GetConfig() const;

    /**
     * Queue up code to be executed in the main worker thread.
     * @param f Function (lambda) to execute in the worker thread.
     * @param args Arguments to pass to the function when it is executed.
     * @return true on success, false on failure
     */
    template<typename Function, typename... Args>
    bool QueueWork(Function&& f, Args&&... args) const
    {
        return mQueueWorker.ExecuteInWorker(std::forward<Function>(f),
            std::forward<Args>(args)...);
    }

protected:
    /**
     * Runs the server until a shutdown message is received or the program
     * is forcefully closed.
     * @return 0 on success, 1 on failure
     */
    virtual int Run();

    /**
     * Called when the server has started.
     */
    virtual void ServerReady();

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
    bool AssignMessageQueue(const std::shared_ptr<
        libcomp::EncryptedConnection>& connection);

    /**
     * Get the next worker to use for new connections.  This implementation
     * uses a "least busy" method to decide which worker to assign but it is
     * meant to be overridden by anything with a different method of assignment.
     * @return Pointer to the worker whose message queue should be assigned
     *  to a new connection
     */
    virtual std::shared_ptr<libcomp::Worker> GetNextConnectionWorker();

    /**
     * Dynamicaly instantiate and insert data from an XML config file. Records
     * will be created in the order they are listed in the file and are assumed
     * to be valid for the database loaded for the server.
     * @param filePath Path to the XML config file
     * @param db Pointer to the database connection
     * @param specificTypes Optional parameter to only load the specified types
     * @return true on success, false if either no data was found or an error
     *  occurred
     */
    bool InsertDataFromFile(const libcomp::String& filePath,
        const std::shared_ptr<Database>& db,
        const std::set<std::string>& specificTypes = std::set<std::string>());

    /// A shared pointer to the config used to set up the server.
    std::shared_ptr<objects::ServerConfig> mConfig;

    /// Command line options for the server.
    std::shared_ptr<ServerCommandLineParser> mCommandLine;

    /// Worker that blocks and runs in the main thread.
    libcomp::Worker mMainWorker;

    /// Worker used for async processing.
    libcomp::Worker mQueueWorker;

    /// List of workers to handle incoming connection packet based work.
    std::list<std::shared_ptr<libcomp::Worker>> mWorkers;

    /// Data store for the server.
    libcomp::DataStore mDataStore;

    /// Custom config path to use during execution.
    static std::string sConfigPath;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_BASESERVER_H
