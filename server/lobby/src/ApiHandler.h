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
#include <ServerDataManager.h>

// libobjgen Includes
#include <Account.h>

// JsonBox Includes
#include <JsonBox.h>

// Standard C++11 Includes
#include <unordered_map>

namespace objects
{
class WebGameSession;
}

namespace libobjects
{
class ScriptEngine;
}

namespace lobby
{

class AccountManager;
class World;

class ApiSession
{
public:
    virtual ~ApiSession() { }

    void Reset();

    libcomp::String username;
    libcomp::String challenge;
    libcomp::String clientAddress;
    std::shared_ptr<objects::Account> account;
};

class WebGameApiSession : public ApiSession
{
public:
    std::shared_ptr<objects::WebGameSession> webGameSession;
    std::shared_ptr<libcomp::ScriptEngine> gameState;
};

class ApiHandler : public CivetHandler
{
public:
    ApiHandler(const std::shared_ptr<objects::LobbyConfig>& config,
        const std::shared_ptr<lobby::LobbyServer>& server);

    virtual ~ApiHandler();

    virtual bool handlePost(CivetServer *pServer,
        struct mg_connection *pConnection);

    void SetAccountManager(AccountManager *pManager);

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
    bool Account_ClientLogin(const JsonBox::Object& request,
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
    bool Admin_GetPromos(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Admin_CreatePromo(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool Admin_DeletePromo(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);

    bool WebGame_GetCoins(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool WebGame_Start(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);
    bool WebGame_Update(const JsonBox::Object& request,
        JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session);

    int64_t WebGameScript_GetCoins(const std::shared_ptr<ApiSession>& session);
    void WebGameScript_SetResponse(JsonBox::Object* response,
        const libcomp::String& key, const libcomp::String& value);
    bool WebGameScript_UpdateCoins(const std::shared_ptr<ApiSession>& session,
        int64_t coins, bool adjust);

private:
    bool GetWebGameSession(JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session,
        std::shared_ptr<objects::WebGameSession>& gameSession,
        std::shared_ptr<World>& world);

    // List of API sessions.
    std::unordered_map<libcomp::String, std::shared_ptr<ApiSession>> mSessions;

    /// List of API parsers.
    std::unordered_map<libcomp::String, std::function<bool(ApiHandler&,
        const JsonBox::Object& request, JsonBox::Object& response,
        const std::shared_ptr<ApiSession>& session)>> mParsers;

    std::shared_ptr<objects::LobbyConfig> mConfig;
    std::shared_ptr<lobby::LobbyServer> mServer;

    std::unordered_map<libcomp::String,
        std::shared_ptr<libcomp::ServerScript>> mGameDefinitions;

    AccountManager *mAccountManager;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_APIHANDLER_H
