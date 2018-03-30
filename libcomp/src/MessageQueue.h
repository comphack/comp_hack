/**
 * @file libcomp/src/MessageQueue.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Thread-safe message queue.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_MESSAGEQUEUE_H
#define LIBCOMP_SRC_MESSAGEQUEUE_H

#include <list>
#include <mutex>
#include <condition_variable>

namespace libcomp
{

/**
 * A thread safe collection of @ref Message instances to be created and
 * handled by a server. Messages queues are shared by both server
 * @ref Worker instances as well as each @ref EncryptedConnection that
 * connects to the server but is not limited to this usage.
 */
template<class T>
class MessageQueue
{
public:
    /**
     * Enqueue a message.
     * @param Message to add
     */
    void Enqueue(T item)
    {
        mQueueLock.lock();
        bool wasEmpty = mQueue.empty();
        mQueue.push_back(item);
        mQueueLock.unlock();

        if(wasEmpty)
        {
            std::unique_lock<std::mutex> uniqueLock(mEmptyConditionLock);
            mEmptyCondition.notify_one();
        }
    }

    /**
     * Enqueue multiple messages.
     * @param Messages to add
     */
    void Enqueue(std::list<T>& items)
    {
        mQueueLock.lock();
        bool wasEmpty = mQueue.empty();
        mQueue.splice(mQueue.end(), items);
        mQueueLock.unlock();

        if(wasEmpty)
        {
            std::unique_lock<std::mutex> uniqueLock(mEmptyConditionLock);
            mEmptyCondition.notify_one();
        }
    }

    /**
     * Dequeue the first message added and wait if empty.
     * @return The first message added
     */
    T Dequeue()
    {
        mQueueLock.lock();

        if(mQueue.empty())
        {
            mQueueLock.unlock();
            std::unique_lock<std::mutex> uniqueLock(mEmptyConditionLock);
            mEmptyCondition.wait(uniqueLock);
            mQueueLock.lock();
        }

        T item = mQueue.front();
        mQueue.pop_front();
        mQueueLock.unlock();

        return item;
    }

    /**
     * Dequeue all the messages and wait if its empty.
     * @param List to add the messages to
     */
    void DequeueAll(std::list<T>& destinationQueue)
    {
        std::list<T> tempQueue;

        mQueueLock.lock();

        if(mQueue.empty())
        {
            mQueueLock.unlock();
            std::unique_lock<std::mutex> uniqueLock(mEmptyConditionLock);
            mEmptyCondition.wait(uniqueLock);
            mQueueLock.lock();
        }

        mQueue.swap(tempQueue);
        mQueueLock.unlock();

        destinationQueue.splice(destinationQueue.end(), tempQueue);
    }

    /**
     * Dequeue all the current messages.
     * @param List to add the messages to
     */
    void DequeueAny(std::list<T>& destinationQueue)
    {
        std::list<T> tempQueue;

        mQueueLock.lock();
        mQueue.swap(tempQueue);
        mQueueLock.unlock();

        destinationQueue.splice(destinationQueue.end(), tempQueue);
    }

private:
    /// The list of messages
    std::list<T> mQueue;

    /// Mutex lock to use when modifying the queue
    std::mutex mQueueLock;

    /// Mutix lock to use when waiting for a message to be queued
    std::mutex mEmptyConditionLock;

    /// Blocking condition to wait for when no messages are queued
    std::condition_variable mEmptyCondition;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGEQUEUE_H
