/**
 * @file libcomp/src/DayCare.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Big brother keeps the little monsters under control (sorta).
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#include "DayCare.h"
#include "SpawnThread.h"
#include "WatchThread.h"

// Standard C++11 Includes
#include <algorithm>

using namespace libcomp;

DayCare::DayCare(bool printDetails, std::function<void()> onDetain) :
    mRunning(true), mPrintDetails(printDetails),
    mSpawnThread(new SpawnThread(this, printDetails, onDetain)),
    mWatchThread(new WatchThread(this))
{
}

DayCare::~DayCare()
{
    delete mSpawnThread;
    mSpawnThread = nullptr;

    delete mWatchThread;
    mWatchThread = nullptr;
}

bool DayCare::DetainMonsters(const std::string& path)
{
    tinyxml2::XMLDocument doc;

    if(tinyxml2::XML_SUCCESS == doc.LoadFile(path.c_str()))
    {
        return LoadProcessDoc(doc);
    }

    return false;
}

bool DayCare::LoadProcessXml(const std::string& xml)
{
    tinyxml2::XMLDocument doc;

    if(tinyxml2::XML_SUCCESS == doc.Parse(xml.c_str()))
    {
        return LoadProcessDoc(doc);
    }

    return false;
}

bool DayCare::LoadProcessDoc(tinyxml2::XMLDocument& doc)
{
    std::list<std::shared_ptr<Child>> children;

    tinyxml2::XMLElement *pRoot = doc.FirstChildElement("programs");

    if(nullptr == pRoot)
    {
        return false;
    }

    tinyxml2::XMLElement *pProgram = pRoot->FirstChildElement("program");

    while(nullptr != pProgram)
    {
        tinyxml2::XMLElement *pPath = pProgram->FirstChildElement("path");

        if(nullptr == pPath)
        {
            return false;
        }

        const char *szPath = pPath->GetText();

        if(nullptr == szPath)
        {
            return false;
        }

        tinyxml2::XMLElement *pArg = pProgram->FirstChildElement("arg");

        std::list<std::string> arguments;

        while(nullptr != pArg)
        {
            const char *szArg = pArg->GetText();

            if(nullptr == szArg)
            {
                return false;
            }

            arguments.push_back(szArg);

            pArg = pArg->NextSiblingElement("arg");
        }

        int timeout = 0;
        bool restart = false;

        const char *szTimeout = pProgram->Attribute("timeout");
        const char *szRestart = pProgram->Attribute("restart");

        if(nullptr != szTimeout)
        {
            timeout = atoi(szTimeout);
        }

        if(nullptr != szRestart)
        {
            std::string s(szRestart);
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);

            if(s == "true" || s == "on" || s == "1" || s == "yes")
            {
                restart = true;
            }
        }

        auto child = std::make_shared<Child>(szPath, arguments,
            timeout, restart);
        children.push_back(child);

        pProgram = pProgram->NextSiblingElement("program");
    }

    std::lock_guard<std::mutex> guard(mChildrenShackles);

    mChildren = children;

    for(auto child : children)
    {
        mSpawnThread->QueueChild(child);
    }

    return true;
}

bool DayCare::IsRunning() const
{
    return mRunning;
}

bool DayCare::HaveChildren()
{
    bool isEmpty;

    {
        std::lock_guard<std::mutex> guard(mChildrenShackles);

        isEmpty = mChildren.empty();
    }

    return !isEmpty;
}

void DayCare::PrintStatus()
{
    std::lock_guard<std::mutex> guard(mChildrenShackles);

    for(auto child : mChildren)
    {
        printf("%d is runnning: %s\n", child->GetPID(),
            child->GetCommandLine().c_str());
    }
}

void DayCare::NotifyExit(pid_t pid, int status)
{
    std::shared_ptr<Child> child;

    std::lock_guard<std::mutex> guard(mChildrenShackles);

    for(auto c : mChildren)
    {
        if(pid == c->GetPID())
        {
            child = c;
            break;
        }
    }

    if(child)
    {
        if(mPrintDetails || 0 != status)
        {
            printf("%d exit with status %d: %s\n", pid, status,
                child->GetCommandLine().c_str());
        }

        if(mRunning && child->ShouldRestart())
        {
            mSpawnThread->QueueChild(child);
        }
        else
        {
            mChildren.remove(child);
        }
    }
}

std::list<std::shared_ptr<Child>> DayCare::OrderChildren(
    const std::list<std::shared_ptr<Child>>& children)
{
    std::list<std::shared_ptr<Child>> ordered;

    std::lock_guard<std::mutex> guard(mChildrenShackles);

    for(auto c : mChildren)
    {
        if(children.end() != std::find(children.begin(), children.end(), c))
        {
            ordered.push_back(c);
        }
    }

    return ordered;
}

void DayCare::CloseDoors(bool kill)
{
    mRunning = false;

    std::lock_guard<std::mutex> guard(mChildrenShackles);

    for(auto child : mChildren)
    {
        if(kill)
        {
            child->Kill();
        }
        else
        {
            child->Interrupt();
        }
    }

    mSpawnThread->RequestExit();
}

void DayCare::WaitForExit()
{
    mSpawnThread->WaitForExit();
    mWatchThread->WaitForExit();
}
