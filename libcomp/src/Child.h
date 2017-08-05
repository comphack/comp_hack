/**
 * @file libcomp/src/Child.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to wrap and manage a child process.
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

#ifndef LIBCOMP_SRC_CHILD_H
#define LIBCOMP_SRC_CHILD_H

// Linux Includes
#include <unistd.h>

// Standard C++11 Includes
#include <list>
#include <string>

namespace libcomp
{

class Child
{
public:
    explicit Child(const std::string& program,
        const std::list<std::string>& arguments,
        int bootTimeout = 0, bool restart = false,
        bool displayOutput = false);
    ~Child();

    bool Start(bool notify = false);
    pid_t GetPID() const;
    std::string GetCommandLine() const;
    bool ShouldRestart() const;
    int GetBootTimeout() const;

    void Kill();
    void Interrupt();

private:
    std::string mProgram;
    std::list<std::string> mArguments;
    pid_t mPID;
    int mBootTimeout;
    bool mRestart;
    bool mDisplayOutput;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_CHILD_H
