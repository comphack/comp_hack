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

#ifndef LIBCOMP_SRC_DAYCARE_H
#define LIBCOMP_SRC_DAYCARE_H

// libcomp Includes
#include "Child.h"

// Standard C++11 Includes
#include <functional>
#include <memory>
#include <mutex>

// tinyxml Includes
#include <tinyxml2.h>

namespace libcomp
{

class SpawnThread;
class WatchThread;

class DayCare
{
public:
    DayCare(bool printDetails = true,
        std::function<void()> onDetain = 0);
    ~DayCare();

    bool DetainMonsters(const std::string& path);
    bool LoadProcessXml(const std::string& xml);

    bool IsRunning() const;
    bool HaveChildren();
    void PrintStatus();

    void WaitForExit();
    void NotifyExit(pid_t pid, int status);
    void CloseDoors(bool kill = false);

    std::list<std::shared_ptr<Child>> OrderChildren(
        const std::list<std::shared_ptr<Child>>& children);

private:
    bool LoadProcessDoc(tinyxml2::XMLDocument& doc);

    bool mRunning;
    bool mPrintDetails;

    SpawnThread *mSpawnThread;
    WatchThread *mWatchThread;

    std::list<std::shared_ptr<Child>> mChildren;
    std::mutex mChildrenShackles;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DAYCARE_H
