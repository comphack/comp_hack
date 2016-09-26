/**
 * @file libcomp/src/InternalServerWorker.cpp
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

#include "InternalServerWorker.h"

 // libcomp Includes
#include "Log.h"

using namespace libcomp;

InternalServerWorker::InternalServerWorker()
{
}

void InternalServerWorker::Start()
{
    mThread = std::thread([this] { doWork(); });
}

void InternalServerWorker::Stop()
{
    mtx.lock();
    stop = true;
    mtx.unlock();

    mThread.join();
}

void InternalServerWorker::AddConnection(std::shared_ptr<TcpConnection> connection)
{
    mtx.lock();
    mPendingConnections.push_back(connection);
    mtx.unlock();
}

void InternalServerWorker::doWork()
{
    while (executing())
    {
        //Check pending updates from server
        mtx.lock();

        //Check for new connections
        if (mPendingConnections.size() > 0)
        {
            int newCount = mPendingConnections.size();
            LOG_DEBUG("[Worker] Adding " + std::to_string(newCount) + " new " + (newCount != 1 ? "connections" : "connection") + "\n");
            mConnections.insert(mConnections.end(), mPendingConnections.begin(), mPendingConnections.end());
            mPendingConnections.clear();
        }
        mtx.unlock();

        //Clean up
        for (int i = mConnections.size() - 1; i >= 0; i--)
        {
            //Remove disconnected connections
            int x = 0;
            if (mConnections[i]->GetStatus() == TcpConnection::STATUS_NOT_CONNECTED)
            {
                mConnections.erase(mConnections.begin() + i);
                x++;
            }

            if (x > 0)
            {
                LOG_DEBUG("[Worker] Cleaning up " + std::to_string(x) + (x != 1 ? " connections" : " connection") + "\n");
            }
        }

        //todo: replace with?
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool InternalServerWorker::executing()
{
    mtx.lock();
    bool e = !stop;
    mtx.unlock();
    return e;
}