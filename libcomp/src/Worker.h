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
#include "MessageExecute.h"
#include "MessageQueue.h"

// Standard C++11 Includes
#include <list>
#include <memory>
#include <thread>
#include <unordered_map>

namespace libcomp
{

/**
 * Generic worker assigned to a message queue used to handle messages as
 * they are received.  Workers can run syncronously or in their own thread
 * and should be shutdown at the same time the executing server does.
 * @sa BaseServer
 */
class Worker
{
public:
    /**
     * Create a new worker.
     */
    Worker();

    /**
     * Cleanup the worker.
     */
    virtual ~Worker();

    /**
     * Add a manager to process messages.
     * @param manager A message manager
     */
    void AddManager(const std::shared_ptr<Manager>& manager);

    /**
     * Loop until stopped, making a call to @ref Worker::Run.
     * @param blocking If false a new thread will be started
     *  to run this function asynchronously
     */
    void Start(bool blocking = false);

    /**
     * Wait for a message to enter the queue then handle it
     * with the appropriate @ref Manager configured for the
     * worker.
     * @param pMessageQueue Queue to check for messages
     */
    virtual void Run(libcomp::MessageQueue<
        libcomp::Message::Message*> *pMessageQueue);

    /**
     * Signal that the worker should shutdown by sending a
     * @ref Message::Shutdown.
     */
    virtual void Shutdown();

    /**
     * Join the thread used for asynchronous execution.
     */
    virtual void Join();

    /**
     * Check if the worker is currently running.
     * @return true if it is running, false if it is not
     */
    bool IsRunning() const;

    /**
     * Get the message queue assinged to the worker.
     * @return Assigned message queue
     */
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> GetMessageQueue() const;

    /**
     * Get the number of active references to the message queue
     * assigned to the worker.
     * @sa BaseServer::GetNextConnectionWorker
     * @return The number of active references to the message queue
     */
    long AssignmentCount() const;

    /**
     * Executes code in the worker thread.
     * @param f Function (lambda) to execute in the worker thread.
     * @param args Arguments to pass to the function when it is executed.
     * @return true on success, false on failure
     */
    template<typename Function, typename... Args>
    bool ExecuteInWorker(Function&& f, Args&&... args) const
    {
        auto queue = GetMessageQueue();

        if(nullptr != queue)
        {
            auto msg = new libcomp::Message::ExecuteImpl<Args...>(
                std::forward<Function>(f), std::forward<Args>(args)...);
            queue->Enqueue(msg);

            return true;
        }

        return false;
    }

protected:
    /**
     * Clean up the worker, deleting the thread if it exists and resetting
     * the message queue.  This is called by the destructor.
     */
    virtual void Cleanup();

private:
    /// Signifier that the worker should continue running
    bool mRunning;

    /// Message queue to retrieve messages from
    std::shared_ptr<libcomp::MessageQueue<
        libcomp::Message::Message*>> mMessageQueue;

    /// Map of pointers to message handlers mapped by message type
    EnumMap<Message::MessageType,
        std::shared_ptr<Manager>> mManagers;

    /// Thread used to handle asynchronous execution
    std::thread *mThread;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_WORKER_H
