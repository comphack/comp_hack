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

#include "Worker.h"

// libcomp Includes
#include "Exception.h"
#include "Log.h"
#include "MessageShutdown.h"

// Standard C++11 Includes
#include <thread>

using namespace libcomp;

Worker::Worker() : mRunning(false), mMessageQueue(new MessageQueue<
    Message::Message*>()), mThread(nullptr)
{
}

Worker::~Worker()
{
    Cleanup();
}

void Worker::AddManager(const std::shared_ptr<Manager>& manager)
{
    for(auto messageType : manager->GetSupportedTypes())
    {
        mManagers[messageType] = manager;
    }
}

void Worker::Start(const libcomp::String& name, bool blocking)
{
    if(blocking)
    {
        mRunning = true;

        while(mRunning)
        {
            Run(mMessageQueue.get());
        }

        mRunning = false;
    }
    else
    {
        mThread = new std::thread([this](std::shared_ptr<MessageQueue<
            Message::Message*>> messageQueue, const libcomp::String& _name)
        {
            (void)_name;

#if !defined(_WIN32)
            pthread_setname_np(pthread_self(), _name.C());
#endif // !defined(_WIN32)

            libcomp::Exception::RegisterSignalHandler();

            mRunning = true;

            while(mRunning)
            {
                Run(messageQueue.get());
            }

            mRunning = false;
        }, mMessageQueue, name);
    }
}

void Worker::Run(MessageQueue<Message::Message*> *pMessageQueue)
{
    std::list<libcomp::Message::Message*> msgs;
    pMessageQueue->DequeueAll(msgs);

    for(auto pMessage : msgs)
    {
        // Check for a shutdown message.
        libcomp::Message::Shutdown *pShutdown = dynamic_cast<
            libcomp::Message::Shutdown*>(pMessage);

        // Check for an execute message.
        libcomp::Message::Execute *pExecute = dynamic_cast<
            libcomp::Message::Execute*>(pMessage);

        // Do not handle any more messages if a shutdown was sent.
        if(nullptr != pShutdown || !mRunning)
        {
            mRunning = false;
        }
        else if(nullptr != pExecute)
        {
            // Run the code now.
            pExecute->Run();
        }
        else
        {
            // Attempt to find a manager to process this message.
            auto it = mManagers.find(pMessage->GetType());

            // Process the message if the manager is valid.
            if(it == mManagers.end())
            {
                LOG_ERROR(libcomp::String("Unhandled message type: %1\n").Arg(
                    static_cast<std::size_t>(pMessage->GetType())));
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
                    LOG_ERROR(libcomp::String("Failed to process message:\n"
                        "%1\n").Arg(pMessage->Dump()));
                }
            }
        }

        // Free the message now.
        delete pMessage;
    }
}

void Worker::Shutdown()
{
    mMessageQueue->Enqueue(new libcomp::Message::Shutdown());
}

void Worker::Join()
{
    if(nullptr != mThread)
    {
        mThread->join();
    }
}

void Worker::Cleanup()
{
    // Delete the main thread (if it exists).
    if(nullptr != mThread)
    {
        delete mThread;
        mThread = nullptr;
    }

    if(nullptr != mMessageQueue)
    {
        // Empty the message queue.
        std::list<libcomp::Message::Message*> msgs;
        mMessageQueue->DequeueAny(msgs);

        for(auto pMessage : msgs)
        {
            delete pMessage;
        }

        mMessageQueue.reset();
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

long Worker::AssignmentCount() const
{
    return mMessageQueue.use_count();
}
