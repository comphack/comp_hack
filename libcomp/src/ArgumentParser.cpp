/**
 * @file libcomp/src/ArgumentParser.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to handle parsing command line arguments for an application.
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

#include "ArgumentParser.h"

// libcomp Includes
#include "Log.h"

using namespace libcomp;

ArgumentParser::ArgumentParser()
{
}

ArgumentParser::~ArgumentParser()
{
    mShortParsers.clear();
    mLongParsers.clear();

    for(auto pArg : mArgumentParsers)
    {
        delete pArg;
    }

    mArgumentParsers.clear();
}

bool ArgumentParser::Parse(int argc, const char * const argv[])
{
    std::vector<String> arguments;

    for(int i = 1; i < argc; ++i)
    {
        String arg(argv[i]);

        if(!arg.IsEmpty())
        {
            arguments.push_back(arg);
        }
    }

    return Parse(arguments);
}

bool ArgumentParser::Parse(const std::vector<String>& arguments)
{
    std::vector<String> standardArgs;

    std::vector<String>::size_type argc = arguments.size();

    for(std::vector<String>::size_type i = 0; i < argc; ++i)
    {
        const String& arg = arguments[i];

        std::vector<String> matches;

        if(arg.Matches("^--([^=]+)=(.+)$", matches))
        {
            auto match = mLongParsers.find(matches[1]);

            if(mLongParsers.end() == match)
            {
                LOG_ERROR(String("Unknown command line option %1\n").Arg(
                    matches[1]));

                return false;
            }

            auto pArg = match->second;

            if(ArgumentType::NONE == pArg->argType)
            {
                LOG_ERROR(String("Command line option %1 can't have an "
                    "argument.\n").Arg(matches[1]));

                return false;
            }

            if(pArg->handler && !pArg->handler(pArg, matches[2]))
            {
                return false;
            }
        }
        else if(2 <= arg.Length() && "--" == arg.Left(2))
        {
            auto match = mLongParsers.find(arg.Mid(2));

            if(mLongParsers.end() == match)
            {
                LOG_ERROR(String("Unknown command line option %1\n").Arg(
                    arg));

                return false;
            }

            auto pArg = match->second;

            if(ArgumentType::OPTIONAL == pArg->argType && i < (argc - 1))
            {
                if(pArg->handler && !pArg->handler(pArg,
                    arguments[++i]))
                {
                    return false;
                }
            }
            else if(ArgumentType::REQUIRED == pArg->argType)
            {
                if(i >= (argc - 1))
                {
                    LOG_ERROR(String("Command line option %1 requires "
                        "an argument.\n").Arg(arg));

                    return false;
                }

                if(pArg->handler && !pArg->handler(pArg,
                    arguments[++i]))
                {
                    return false;
                }
            }
            else if(pArg->handler && !pArg->handler(pArg, String()))
            {
                return false;
            }
        }
        else if(!arg.IsEmpty() && '-' == arg.At(0))
        {
            for(auto opt : arg.Mid(1).ToUtf8())
            {
                auto match = mShortParsers.find(opt);

                if(mShortParsers.end() == match)
                {
                    LOG_ERROR(String("Unknown command line option -%1\n").Arg(
                        std::string(1, opt)));

                    return false;
                }

                auto pArg = match->second;

                if(ArgumentType::NONE != pArg->argType && 2 < arg.Length())
                {
                    LOG_ERROR(String("Multiple short arguments can't be "
                        "specified together if any of them can have "
                        "an argument: %1\n").Arg(arg));

                    return false;
                }

                if(ArgumentType::OPTIONAL == pArg->argType && i < (argc - 1))
                {
                    if(pArg->handler && !pArg->handler(pArg,
                        arguments[++i]))
                    {
                        return false;
                    }
                }
                else if(ArgumentType::REQUIRED == pArg->argType)
                {
                    if(i >= (argc - 1))
                    {
                        LOG_ERROR(String("Command line option -%1 requires "
                            "an argument.\n").Arg(std::string(1, opt)));

                        return false;
                    }

                    if(pArg->handler && !pArg->handler(pArg,
                        arguments[++i]))
                    {
                        return false;
                    }
                }
                else if(pArg->handler && !pArg->handler(pArg,
                    String()))
                {
                    return false;
                }
            }
        }
        else
        {
            standardArgs.push_back(arg);
        }
    }

    return ParseStandardArguments(standardArgs);
}

void ArgumentParser::RegisterArgument(char shortName, const String& longName,
    ArgumentType argType, std::function<bool(Argument*,
        const String&)> handler)
{
    Argument *pArg = new Argument;
    pArg->shortName = shortName;
    pArg->longName = longName;
    pArg->argType = argType;
    pArg->handler = handler;

    mArgumentParsers.push_back(pArg);
    mShortParsers[shortName] = pArg;

    if(!longName.IsEmpty())
    {
        mLongParsers[longName] = pArg;
    }
}

bool ArgumentParser::ParseStandardArguments(
    const std::vector<String>& arguments)
{
    mStandardArguments = arguments;

    return true;
}

std::vector<String> ArgumentParser::GetStandardArguments() const
{
    return mStandardArguments;
}
