/**
 * @file server/lobby/src/ImportHandler.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Civet API handler for account import.
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

#include "ImportHandler.h"

// libcomp Includes
#include <Log.h>

// object Includes
#include <Character.h>

// lobby Includes
#include "World.h"

using namespace lobby;

#define MAX_PAYLOAD (1 * 1024 * 1024) // 1 MiB

ImportHandler::ImportHandler(
    const std::shared_ptr<objects::LobbyConfig>& config,
    const std::shared_ptr<lobby::LobbyServer>& server) :
    mConfig(config), mServer(server)
{
}

ImportHandler::~ImportHandler()
{
}

bool ImportHandler::handlePost(CivetServer *pServer,
    struct mg_connection *pConnection)
{
    (void)pServer;

    // If import is disable ignore the request.
    if(!mConfig->GetAllowImport())
    {
        mg_printf(pConnection, "HTTP/1.1 401 Unauthorized\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    const mg_request_info *pRequestInfo = mg_get_request_info(pConnection);

    // Sanity check the request info.
    if(nullptr == pRequestInfo)
    {
        return false;
    }

    libcomp::String requestURI(pRequestInfo->request_uri);

    if("/import" != requestURI)
    {
        return false;
    }

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

    libcomp::String importError;

    // Import the account and collect the error.
    if(mServer)
    {
        /// @todo This is probably not a good way to do this.
        importError = mServer->ImportAccount(libcomp::String(
            szPostData).RightOf("\r\n\r\n").LeftOf("\r\n"),
            mConfig->GetImportWorld());
    }
    else
    {
        importError = "Internal error.";
    }

    delete[] szPostData;

    std::stringstream ss;
    JsonBox::Object response;

    if(importError.IsEmpty())
    {
        response["error"] = "Success";
    }
    else
    {
        response["error"] = importError.ToUtf8();
    }


    // Save the response and send it back to the client.
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
