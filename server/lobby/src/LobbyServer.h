/**
 * @file server/lobby/src/LobbyServer.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby server class.
 *
 * This file is part of the Lobby Server (lobby).
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

#ifndef SERVER_LOBBY_SRC_LOBBYSERVER_H
#define SERVER_LOBBY_SRC_LOBBYSERVER_H

// libcomp Includes
#include <BaseServer.h>
#include <Worker.h>

// lobby Includes
#include "AccountManager.h"
#include "ManagerConnection.h"
#include "SessionManager.h"
#include "World.h"

namespace objects
{

class SetupConfig;

} // namespace objects

namespace lobby
{

class LobbyServer : public libcomp::BaseServer
{
public:
    /**
     * Create a new lobby server.
     * @param szProgram First command line argument for the application.
     * @param config Pointer to a casted LobbyConfig that will contain properties
     *   every server has in addition to lobby specific ones.
     * @param configPath File path to the location of the config to be loaded.
     * @param unitTestMode Debug parameter to use in unit tests.  Set to true
     *  to enable unit test mode.
     */
    LobbyServer(const char *szProgram, std::shared_ptr<
        objects::ServerConfig> config, const libcomp::String& configPath,
        bool unitTestMode);

    /**
     * Clean up the server.
     */
    virtual ~LobbyServer();

    /**
     * Initialize the database connection and do anything else that can fail
     * to execute that needs to be handled outside of a constructor.  This
     * calls the BaseServer version as well to perform shared init steps.
     * @param self Pointer to this server to be used as a reference in
     *   packet handling code.
     * @return true on success, false on failure
     */
    virtual bool Initialize();

    /**
     * Get a list of pointers to the connected worlds.
     * @return List of pointers to the connected worlds
     */
    std::list<std::shared_ptr<lobby::World>> GetWorlds();

    /**
     * Get a world by ID.
     * @param worldID ID of the world to load
     * @return Pointer to the matching world
     */
    std::shared_ptr<lobby::World> GetWorldByID(uint8_t worldID);

    /**
     * Get information about a connected world by its connection.
     * @param connection Pointer to the world's connection.
     * @return Pointer to the connected world.
     */
    std::shared_ptr<lobby::World> GetWorldByConnection(
        std::shared_ptr<libcomp::InternalConnection> connection);

    /**
     * Register a world with the connection manager.
     * @param world Pointer to the world to register
     * @return Pointer to the new registered server or nullptr
     *  on failure
     * @sa ManagerConnection::RegisterWorld
     */
    const std::shared_ptr<lobby::World> RegisterWorld(
        std::shared_ptr<lobby::World>& world);

    /**
     * Get the main database.
     * @return Pointer to the main database
     */
    std::shared_ptr<libcomp::Database> GetMainDatabase() const;

    /**
     * Get the connection manager for the server.
     * @return Pointer to the connection manager.
     */
    std::shared_ptr<ManagerConnection> GetManagerConnection() const;

    /**
     * Get the account manager for the server.
     * @return Account manager for the server.
     */
    AccountManager* GetAccountManager();

    /**
     * Get the session manager for the server.
     * @return Session manager for the server.
     */
    SessionManager* GetSessionManager();

protected:
    /**
     * Set up required test data for unit testing, removing the
     * need for human interaction via the usual prompt.
     * @return true on success, false on failure
     */
    bool InitializeTestMode();

    /**
     * Create the first account when none currently exist
     * in the connected database via PromptCreateAccount.
     * The user will also be prompted to create more accounts
     * should they want to here.
     */
    void CreateFirstAccount();

    /**
     * Prompt for and create an account via pre-populated or
     * user entered values.
     */
    void PromptCreateAccount();

    /**
     * Reset the values in the RegisteredWorld table, pulling
     * World information from it before servers connect.
     * @return true on success, false on failure
     */
    bool ResetRegisteredWorlds();

    /**
     * Setup the server based on the setup config file.
     * @return true on success, false on failure or no work done
     */
    bool Setup();

    /**
     * Create a connection to a newly active socket.
     * @param socket A new socket connection.
     * @return Pointer to the newly created connection
     */
    virtual std::shared_ptr<libcomp::TcpConnection> CreateConnection(
        asio::ip::tcp::socket& socket);

    /// A shared pointer to the main database used by the server.
    std::shared_ptr<libcomp::Database> mDatabase;

    /// Pointer to the manager in charge of connections.
    std::shared_ptr<ManagerConnection> mManagerConnection;

    /// Indicates the unit test database should be used.
    bool mUnitTestMode;

    /// Account manager for the server.
    AccountManager mAccountManager;

    /// Session manager for the server.
    SessionManager mSessionManager;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_LOBBYSERVER_H
