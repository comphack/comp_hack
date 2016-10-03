/**
 * @file libcomp/src/Worker.cpp
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

#include "Worker.h"

// libcomp Includes
#include "Log.h"

// Standard C++11 Includes
#include <thread>

using namespace libcomp;

Worker::Worker() : mRunning(false), mMessageQueue(new MessageQueue<
    Message::Message*>()), mThread(0)
{
}

Worker::~Worker()
{
    delete mThread;
}

void Worker::AddManager(const std::shared_ptr<Manager>& manager)
{
    for(auto messageType : manager->GetSupportedTypes())
    {
        mManagers[messageType] = manager;
    }
}

void Worker::Start()
{
    mThread = new std::thread([this](std::shared_ptr<MessageQueue<
        Message::Message*>> messageQueue)
    {
        mRunning = true;

        while(mRunning)
        {
            Run(messageQueue.get());
        }

        mRunning = false;
    }, mMessageQueue);
}

void Worker::Run(MessageQueue<Message::Message*> *pMessageQueue)
{
    std::list<libcomp::Message::Message*> msgs;
    pMessageQueue->DequeueAll(msgs);

    for(auto pMessage : msgs)
    {
        // Attempt to find a manager to process this message.
        auto it = mManagers.find(pMessage->GetType());

        // Process the message if the manager is valid.
        if(it == mManagers.end())
        {
            LOG_ERROR("Unhandled message type!\n");
        }
        else
        {
            auto manager = it->second;

            if(!manager)
            {
                LOG_ERROR("Manager is null!\n");
            }
            else if(!manager->ProcessMessage(pMessage))
            {
                LOG_ERROR("Failed to process message!\n");
            }
        }

        // Free the message now.
        delete pMessage;
    }
}

bool Worker::IsRunning() const
{
    return mRunning;
}

std::shared_ptr<MessageQueue<Message::Message*>> Worker::GetMessageQueue() const
{
    return mMessageQueue;
}
