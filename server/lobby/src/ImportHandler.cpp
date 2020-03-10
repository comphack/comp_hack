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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#define MAX_PAYLOAD (5 * 1024 * 1024) // 5 MiB

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
        LogWebAPIErrorMsg(libcomp::String("API payload size of %1 bytes "
            "rejected.\n").Arg(postContentLength));

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

    const char *szContentType = mg_get_header(pConnection, "Content-Type");

    if(!szContentType)
    {
        mg_printf(pConnection, "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n\r\n");

        return true;
    }

    // Extract the file from the POST data.
    libcomp::String importData = ExtractFile(szContentType, szPostData);

    delete[] szPostData;

    // Import the account and collect the error.
    if(mServer)
    {
        if( !importData.IsEmpty() )
        {
            importError = mServer->ImportAccount(importData,
                mConfig->GetImportWorld());
        }
        else
        {
            mg_printf(pConnection, "HTTP/1.1 400 Bad Request\r\n"
                "Connection: close\r\n\r\n");

            return true;
        }
    }
    else
    {
        importError = "Internal error.";
    }

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

libcomp::String ImportHandler::ExtractFile(const libcomp::String& contentType,
    const libcomp::String& contentData)
{
    libcomp::String boundary;

    auto data = libcomp::String("\r\n") + contentData;

    for(auto _s : contentType.Split(";"))
    {
        auto s = _s.Trimmed();

        if("boundary=" == s.Left(strlen("boundary=")))
        {
            // Grab the boundary value.
            boundary = s.Mid(strlen("boundary="));

            // Remove the end boundary.
            data = data.LeftOf(libcomp::String("\r\n--%1--\r\n").Arg(boundary));

            // Save out the proper boundary.
            boundary = libcomp::String("\r\n--%1\r\n").Arg(
                boundary);

            // Put back a normal boundary at the end.
            data += boundary;

            break;
        }
    }

    if(boundary.IsEmpty())
    {
        return {};
    }

    while(!data.IsEmpty())
    {
        // Parse each part of the multi-part form.
        auto part = data.LeftOf(boundary);
        auto headers = part.LeftOf("\r\n\r\n").Split("\r\n");
        part = part.RightOf("\r\n\r\n");

        // Look in the headers to see if this is a file.
        for(auto header : headers)
        {
            if("Content-Disposition:" ==
                header.Left(strlen("Content-Disposition:")))
            {
                for(auto keyValuePair : header.RightOf(
                    "Content-Disposition:").Split(";"))
                {
                    auto pair = keyValuePair.Trimmed().Split("=");

                    // If this is the first file return the data.
                    if(!pair.empty() && "filename" == pair.front())
                    {
                        return part;
                    }
                }
            }
        }

        data = data.RightOf(boundary);
    }

    return {};
}
