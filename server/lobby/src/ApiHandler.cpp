/**
 * @file server/lobby/src/ApiHandler.cpp
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

#include "ApiHandler.h"

// libcomp Includes
#include <AccountManager.h>
#include <BaseServer.h>
#include <CString.h>
#include <DatabaseConfigMariaDB.h>
#include <DatabaseConfigSQLite3.h>
#include <Decrypt.h>
#include <ErrorCodes.h>
#include <Log.h>
#include <PacketCodes.h>
#include <ScriptEngine.h>

// object Includes
#include <Character.h>
#include <CharacterProgress.h>
#include <Promo.h>
#include <WebGameSession.h>

// lobby Includes
#include "LobbySyncManager.h"
#include "ManagerConnection.h"
#include "World.h"

using namespace lobby;

#define MAX_PAYLOAD (4096)

#ifdef _WIN32
    // Disable "decorated name length exceeded" warning for
    // JsonBox::Object binding
    #pragma warning(disable : 4503)
#endif

void ApiSession::Reset()
{
    username.Clear();
    challenge.Clear();
    account.reset();
}

ApiHandler::ApiHandler(const std::shared_ptr<objects::LobbyConfig>& config,
    const std::shared_ptr<lobby::LobbyServer>& server) : mConfig(config),
    mServer(server), mAccountManager(nullptr)
{
    mParsers["/auth/get_challenge"] = &ApiHandler::Auth_Token;
    mParsers["/account/get_cp"] = &ApiHandler::Account_GetCP;
    mParsers["/account/get_details"] = &ApiHandler::Account_GetDetails;
    mParsers["/account/change_password"] = &ApiHandler::Account_ChangePassword;
    mParsers["/account/client_login"] = &ApiHandler::Account_ClientLogin;
    mParsers["/account/register"] = &ApiHandler::Account_Register;
    mParsers["/admin/get_accounts"] = &ApiHandler::Admin_GetAccounts;
    mParsers["/admin/get_account"] = &ApiHandler::Admin_GetAccount;
    mParsers["/admin/delete_account"] = &ApiHandler::Admin_DeleteAccount;
    mParsers["/admin/update_account"] = &ApiHandler::Admin_UpdateAccount;
    mParsers["/admin/get_promos"] = &ApiHandler::Admin_GetPromos;
    mParsers["/admin/create_promo"] = &ApiHandler::Admin_CreatePromo;
    mParsers["/admin/delete_promo"] = &ApiHandler::Admin_DeletePromo;
    mParsers["/webgame/get_coins"] = &ApiHandler::WebGame_GetCoins;
    mParsers["/webgame/start"] = &ApiHandler::WebGame_Start;
    mParsers["/webgame/update"] = &ApiHandler::WebGame_Update;

    LOG_DEBUG("Loading web games...\n");

    auto serverDataManager =  new libcomp::ServerDataManager;

    bool gamesLoaded = false;
    for(auto serverScript : serverDataManager->LoadScripts(server
        ->GetDataStore(), "/webgames", gamesLoaded, false))
    {
        if(serverScript->Type.ToLower() == "webgame")
        {
            mGameDefinitions[serverScript->Name.ToLower()] = serverScript;
        }
    }

    if(!gamesLoaded)
    {
        LOG_ERROR(libcomp::String("API handler failed after loading %1"
            " web game(s)\n").Arg(mGameDefinitions.size()));
    }
    else if(mGameDefinitions.size() == 0)
    {
        LOG_DEBUG("No web games found\n");
    }
    else
    {
        LOG_DEBUG(libcomp::String("API handler successfully loaded %1"
            " web game(s)\n").Arg(mGameDefinitions.size()));
    }

    delete serverDataManager;
}

ApiHandler::~ApiHandler()
{
}

bool ApiHandler::Auth_Token(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    auto it = request.find("username");

    if(it == request.end())
    {
        LOG_ERROR("get_challenge request missing a username.\n");

        session->Reset();
        return false;
    }

    libcomp::String username = it->second.getString();
    username = username.ToLower();

    // Make sure the username did not change.
    if(!session->username.IsEmpty() && session->username != username)
    {
        LOG_ERROR(libcomp::String("Session username has change from "
            "'%1' to '%2'.\n").Arg(session->username).Arg(username));

        session->Reset();
    }

    // Grab a new database connection.
    auto db = GetDatabase();

    // Sanity in an insane world.
    if(!db)
    {
        LOG_ERROR("Failed to get the database.\n");

        session->Reset();
        return false;
    }

    // Find the account for the given username.
    session->account = objects::Account::LoadAccountByUsername(
        db, username);

    // We must have a valid account for this to work.
    if(!session->account || !session->account->GetEnabled())
    {
        LOG_ERROR(libcomp::String("Invalid account (is it disabled?): "
            "%1\n").Arg(username));

        session->Reset();
        return false;
    }

    libcomp::String challenge = libcomp::Decrypt::GenerateRandom(10);

    // Save the challenge.
    session->username = username;
    session->challenge = challenge;

    response["challenge"] = JsonBox::Value(challenge.ToUtf8());
    response["salt"] = JsonBox::Value(session->account->GetSalt().ToUtf8());

    return true;
}

bool ApiHandler::Account_GetCP(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)request;

    // Find the account for the given username.
    auto account = objects::Account::LoadAccountByUsername(
        GetDatabase(), session->username);

    if(!account)
    {
        return false;
    }

    response["cp"] = (int)account->GetCP();

    return true;
}

bool ApiHandler::Account_GetDetails(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)request;

    // Find the account for the given username.
    auto account = objects::Account::LoadAccountByUsername(
        GetDatabase(), session->username);

    if(!account)
    {
        return false;
    }

    response["cp"] = (int)account->GetCP();
    response["username"] = account->GetUsername().ToUtf8();
    response["disp_name"] = account->GetDisplayName().ToUtf8();
    response["email"] = account->GetEmail().ToUtf8();
    response["ticket_count"] = (int)account->GetTicketCount();
    response["user_level"] = (int)account->GetUserLevel();
    response["enabled"] = account->GetEnabled();
    response["last_login"] = (int)account->GetLastLogin();

    int count = 0;

    for(size_t i = 0; i < account->CharactersCount(); ++i)
    {
        if(account->GetCharacters(i))
        {
            count++;
        }
    }

    response["character_count"] = count;

    return true;
}

bool ApiHandler::Account_ChangePassword(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    libcomp::String password;

    auto db = GetDatabase();

    // Find the account for the given username.
    auto account = objects::Account::LoadAccountByUsername(
        db, session->username);

    if(!account)
    {
        response["error"] = "Account not found.";

        return true;
    }

    auto it = request.find("password");

    if(it != request.end())
    {
        password = it->second.getString();

        if(!password.Matches("^[a-zA-Z0-9\\\\\\(\\)\\[\\]\\/{}~`'\"<>"
            ".,_|!@#$%^&*+=-]{6,16}$"))
        {
            response["error"] = "Bad password";

            return true;
        }
        else
        {
            libcomp::String salt = libcomp::Decrypt::GenerateRandom(10);

            // Hash the password for database storage.
            password = libcomp::Decrypt::HashPassword(password, salt);

            account->SetPassword(password);
            account->SetSalt(salt);
        }
    }
    else
    {
        response["error"] = "Password is missing.";

        return true;
    }

    bool didUpdate = account->Update(db);

    // Clear the session and make the user re-authenticate.
    session->username.Clear();
    session->account.reset();

    if(didUpdate)
    {
        response["error"] = "Success";
    }
    else
    {
        response["error"] = "Failed to update password.";
    }

    return true;
}

bool ApiHandler::Account_ClientLogin(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    libcomp::String password;

    auto db = GetDatabase();

    // Find the account for the given username.
    auto account = objects::Account::LoadAccountByUsername(
        db, session->username);

    if(!account)
    {
        response["error"] = ErrorCodeString(
            ErrorCodes_t::BAD_USERNAME_PASSWORD).ToUtf8();
        response["error_code"] = to_underlying(
            ErrorCodes_t::BAD_USERNAME_PASSWORD);

        return true;
    }

    auto it = request.find("client_version");

    if(it == request.end())
    {
        response["error"] = ErrorCodeString(
            ErrorCodes_t::WRONG_CLIENT_VERSION).ToUtf8();
        response["error_code"] = to_underlying(
            ErrorCodes_t::WRONG_CLIENT_VERSION);

        return true;
    }

    // Check the account manager.
    if(!mAccountManager)
    {
        response["error"] = ErrorCodeString(
            ErrorCodes_t::SYSTEM_ERROR).ToUtf8();
        response["error_code"] = to_underlying(
            ErrorCodes_t::SYSTEM_ERROR);

        return true;
    }

    libcomp::String sid1;

    // This session ID is never used. If you notice it being used file a bug.
    static const libcomp::String sid2 = "deadc0dedeadc0dedeadc0dedeadc0de"
        "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
        "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
        "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
        "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
        "deadc0dedead";

    // Grab the client version as a string.
    libcomp::String clientVersion = it->second.getString();

    // Attempt to login for the user.
    ErrorCodes_t error = mAccountManager->WebAuthLoginApi(
        session->username, (uint32_t)(clientVersion.ToDecimal<float>() *
            1000.0f + 0.5f), sid1);

    response["error"] = ErrorCodeString(error).ToUtf8();
    response["error_code"] = to_underlying(error);

    if(ErrorCodes_t::SUCCESS == error)
    {
        response["sid1"] = sid1.ToUtf8();
        response["sid2"] = sid1.ToUtf8();
    }

    return true;
}

bool ApiHandler::Account_Register(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)session;

    libcomp::String username, email, password;

    auto it = request.find("username");

    if(it != request.end())
    {
        username = it->second.getString();
        username = username.ToLower();
    }

    it = request.find("email");

    if(it != request.end())
    {
        email = it->second.getString();
    }

    it = request.find("password");

    if(it != request.end())
    {
        password = it->second.getString();
    }

    if(username.IsEmpty() || email.IsEmpty() || password.IsEmpty())
    {
        return false;
    }

    if(!username.Matches("^[a-z][a-z0-9]{3,31}$"))
    {
        response["error"] = "Bad username";

        return true;
    }
    else if(!password.Matches("^[a-zA-Z0-9\\\\\\(\\)\\[\\]\\/{}~`'\"<>"
        ".,_|!@#$%^&*+=-]{6,16}$"))
    {
        response["error"] = "Bad password";

        return true;
    }
    else if(!email.Matches("(?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\\.[a-z0-9!"
        "#$%&'*+/=?^_`{|}~-]+)*|\"(?:[\\x01-\\x08\\x0b\\x0c\\x0e-\\x1f\\x21"
        "\\x23-\\x5b\\x5d-\\x7f]|\\\\[\\x01-\\x09\\x0b\\x0c\\x0e-\\x7f])*\")"
        "@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\\.)+[a-z0-9](?:[a-z0-9-]*"
        "[a-z0-9])?|\\[(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
        "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?|[a-z0-9-]*[a-z0-9]:(?:["
        "\\x01-\\x08\\x0b\\x0c\\x0e-\\x1f\\x21-\\x5a\\x53-\\x7f]|\\\\[\\x01"
        "-\\x09\\x0b\\x0c\\x0e-\\x7f])+)\\])"))
    {
        // RFC 5322
        response["error"] = "Bad email";

        return true;
    }

    auto db = GetDatabase();

    if(objects::Account::LoadAccountByUsername(db, username) ||
        objects::Account::LoadAccountByEmail(db, email))
    {
        response["error"] = "Account exists";

        return true;
    }

    std::shared_ptr<objects::Account> account(new objects::Account);

    libcomp::String displayName = username;
    libcomp::String salt = libcomp::Decrypt::GenerateRandom(10);
    uint32_t cp = mConfig->GetRegistrationCP();
    uint8_t ticketCount = mConfig->GetRegistrationTicketCount();
    int32_t userLevel = mConfig->GetRegistrationUserLevel();
    bool enabled = mConfig->GetRegistrationAccountEnabled();

    // Hash the password for database storage.
    password = libcomp::Decrypt::HashPassword(password, salt);

    account->SetUsername(username);
    account->SetDisplayName(displayName);
    account->SetEmail(email);
    account->SetPassword(password);
    account->SetSalt(salt);
    account->SetCP(cp);
    account->SetTicketCount(ticketCount);
    account->SetUserLevel(userLevel);
    account->SetEnabled(enabled);
    account->Register(account);

    if(!account->Insert(db))
    {
        response["error"] = "Failed to create account.";
    }
    else
    {
        response["error"] = "Success";
    }

    return true;
}

bool ApiHandler::Admin_GetAccounts(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)request;
    (void)session;

    auto accounts = libcomp::PersistentObject::LoadAll<objects::Account>(
        GetDatabase());

    JsonBox::Array accountObjects;

    for(auto account : accounts)
    {
        JsonBox::Object obj;

        obj["cp"] = (int)account->GetCP();
        obj["username"] = account->GetUsername().ToUtf8();
        obj["disp_name"] = account->GetDisplayName().ToUtf8();
        obj["email"] = account->GetEmail().ToUtf8();
        obj["ticket_count"] = (int)account->GetTicketCount();
        obj["user_level"] = (int)account->GetUserLevel();
        obj["enabled"] = account->GetEnabled();
        obj["last_login"] = (int)account->GetLastLogin();

        int count = 0;

        for(size_t i = 0; i < account->CharactersCount(); ++i)
        {
            if(account->GetCharacters(i))
            {
                count++;
            }
        }

        obj["character_count"] = count;

        accountObjects.push_back(obj);
    }

    std::sort(accountObjects.begin(), accountObjects.end(),
        [](const JsonBox::Value& a,const JsonBox::Value& b)
        {
            return a.getObject().at("username").getString() <
                b.getObject().at("username").getString();
        });

    response["accounts"] = accountObjects;

    return true;
}

bool ApiHandler::Admin_GetAccount(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)session;

    libcomp::String username;

    auto it = request.find("username");

    if(it != request.end())
    {
        username = it->second.getString();
        username = username.ToLower();
    }
    else
    {
        return false;
    }

    // Find the account for the given username.
    auto account = objects::Account::LoadAccountByUsername(
        GetDatabase(), username);

    if(!account)
    {
        return false;
    }

    response["cp"] = (int)account->GetCP();
    response["username"] = account->GetUsername().ToUtf8();
    response["disp_name"] = account->GetDisplayName().ToUtf8();
    response["email"] = account->GetEmail().ToUtf8();
    response["ticket_count"] = (int)account->GetTicketCount();
    response["user_level"] = (int)account->GetUserLevel();
    response["enabled"] = account->GetEnabled();
    response["last_login"] = (int)account->GetLastLogin();

    int count = 0;

    for(size_t i = 0; i < account->CharactersCount(); ++i)
    {
        if(account->GetCharacters(i))
        {
            count++;
        }
    }

    response["character_count"] = count;

    return true;
}

bool ApiHandler::Admin_DeleteAccount(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)response;

    libcomp::String username;

    auto it = request.find("username");

    if(it != request.end())
    {
        username = it->second.getString();
        username = username.ToLower();
    }
    else
    {
        return false;
    }

    auto db = GetDatabase();
    auto account = objects::Account::LoadAccountByUsername(db, username);

    if(!account)
    {
        return false;
    }

    bool didDelete = account->Delete(db);

    if(session->username == username)
    {
        session->username.Clear();
        session->account.reset();
    }

    return didDelete;
}

bool ApiHandler::Admin_UpdateAccount(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    libcomp::String username, password;

    auto it = request.find("username");

    if(it != request.end())
    {
        username = it->second.getString();
        username = username.ToLower();
    }
    else
    {
        response["error"] = "Username not found.";

        return true;
    }

    auto db = GetDatabase();
    auto account = objects::Account::LoadAccountByUsername(db, username);

    if(!account)
    {
        response["error"] = "Account not found.";

        return true;
    }

    it = request.find("password");

    if(it != request.end())
    {
        password = it->second.getString();

        if(!password.Matches("^[a-zA-Z0-9\\\\\\(\\)\\[\\]\\/{}~`'\"<>"
            ".,_|!@#$%^&*+=-]{6,16}$"))
        {
            response["error"] = "Bad password";

            return true;
        }
        else
        {
            libcomp::String salt = libcomp::Decrypt::GenerateRandom(10);

            // Hash the password for database storage.
            password = libcomp::Decrypt::HashPassword(password, salt);

            account->SetPassword(password);
            account->SetSalt(salt);
        }
    }

    it = request.find("disp_name");

    if(it != request.end())
    {
        account->SetDisplayName(it->second.getString());
    }

    it = request.find("cp");

    if(it != request.end())
    {
        int cp = it->second.getInteger();

        if(0 > cp)
        {
            response["error"] = "CP must be a positive integer or zero.";

            return true;
        }

        account->SetCP((uint32_t)cp);
    }

    int count = 0;

    for(size_t i = 0; i < account->CharactersCount(); ++i)
    {
        if(account->GetCharacters(i))
        {
            count++;
        }
    }

    it = request.find("ticket_count");

    if(it != request.end())
    {
        int ticket_count = it->second.getInteger();

        if((ticket_count + count) > (int)account->CharactersCount() ||
            0 > ticket_count)
        {
            response["error"] = "Ticket count must be a positive integer or "
                "zero. Ticket count must not be more than the number of "
                "free character slots.";

            return true;
        }

        account->SetTicketCount((uint8_t)ticket_count);
    }

    it = request.find("user_level");

    if(it != request.end())
    {
        int user_level = it->second.getInteger();

        if(0 > user_level || 1000 < user_level)
        {
            response["error"] = "User level must be in the range [0, 1000].";

            return true;
        }

        account->SetUserLevel(user_level);
    }

    it = request.find("enabled");

    if(it != request.end())
    {
        account->SetEnabled(it->second.getBoolean());
    }

    bool didUpdate = account->Update(db);

    if(session->username == username)
    {
        session->username.Clear();
        session->account.reset();
    }

    if(didUpdate)
    {
        response["error"] = "Success";
    }
    else
    {
        response["error"] = "Failed to update account.";
    }

    return true;
}

bool ApiHandler::Admin_GetPromos(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)request;
    (void)session;

    auto promos = libcomp::PersistentObject::LoadAll<objects::Promo>(
        GetDatabase());

    JsonBox::Array promoObjects;

    for(auto promo : promos)
    {
        JsonBox::Object obj;
        JsonBox::Array items;

        obj["code"] = promo->GetCode().ToUtf8();
        obj["startTime"] = (int)promo->GetStartTime();
        obj["endTime"] = (int)promo->GetEndTime();
        obj["useLimit"] = (int)promo->GetUseLimit();

        switch(promo->GetLimitType())
        {
            case objects::Promo::LimitType_t::PER_CHARACTER:
                obj["limitType"] = "character";
                break;
            case objects::Promo::LimitType_t::PER_WORLD:
                obj["limitType"] = "world";
                break;
            default:
                obj["limitType"] = "account";
                break;
        }

        for(auto item : promo->GetPostItems())
        {
            JsonBox::Value val;
            val = (int)item;

            items.push_back(val);
        }

        obj["items"] = items;

        promoObjects.push_back(obj);
    }

    response["promos"] = promoObjects;

    return true;
}

bool ApiHandler::Admin_CreatePromo(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)session;

    libcomp::String code;
    libcomp::String limitType;
    int32_t startTime = 0;
    int32_t endTime = 0;
    int32_t useLimit = 0;
    objects::Promo::LimitType_t limitEnum;
    JsonBox::Array items;

    auto it = request.find("code");

    if(it != request.end())
    {
        code = it->second.getString();
    }

    if(code.IsEmpty())
    {
        response["error"] = "Invalid promo code.";

        return true;
    }

    it = request.find("startTime");

    if(it != request.end())
    {
        startTime = it->second.getInteger();
    }

    it = request.find("endTime");

    if(it != request.end())
    {
        endTime = it->second.getInteger();
    }

    if(0 == startTime || 0 == endTime || endTime < startTime)
    {
        response["error"] = "Invalid start or end timestamp.";

        return true;
    }

    it = request.find("useLimit");

    if(it != request.end())
    {
        useLimit = it->second.getInteger();
    }

    if(0 > useLimit || 255 < useLimit)
    {
        response["error"] = "Invalid use limit.";

        return true;
    }

    it = request.find("limitType");

    if(it != request.end())
    {
        limitType = it->second.getString();
    }

    if("character" == limitType)
    {
        limitEnum = objects::Promo::LimitType_t::PER_CHARACTER;
    }
    else if("world" == limitType)
    {
        limitEnum = objects::Promo::LimitType_t::PER_WORLD;
    }
    else if("account" == limitType)
    {
        limitEnum = objects::Promo::LimitType_t::PER_ACCOUNT;
    }
    else
    {
        response["error"] = "Invalid limit type.";

        return true;
    }

    it = request.find("items");

    if(it != request.end())
    {
        items = it->second.getArray();
    }

    if(items.empty())
    {
        response["error"] = "Promo has no item.";

        return true;
    }

    for(auto item : items)
    {
        int32_t productID = item.getInteger();

        /// @todo Check the shop product ID is valid.

        if(0 == productID)
        {
            response["error"] = "Invalid item.";

            return true;
        }
    }

    // Check if the promo code exists.
    auto promos = objects::Promo::LoadPromoListByCode(GetDatabase(), code);

    if(!promos.empty())
    {
        response["error"] = "Promotion with that code already exists. "
            "Another will be made.";
    }
    else
    {
        response["error"] = "Success";
    }

    std::shared_ptr<objects::Promo> promo(new objects::Promo);
    promo->SetCode(code);
    promo->SetStartTime((uint32_t)startTime);
    promo->SetEndTime((uint32_t)endTime);
    promo->SetUseLimit((uint8_t)useLimit);
    promo->SetLimitType(limitEnum);

    for(auto item : items)
    {
        promo->AppendPostItems((uint32_t)item.getInteger());
    }

    promo->Register(std::dynamic_pointer_cast<
        libcomp::PersistentObject>(promo));

    if(!promo->Insert(GetDatabase()))
    {
        response["error"] = "Failed to create promotion.";
    }

    return true;
}

bool ApiHandler::Admin_DeletePromo(const JsonBox::Object& request,
    JsonBox::Object& response, const std::shared_ptr<ApiSession>& session)
{
    (void)session;

    libcomp::String code;
    int promoCount = 0;

    auto it = request.find("code");

    if(it != request.end())
    {
        code = it->second.getString();
    }

    if(code.IsEmpty())
    {
        response["error"] = "Invalid promo code.";

        return true;
    }

    // Get the list of promos with that code.
    auto promos = objects::Promo::LoadPromoListByCode(GetDatabase(), code);
    auto db = GetDatabase();

    for(auto promo : promos)
    {
        if(!promo->Delete(db))
        {
            response["error"] = "Failed to delete promo.";

            return true;
        }

        promoCount++;
    }

    response["error"] = libcomp::String(
        "Deleted %1 promotions.").Arg(promoCount).ToUtf8();

    return true;
}

bool ApiHandler::WebGame_GetCoins(const JsonBox::Object& request,
    JsonBox::Object& response,
    const std::shared_ptr<ApiSession>& session)
{
    (void)request;

    std::shared_ptr<objects::WebGameSession> gameSession;
    std::shared_ptr<World> world;
    if(!GetWebGameSession(response, session, gameSession, world))
    {
        return true;
    }

    int64_t coins = WebGameScript_GetCoins(session);
    if(coins == -1)
    {
        response["error"] = "Failed to get coins";
        return true;
    }

    response["error"] = "Success";
    response["coins"] = libcomp::String("%1").Arg(coins).C();

    return true;
}

bool ApiHandler::WebGame_Start(const JsonBox::Object& request,
    JsonBox::Object& response,
    const std::shared_ptr<ApiSession>& session)
{
    std::shared_ptr<objects::WebGameSession> gameSession;
    std::shared_ptr<World> world;
    if(!GetWebGameSession(response, session, gameSession, world))
    {
        return true;
    }

    auto webGameSession = std::dynamic_pointer_cast<WebGameApiSession>(
        session);
    if(webGameSession->gameState)
    {
        response["error"] = "Game has already been started";
        return true;
    }

    auto it = request.find("type");
    if(it == request.end())
    {
        response["error"] = "Game type was not specified";
        return true;
    }

    libcomp::String type(it->second.getString());

    auto gameIter = mGameDefinitions.find(type);
    if(gameIter == mGameDefinitions.end())
    {
        response["error"] = "Specified game type is not valid";
        return true;
    }

    webGameSession->gameState = std::make_shared<libcomp::ScriptEngine>();
    webGameSession->gameState->Using<objects::Character>();

    // Bind the handler and the JSON response structure and session as well
    // but nothing on them since we only need to pass through to the API
    // functions
    {
        auto vm = webGameSession->gameState->GetVM();

        Sqrat::Class<ApiSession,
            Sqrat::NoConstructor<ApiSession>> sBinding(vm, "ApiSession");
        Sqrat::RootTable(vm).Bind("ApiSession", sBinding);

        Sqrat::Class<JsonBox::Object,
            Sqrat::NoConstructor<JsonBox::Object>> oBinding(vm, "JsonObject");
        Sqrat::RootTable(vm).Bind("JsonObject", oBinding);

        Sqrat::Class<ApiHandler,
            Sqrat::NoConstructor<ApiHandler>> apiBinding(vm, "ApiHandler");
        apiBinding
            .Func("GetCoins", &ApiHandler::WebGameScript_GetCoins)
            .Func("SetResponse", &ApiHandler::WebGameScript_SetResponse)
            .Func("UpdateCoins", &ApiHandler::WebGameScript_UpdateCoins);
        Sqrat::RootTable(vm).Bind("ApiHandler", apiBinding);
    }

    if(!webGameSession->gameState->Eval(gameIter->second->Source))
    {
        response["error"] = "Game could not be started";
        return true;
    }

    auto worldDB = world->GetWorldDatabase();

    auto character = gameSession->GetCharacter().Get(worldDB, true);
    auto progress = character
        ? character->GetProgress().Get(worldDB, true) : nullptr;
    if(!character || !progress)
    {
        response["error"] = "Character information could not be retrieved";
        return true;
    }

    // Call the start function first then write standard response values
    auto vm = webGameSession->gameState->GetVM();
    Sqrat::Function f(Sqrat::RootTable(vm), "start");
    if(!f.IsNull())
    {
        Sqrat::Table sqOutTable(vm);

        auto result = !f.IsNull() ? f.Evaluate<int>(character,
            progress->GetCoins(), sqOutTable) : 0;
        if(!result || (*result != 0))
        {
            response["error"] = "Unknown error encountered while starting"
                " game";
            return true;
        }

        Sqrat::Table::iterator tableIter;
        while(sqOutTable.Next(tableIter))
        {
            auto name = tableIter.getName();
            if(name)
            {
                auto val = sqOutTable.GetValue<std::string>(name);
                if(val)
                {
                    response[name] = val->c_str();
                }
            }
        }

        if(response.find("error") == response.end())
        {
            response["error"] = "Success";
        }
    }

    response["name"] = character->GetName().C();
    response["coins"] = libcomp::String("%1").Arg(progress->GetCoins()).C();

    return true;
}

bool ApiHandler::WebGame_Update(const JsonBox::Object& request,
    JsonBox::Object& response,
    const std::shared_ptr<ApiSession>& session)
{
    std::shared_ptr<objects::WebGameSession> gameSession;
    std::shared_ptr<World> world;
    if(!GetWebGameSession(response, session, gameSession, world))
    {
        return true;
    }

    auto webGameSession = std::dynamic_pointer_cast<WebGameApiSession>(
        session);
    if(!webGameSession->gameState)
    {
        response["error"] = "Game not started";
        return true;
    }

    auto it = request.find("action");
    if(it == request.end())
    {
        response["error"] = "No action specified";
        return true;
    }

    libcomp::String action(it->second.getString());

    auto vm = webGameSession->gameState->GetVM();
    Sqrat::Function f(Sqrat::RootTable(vm), action.C());
    if(!f.IsNull())
    {
        Sqrat::Table sqTable(vm);
        for(auto& rPair : request)
        {
            // Forward everything but the system params
            if(rPair.first != "action" && rPair.first != "sessionid" &&
                rPair.first != "username")
            {
                libcomp::String val;
                if(rPair.second.isInteger())
                {
                    val = libcomp::String("%1").Arg(rPair.second.getInteger());
                }
                else
                {
                    val = libcomp::String(rPair.second.getString());
                }

                sqTable.SetValue<libcomp::String>(rPair.first.c_str(),
                    val);
            }
        }

        // Tables work fine as input parameters but seem to be read-only
        // so bind the response directly and write to it with a utility
        // function
        auto result = !f.IsNull() ? f.Evaluate<int>(this, session, sqTable,
            &response) : 0;
        if(!result || (*result != 0))
        {
            response["error"] = "Unknown error encountered";
            return true;
        }

        if(response.find("error") == response.end())
        {
            response["error"] = "Success";
        }
    }
    else
    {
        response["error"] = "Invalid action attempted";
        return true;
    }

    return true;
}

int64_t ApiHandler::WebGameScript_GetCoins(const std::shared_ptr<
    ApiSession>& session)
{
    auto wgSession = std::dynamic_pointer_cast<WebGameApiSession>(session);
    auto gameSession = wgSession ? wgSession->webGameSession : nullptr;
    if(!gameSession)
    {
        return -1;
    }

    auto world = mServer->GetManagerConnection()->GetWorldByID(
        gameSession->GetWorldID());
    if(!world)
    {
        return -1;
    }

    auto worldDB = world->GetWorldDatabase();

    auto character = gameSession->GetCharacter().Get(worldDB);
    auto progress = character
        ? character->GetProgress().Get(worldDB) : nullptr;
    if(progress)
    {
        return progress->GetCoins();
    }

    return -1;
}

void ApiHandler::WebGameScript_SetResponse(JsonBox::Object* response,
    const libcomp::String& key, const libcomp::String& value)
{
    if(response)
    {
        (*response)[key.C()] = value.C();
    }
}

bool ApiHandler::WebGameScript_UpdateCoins(
    const std::shared_ptr<ApiSession>& session, int64_t coins, bool adjust)
{
    auto wgSession = std::dynamic_pointer_cast<WebGameApiSession>(session);
    auto gameSession = wgSession ? wgSession->webGameSession : nullptr;
    if(!gameSession)
    {
        return false;
    }

    auto world = mServer->GetManagerConnection()->GetWorldByID(
        gameSession->GetWorldID());
    if(!world)
    {
        return false;
    }

    auto worldDB = world->GetWorldDatabase();

    auto character = gameSession->GetCharacter().Get(worldDB);
    auto progress = character
        ? character->GetProgress().Get(worldDB) : nullptr;

    bool failure = !character || !progress;
    if(!failure)
    {
        int64_t amount = progress->GetCoins();
        int64_t newAmount = adjust ? amount + coins : coins;
        if(newAmount < 0)
        {
            newAmount = 0;
        }

        bool success = false;
        if(amount == newAmount)
        {
            success = true;
        }
        else
        {
            auto changes = std::make_shared<libcomp::DBOperationalChangeSet>();
            auto expl = std::make_shared<libcomp::DBExplicitUpdate>(progress);
            expl->SetFrom<int64_t>("Coins", newAmount, progress->GetCoins());
            changes->AddOperation(expl);
            success = worldDB->ProcessChangeSet(changes);
        }

        if(success)
        {
            gameSession->SetCoins(newAmount);

            // Sync with the world
            mServer->GetLobbySyncManager()->UpdateRecord(progress,
                "CharacterProgress");
        }
        else
        {
            failure = true;
        }
    }

    return !failure;
}

bool ApiHandler::Authenticate(const JsonBox::Object& request,
    JsonBox::Object& response,
    const std::shared_ptr<ApiSession>& session)
{
    // Check first if a challenge was ever requested.
    if(session->username.IsEmpty() || !session->account)
    {
        return false;
    }

    // Check for the challenge reply.
    auto it = request.find("challenge");

    if(it == request.end())
    {
        // Force the client to re-authenticate.
        session->Reset();
        return false;
    }

    libcomp::String challenge = it->second.getString();

    // Calculate the correct response.
    libcomp::String validChallenge = libcomp::Decrypt::HashPassword(
        session->account->GetPassword(), session->challenge);

    // Check the challenge.
    if(challenge != validChallenge)
    {
        // Force the client to re-authenticate.
        session->Reset();
        return false;
    }

    // Generate a new challenge.
    challenge = libcomp::Decrypt::GenerateRandom(10);
    session->challenge = challenge;

    response["challenge"] = JsonBox::Value(challenge.ToUtf8());

    return true;
}

std::shared_ptr<libcomp::Database> ApiHandler::GetDatabase() const
{
    libcomp::EnumMap<objects::ServerConfig::DatabaseType_t,
        std::shared_ptr<objects::DatabaseConfig>> configMap;

    configMap[objects::ServerConfig::DatabaseType_t::SQLITE3]
        = mConfig->GetSQLite3Config();

    configMap[objects::ServerConfig::DatabaseType_t::MARIADB]
        = mConfig->GetMariaDBConfig();

    auto dbType = mConfig->GetDatabaseType();
    auto db = libcomp::BaseServer::GetDatabase(dbType, configMap);

    if(db && !db->Use())
    {
        return {};
    }

    return db;
}

bool ApiHandler::handlePost(CivetServer *pServer,
    struct mg_connection *pConnection)
{
    (void)pServer;

    const mg_request_info *pRequestInfo = mg_get_request_info(pConnection);

    // Sanity check the request info.
    if(nullptr == pRequestInfo)
    {
        return false;
    }

    libcomp::String method(pRequestInfo->request_uri);

    if("/api/" != method.Left(strlen("/api/")))
    {
        return false;
    }

    method = method.Mid(strlen("/api"));

    size_t postContentLength = static_cast<size_t>(
        pRequestInfo->content_length);

    // Sanity check the post content length.
    if(0 == postContentLength)
    {
        mg_printf(pConnection, "HTTP/1.1 411 Length Required\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    // Make sure the post request is not too large.
    if(MAX_PAYLOAD < postContentLength)
    {
        mg_printf(pConnection, "HTTP/1.1 413 Payload Too Large\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    // Allocate.
    char *szPostData = new char[postContentLength + 1];

    // Read the post data.
    postContentLength = static_cast<size_t>(mg_read(
        pConnection, szPostData, postContentLength));
    szPostData[postContentLength] = 0;

    JsonBox::Value request;
    request.loadFromString(szPostData);

    if(request.isNull() || !request.isObject())
    {
        mg_printf(pConnection, "HTTP/1.1 418 I'm a teapot\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    delete[] szPostData;

    const JsonBox::Object& obj = request.getObject();

    std::stringstream ss;
    JsonBox::Object response;

    libcomp::String clientAddress(pRequestInfo->remote_addr);

    bool webGame = method.Left(9) == "/webgame/";

    bool unauthorized = false;

    std::shared_ptr<ApiSession> session;
    if(webGame)
    {
        // Username and session ID must be included in all web-game requests
        auto it = obj.find("username");

        libcomp::String username = it != obj.end()
            ? it->second.getString() : "";

        it = obj.find("sessionid");

        libcomp::String sessionID = it != obj.end()
            ? it->second.getString() : "";

        auto accountManager = mServer ? mServer->GetAccountManager() : nullptr;
        session = accountManager ? accountManager->GetWebGameApiSession(
            username, sessionID, clientAddress) : nullptr;
        if(!session)
        {
            unauthorized = true;
        }
    }
    else
    {
        // Normal API sessions are stored per address
        auto it = mSessions.find(clientAddress);

        if(it != mSessions.end())
        {
            session = it->second;
        }
        else
        {
            session = std::make_shared<ApiSession>();
            session->clientAddress = clientAddress;

            mSessions[clientAddress] = session;
        }

        if(("/auth/get_challenge" != method &&
            "/account/register" != method &&
            !Authenticate(obj, response, session)) ||
            ("/admin/" == method.Left(strlen("/admin/")) &&
            (!session->account || session->account->GetUserLevel() < 1000)))
        {
            unauthorized = true;
        }
    }

    if(unauthorized)
    {
        mg_printf(pConnection, "HTTP/1.1 401 Unauthorized\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    auto it = mParsers.find(method);

    if(it == mParsers.end())
    {
        mg_printf(pConnection, "HTTP/1.1 404 Not Found\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    if(!it->second(*this, obj, response, session))
    {
        mg_printf(pConnection, "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    JsonBox::Value responseValue(response);
    responseValue.writeToStream(ss);

    mg_printf(pConnection, "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %u\r\n"
        "Connection: close\r\n"
        "\r\n%s", (uint32_t)ss.str().size(),
        ss.str().c_str());

    return true;
}

void ApiHandler::SetAccountManager(AccountManager *pManager)
{
    mAccountManager = pManager;
}

bool ApiHandler::GetWebGameSession(JsonBox::Object& response,
    const std::shared_ptr<ApiSession>& session, std::shared_ptr<
    objects::WebGameSession>& gameSession, std::shared_ptr<World>& world)
{
    auto wgSession = std::dynamic_pointer_cast<WebGameApiSession>(session);
    gameSession = wgSession ? wgSession->webGameSession : nullptr;
    if(!gameSession)
    {
        response["error"] = "Invalid session";
        return false;
    }

    world = mServer->GetManagerConnection()->GetWorldByID(
        gameSession->GetWorldID());
    if(!world)
    {
        response["error"] = "World connection down";
        return false;
    }

    return true;
}

