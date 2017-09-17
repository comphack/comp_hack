/**
 * @file server/lobby/src/ApiHandler.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Civet API handler for the RESTful API.
 *
 * This file is part of the Lobby Server (lobby).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#ifndef SERVER_LOBBY_SRC_APIHANDLER_H
#define SERVER_LOBBY_SRC_APIHANDLER_H

// lobby Includes
#include "LobbyConfig.h"
#include "LobbyServer.h"

// Civet Includes
#include <CivetServer.h>

// libcomp Includes
#include <CString.h>
#include <Database.h>

// libobjgen Includes
#include <Account.h>

// JsonBox Includes
#include <JsonBox.h>

// Standard C++11 Includes
#include <unordered_map>

namespace lobby
{

class ApiSession
{
public:
    void Reset();

    libcomp::String username;
    libcomp::String challenge;
    libcomp::String clientAddress;
    std::shared_ptr<objects::Account> account;
};

class ApiHandler : public CivetHandler
{
public:
    ApiHandler(const std::shared_ptr<objects::LobbyConfig>& config,
        const std::shared_ptr<lobby::LobbyServer>& server);

    virtual ~ApiHandler();

    virtual bool handlePost(CivetServer *pServer,
        struct mg_connection *pConnection);

protected:
    bool Authenticate(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    std::shared_ptr<libcomp::Database> GetDatabase() const;

    bool Auth_Token(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);

    bool Account_GetCP(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Account_GetDetails(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Account_ChangePassword(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Account_Register(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);

    bool Admin_GetAccounts(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Admin_GetAccount(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Admin_DeleteAccount(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Admin_UpdateAccount(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);

private:
    // List of API sessions.
    std::unordered_map<libcomp::String, std::shared_ptr<ApiSession>> mSessions;

    /// List of API parsers.
    std::unordered_map<libcomp::String, std::function<bool(ApiHandler&,
        const JsonBox::Object& request, JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session)>> mParsers;

    std::shared_ptr<objects::LobbyConfig> mConfig;
    std::shared_ptr<lobby::LobbyServer> mServer;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_APIHANDLER_H
