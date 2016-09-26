/**
 * @file libcomp/src/InternalServerWorker.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Internal server worker class.
 *
 * This file is part of the COMP_hack Library (libcomp).
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

#ifndef LIBCOMP_SRC_INTERNALSERVERWORKER_H
#define LIBCOMP_SRC_INTERNALSERVERWORKER_H

 // libcomp Includes
#include "InternalConnection.h"

 // C++ Includes
#include <mutex>
#include <thread>
#include <vector>

namespace libcomp
{

class InternalServerWorker
{
public:
    InternalServerWorker();

    void Start();
    void Stop();

    void AddConnection(std::shared_ptr<TcpConnection> connection);

private:
    void doWork();
    bool executing();

    std::mutex mtx;
    bool stop = false;

    std::thread mThread;
    std::vector<std::shared_ptr<TcpConnection>> mConnections;
    std::vector<std::shared_ptr<TcpConnection>> mPendingConnections;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_INTERNALSERVERWORKER_H
