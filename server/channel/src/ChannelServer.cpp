/**
 * @file server/channel/src/ChannelServer.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel server class.
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

#include "ChannelServer.h"

// channel Includes
#include "InternalConnection.h"

using namespace channel;

ChannelServer::ChannelServer(libcomp::String listenAddress, uint16_t port) :
    libcomp::InternalServer(listenAddress, port)
{
    //todo: add worker managers

    //Start the workers
    mWorker.Start();
}

ChannelServer::~ChannelServer()
{
}