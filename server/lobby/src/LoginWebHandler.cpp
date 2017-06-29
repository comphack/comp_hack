/**
 * @file server/lobby/src/LoginWebHandler.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Civet login webpage handler.
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

#include "LoginWebHandler.h"

// lobby Includes
#include "AccountManager.h"
#include "ResourceLogin.h"
#include "SessionManager.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>

// libobjgen Includes
#include <Account.h>

using namespace lobby;

LoginHandler::LoginHandler(const std::shared_ptr<libcomp::Database>& database)
    : mDatabase(database), mAccountManager(nullptr), mSessionManager(nullptr)
{
    mVfs.AddArchiveLoader(new ttvfs::VFSZipArchiveLoader);

    ttvfs::CountedPtr<ttvfs::MemFile> pMemoryFile = new ttvfs::MemFile(
        "login.zip", ResourceLogin, static_cast<uint32_t>(ResourceLoginSize));

    if(!mVfs.AddArchive(pMemoryFile, ""))
    {
        LOG_CRITICAL("Failed to add login resource archive.\n");
    }
}

LoginHandler::~LoginHandler()
{
}

void LoginHandler::SetConfig(const std::shared_ptr<
    objects::LobbyConfig>& config)
{
    mConfig = config;
}

LoginHandler::ReplacementVariables::ReplacementVariables() : birthday("1"),
    cv("Unknown"), idsave("checked"), auth(false), quit(false)
{
    msg = "<span style=\"font-size:12px;color:#c3c3c3;"
        "font-weight:bold;\"><br>&nbsp;Please enter your "
        "username and password.</span>";

    submit = "<input class=\"login\" type=\"submit\" "
        "value=\"\" tabindex=\"4\" name=\"login\" height=\"60\" "
        "width=\"67\" />";
}

bool LoginHandler::handleGet(CivetServer *pServer,
    struct mg_connection *pConnection)
{
    ReplacementVariables postVars;

    return HandlePage(pServer, pConnection, postVars);
}

bool LoginHandler::handlePost(CivetServer *pServer,
    struct mg_connection *pConnection)
{
    ReplacementVariables postVars;

    ParsePost(pServer, pConnection, postVars);

    return HandlePage(pServer, pConnection, postVars);
}

void LoginHandler::ParsePost(CivetServer *pServer,
    struct mg_connection *pConnection, ReplacementVariables& postVars)
{
    const mg_request_info *pRequestInfo = mg_get_request_info(pConnection);

    // Sanity check the request info.
    if(nullptr == pRequestInfo || nullptr == mConfig)
    {
        return;
    }

    size_t postContentLength = static_cast<size_t>(
        pRequestInfo->content_length);

    // Sanity check the post content length.
    if(0 == postContentLength)
    {
        return;
    }

    // Allocate.
    char *szPostData = new char[postContentLength + 1];

    // Read the post data.
    postContentLength = static_cast<size_t>(mg_read(
        pConnection, szPostData, postContentLength));
    szPostData[postContentLength] = 0;

    // Last read post value.
    std::string postValue;

    if(pServer->getParam(szPostData, "quit", postValue))
    {
        postVars.quit = true;
    }

    if(pServer->getParam(szPostData, "ID", postValue))
    {
        postVars.id = postValue;
    }

    if(pServer->getParam(szPostData, "PASS", postValue))
    {
        postVars.pass = postValue;
    }

    if(pServer->getParam(szPostData, "idsave", postValue) && "on" != postValue)
    {
        postVars.idsave.Clear();
    }

    if(pServer->getParam(szPostData, "cv", postValue))
    {
        postVars.cv = postValue;
    }

    // Copy this POST variable.
    postVars.cvDisp = postVars.cv;

    // Get the required client version.
    uint32_t clientVersion = static_cast<uint32_t>(
        mConfig->GetClientVersion() * 1000.0f);

    std::pair<libcomp::String, libcomp::String> sids;

    if(postVars.cv != libcomp::String("%1.%2").Arg(clientVersion / 1000).Arg(
        clientVersion % 1000))
    {
        postVars.cvDisp += "&nbsp;<span style=\"font-weight:bold;color:#edb81e;\">"
            "~ Client needs to be updated ~</span>";
        postVars.msg = "<span style=\"font-size:12px;color:#edb81e;"
        "font-weight:bold;\"><br>&nbsp;You must update the "
        "client before you can login.</span>";
        postVars.submit = "<input class=\"login_disabled\" type=\"submit\" "
            "value=\"\" tabindex=\"4\" name=\"login\" height=\"60\" "
            "width=\"67\" />";
        postVars.idReadOnly = "readonly=\"readonly\" ";
        postVars.passReadOnly = "readonly=\"readonly\" ";
        postVars.idsaveReadOnly = "readonly=\"readonly\" ";

        postVars.id.Clear();
        postVars.pass.Clear();
    }
    else if(pServer->getParam(szPostData, "login", postValue))
    {
        // Get the information for this account.
        auto account = objects::Account::LoadAccountByUsername(mDatabase, postVars.id);

        // Check the password.
        if(nullptr != account && account->GetPassword() ==
            libcomp::Decrypt::HashPassword(postVars.pass, account->GetSalt()))
        {
            // If an expired session exists, expire it and log the user out
            if(mSessionManager->HasExpiredSession(postVars.id))
            {
                int8_t loginWorldID;
                mAccountManager->IsLoggedIn(postVars.id, loginWorldID);
                mSessionManager->ExpireSession(postVars.id);
                mAccountManager->LogoutUser(postVars.id, loginWorldID);
            }

            // Make sure the account is not logged in.
            if(mAccountManager->LoginUser(postVars.id))
            {
                // Generate the session IDs.
                sids = mSessionManager->GenerateSIDs(postVars.id);

                // Make sure we logout and check the SIDs.
                if(mAccountManager->LogoutUser(postVars.id) &&
                    !sids.first.IsEmpty() && !sids.second.IsEmpty())
                {
                    postVars.auth = true;
                }
            }

            if(!postVars.auth)
            {
                postVars.msg = "<span style=\"font-size:12px;color:#edb81e;"
                    "font-weight:bold;\"><br>&nbsp;Account is already "
                    "logged in.</span>";
            }
        }
        else
        {
            postVars.msg = "<span style=\"font-size:12px;color:#edb81e;"
                "font-weight:bold;\"><br>&nbsp;Invalid username "
                "or password.</span>";
        }
    }

    // Do some extra things if the user has been authenticated.
    if(postVars.auth)
    {
        // Change the POST variable into an integer for the auth page.
        if(postVars.idsave == "checked")
        {
            postVars.idsave = "1";
        }
        else
        {
            postVars.idsave = "0";
        }

        // The session IDs need to be generated.
        postVars.sid1 = sids.first;
        postVars.sid2 = sids.second;
    }

    // Free.
    delete[] szPostData;
}

bool LoginHandler::HandlePage(CivetServer *pServer,
    struct mg_connection *pConnection, ReplacementVariables& postVars)
{
    (void)pServer;

    libcomp::String uri("index.html");

    const mg_request_info *pRequestInfo = mg_get_request_info(pConnection);

    // Sanity check the request info.
    if(nullptr == pRequestInfo)
    {
        return false;
    }

    // Resolve a "/" URI into "/index.html".
    if(nullptr != pRequestInfo->local_uri &&
        libcomp::String("/") != pRequestInfo->local_uri)
    {
        uri = pRequestInfo->local_uri;
    }

    // Change the page if needed.
    if(postVars.quit)
    {
        uri = "quit.html";
    }
    else if(postVars.auth)
    {
        uri = "authenticated.html";
    }

    // Remove any leading slash.
    if("/" == uri.Left(1))
    {
        uri = uri.Mid(1);
    }

    LOG_DEBUG(libcomp::String("URI: %1\n").Arg(uri));

    // Attempt to load the URI.
    std::vector<char> pageData = LoadVfsFile(uri);

    // Make sure the page was loaded or return a 404.
    if(pageData.empty())
    {
        return false;
    }

    if(".png" == uri.Right(strlen(".png")))
    {
        mg_printf(pConnection, "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/png; charset=UTF-8\r\n"
            "Content-Length: %u\r\n"
            "Connection: close\r\n"
            "\r\n", (unsigned int)pageData.size());
        mg_write(pConnection, &pageData[0], pageData.size());
    }
    else
    {
        libcomp::String page(&pageData[0], pageData.size());

        // Replace our variables
        page = page.Replace("{COMP_HACK_MSG}",
            postVars.msg);
        page = page.Replace("{COMP_HACK_SUBMIT}",
            postVars.submit);
        page = page.Replace("{COMP_HACK_ID}",
            postVars.id);
        page = page.Replace("{COMP_HACK_ID_READONLY}",
            postVars.idReadOnly);
        page = page.Replace("{COMP_HACK_PASS}",
            postVars.pass);
        page = page.Replace("{COMP_HACK_PASS_READONLY}",
            postVars.passReadOnly);
        page = page.Replace("{COMP_HACK_IDSAVE}",
            postVars.idsave);
        page = page.Replace("{COMP_HACK_IDSAVE_READONLY}",
            postVars.idsaveReadOnly);
        page = page.Replace("{COMP_HACK_BIRTHDAY}",
            postVars.birthday);
        page = page.Replace("{COMP_HACK_CV_INPUT}",
            postVars.cv);
        page = page.Replace("{COMP_HACK_CV}",
            postVars.cvDisp);
        page = page.Replace("{COMP_HACK_SID1}",
            postVars.sid1);
        page = page.Replace("{COMP_HACK_SID2}",
            postVars.sid2);

        mg_printf(pConnection, "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %u\r\n"
            "Connection: close\r\n"
            "\r\n", (unsigned int)page.Size());
        mg_write(pConnection, page.C(), page.Size());
    }

    return true;
}

std::vector<char> LoginHandler::LoadVfsFile(const libcomp::String& path)
{
    std::vector<char> data;

    ttvfs::File *vf = mVfs.GetFile(path.C());
    if(!vf)
    {
        LOG_ERROR(libcomp::String("Failed to find file: %1\n").Arg(path));

        return std::vector<char>();
    }

    size_t fileSize = static_cast<size_t>(vf->size());

    data.resize(fileSize);

    if(!vf->open("rb"))
    {
        LOG_ERROR(libcomp::String("Failed to open file: %1\n").Arg(path));

        return std::vector<char>();
    }

    if(fileSize != vf->read(&data[0], fileSize))
    {
        LOG_ERROR(libcomp::String("Failed to read file: %1\n").Arg(path));

        return std::vector<char>();
    }

    return data;
}

void LoginHandler::SetAccountManager(AccountManager *pManager)
{
    mAccountManager = pManager;
}

void LoginHandler::SetSessionManager(SessionManager *pManager)
{
    mSessionManager = pManager;
}
