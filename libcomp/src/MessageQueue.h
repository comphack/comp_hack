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

#ifndef LIBCOMP_SRC_MESSAGEQUEUE_H
#define LIBCOMP_SRC_MESSAGEQUEUE_H

#include <list>
#include <mutex>
#include <condition_variable>

namespace libcomp
{

template<class T>
class MessageQueue
{
public:
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

    void DequeueAny(std::list<T>& destinationQueue)
    {
        std::list<T> tempQueue;

        mQueueLock.lock();
        mQueue.swap(tempQueue);
        mQueueLock.unlock();

        destinationQueue.splice(destinationQueue.end(), tempQueue);
    }

private:
    std::list<T> mQueue;
    std::mutex mQueueLock;
    std::mutex mEmptyConditionLock;
    std::condition_variable mEmptyCondition;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_MESSAGEQUEUE_H
