/**
 * @file libcomp/src/Worker.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base worker class to process messages for a thread.
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

#ifndef LIBCOMP_SRC_WORKER_H
#define LIBCOMP_SRC_WORKER_H

// libcomp Includes
#include "EnumMap.h"
#include "Manager.h"
#include "Message.h"
#include "MessageQueue.h"

// Standard C++11 Includes
#include <list>
#include <memory>
#include <thread>
#include <unordered_map>

namespace libcomp
{

class Worker
{
public:
    Worker();
    virtual ~Worker();

    /**
     * Add a manager to process messages.
     */
    void AddManager(const std::shared_ptr<Manager>& manager);

    /**
     * @brief Start the worker thread.
     */
    void Start(bool blocking = false);

    /**
     * @brief Main loop of the worker thread.
     */
    virtual void Run(libcomp::MessageQueue<
        libcomp::Message::Message*> *pMessageQueue);

    virtual void Shutdown();
    virtual void Join();

    bool IsRunning() const;

    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> GetMessageQueue() const;

    long AssignmentCount() const;

protected:
    virtual void Cleanup();

private:
    bool mRunning;
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;
    EnumMap<Message::MessageType,
        std::shared_ptr<Manager>> mManagers;
    std::thread *mThread;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_WORKER_H
