/**
 * @file server/world/src/WorldHandler.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Civet world event handler.
 *
 * This file is part of the World Server (world).
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

#include "WorldHandler.h"

// world Includes
#include "ResourceLogin.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>

using namespace world;

WorldHandler::WorldHandler()
{
}

WorldHandler::~WorldHandler()
{
}

WorldHandler::ReplacementVariables::ReplacementVariables() : connecting(false)
{
}

bool WorldHandler::handleGet(CivetServer *pServer,
    struct mg_connection *pConnection)
{
    ReplacementVariables postVars;

    return HandleResponse(pServer, pConnection, postVars);
}

bool WorldHandler::handlePost(CivetServer *pServer,
    struct mg_connection *pConnection)
{
    ReplacementVariables postVars;

    ParsePost(pServer, pConnection, postVars);

    return HandleResponse(pServer, pConnection, postVars);
}

void WorldHandler::ParsePost(CivetServer *pServer,
    struct mg_connection *pConnection, ReplacementVariables& postVars)
{
    const mg_request_info *pRequestInfo = mg_get_request_info(pConnection);

    // Sanity check the request info.
    if(nullptr == pRequestInfo)
    {
        return;
    }

    size_t postContentLength = pRequestInfo->content_length;

    // Sanity check the post content length.
    if(0 == postContentLength)
    {
        return;
    }

    // Allocate.
    char *szPostData = new char[postContentLength + 1];

    // Read the post data.
    postContentLength = mg_read(pConnection, szPostData, postContentLength);
    szPostData[postContentLength] = 0;

    // Last read post value.
    std::string postValue;

    if(pServer->getParam(szPostData, "connecting", postValue))
    {
        postVars.connecting = true;
    }

    // Free.
    delete[] szPostData;
}

bool WorldHandler::HandleResponse(CivetServer *pServer,
    struct mg_connection *pConnection, ReplacementVariables& postVars)
{
    (void)pServer;

	libcomp::String reply("Unrecognized Request\r\n");
	if (postVars.connecting)
	{
		reply = libcomp::String("Connection Received\r\n");
	}

	mg_write(pConnection, reply.C(), reply.Size());

    return true;
}