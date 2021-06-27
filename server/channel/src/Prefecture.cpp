/**
 * @file server/channel/src/Prefecture.cpp
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Oversees a collection of zones assigned to a worker thread.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2021 COMP_hack Team <compomega@tutanota.com>
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

#include "Prefecture.h"

using namespace channel;

Prefecture::Prefecture(const std::weak_ptr<ChannelServer> &server)
    : mServer(server) {}

std::shared_ptr<ChannelServer> Prefecture::GetServer() const {
  return mServer.lock();
}
