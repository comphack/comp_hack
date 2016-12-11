/**
 * @file server/lobby/src/ManagerPacketInternal.cpp
 * @ingroup lobby
 *
 * @author HACKfrost
 *
 * @brief Manager to handle internal lobby packets.
 *
 * This file is part of the lobby Server (lobby).
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

#ifndef LIBCOMP_SRC_MANAGERPACKETINTERNAL_H
#define LIBCOMP_SRC_MANAGERPACKETINTERNAL_H

// channel Includes
#include "ManagerPacket.h"

namespace lobby
{
class ManagerPacketInternal : public ManagerPacket
{
public:
    ManagerPacketInternal(const std::shared_ptr<libcomp::BaseServer>& server);
    virtual ~ManagerPacketInternal();
};

} // namespace channel

#endif // LIBCOMP_SRC_MANAGERPACKETINTERNAL_H
