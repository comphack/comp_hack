/**
 * @file libtester/src/ServerTest.cpp
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Function to assist with testing a suite of server applications.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
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

#include "ServerTest.h"

using namespace libtester;

ServerTestConfig::ServerTestConfig(
    const std::chrono::milliseconds& testTime,
    const std::chrono::milliseconds& bootTime,
    const std::string& programsPath,
    bool debug) : mTestTime(testTime), mBootTime(bootTime),
    mProgramsPath(programsPath), mDebug(debug)
{
}

std::chrono::milliseconds ServerTestConfig::GetTestTime() const
{
    return mTestTime;
}

std::chrono::milliseconds ServerTestConfig::GetBootTime() const
{
    return mBootTime;
}

std::string ServerTestConfig::GetProgramsPath() const
{
    return mProgramsPath;
}

bool ServerTestConfig::GetDebug() const
{
    return mDebug;
}
