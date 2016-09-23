/**
 * @file server/channel/src/ChannelHandler.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Civet channel event handler.
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_CHANNELHANDLER_H
#define SERVER_CHANNEL_SRC_CHANNELHANDLER_H

// Civet Includes
#include <CivetServer.h>

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <vector>

// VFS Includes
#include "PushIgnore.h"
#include <ttvfs/ttvfs.h>
#include <ttvfs/ttvfs_zip.h>
#include "PopIgnore.h"

namespace channel
{

class ChannelHandler : public CivetHandler
{
public:
    ChannelHandler();
    virtual ~ChannelHandler();

    virtual bool handleGet(CivetServer *pServer,
        struct mg_connection *pConnection);

    virtual bool handlePost(CivetServer *pServer,
        struct mg_connection *pConnection);

private:
    class ReplacementVariables
    {
    public:
        ReplacementVariables();

        bool connecting;
    };

    void ParsePost(CivetServer *pServer, struct mg_connection *pConnection,
        ReplacementVariables& postVars);

    bool HandleResponse(CivetServer *pServer, struct mg_connection *pConnection,
        ReplacementVariables& postVars);
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_CHANNELHANDLER_H
