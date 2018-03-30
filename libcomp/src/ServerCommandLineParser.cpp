/**
 * @file libcomp/src/ServerCommandLineParser.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to handle parsing command line arguments for a server.
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

#include "ServerCommandLineParser.h"

// libcomp Includes
#include "Log.h"

using namespace libcomp;

ServerCommandLineParser::ServerCommandLineParser() : ArgumentParser(),
    objects::ServerCommandLine()
{
    RegisterArgument('\0', "test", ArgumentType::NONE, std::bind([](
        ServerCommandLineParser *pParser, ArgumentParser::Argument *pArg,
            const String& arg) -> bool
    {
        (void)pArg;
        (void)arg;

        pParser->SetTestingEnabled(true);

        return true;
    }, this, std::placeholders::_1, std::placeholders::_2));

    RegisterArgument('\0', "notify", ArgumentType::REQUIRED, std::bind([](
        ServerCommandLineParser *pParser, ArgumentParser::Argument *pArg,
            const String& arg) -> bool
    {
        (void)pArg;

        bool ok = false;

        pParser->SetNotifyProcess(arg.ToInteger<int32_t>(&ok));

        if(!ok)
        {
            LOG_ERROR(String("Invalid process ID %1\n").Arg(arg));
        }

        return ok;
    }, this, std::placeholders::_1, std::placeholders::_2));
}

ServerCommandLineParser::~ServerCommandLineParser()
{
}
