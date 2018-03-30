/**
 * @file libcomp/src/ArgumentParser.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to handle parsing command line arguments for an application.
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

#ifndef LIBCOMP_SRC_ARGUMENTPARSER_H
#define LIBCOMP_SRC_ARGUMENTPARSER_H

// libcomp Includes
#include "CString.h"

// Standard C++ Includes
#include <functional>
#include <list>
#include <unordered_map>
#include <vector>

#if defined(_WIN32) && defined(OPTIONAL)
#undef OPTIONAL
#endif // defined(_WIN32) && defined(OPTIONAL)

namespace libcomp
{

/**
 * Class to handle parsing command line arguments for an application.
 */
class ArgumentParser
{
public:
    enum class ArgumentType
    {
        NONE,
        OPTIONAL,
        REQUIRED,
    };

    struct Argument
    {
        char shortName;
        String longName;
        ArgumentType argType;
        std::function<bool(Argument*, const String&)> handler;
    };

    ArgumentParser();
    virtual ~ArgumentParser();

    virtual bool Parse(int argc, const char * const argv[]);

    std::vector<String> GetStandardArguments() const;

protected:
    virtual bool Parse(const std::vector<String>& arguments);
    virtual bool ParseStandardArguments(
        const std::vector<String>& arguments);

    virtual void RegisterArgument(char shortName, const String& longName,
        ArgumentType argType, std::function<bool(Argument*,
            const String&)> handler);

private:
    std::unordered_map<char, Argument*> mShortParsers;
    std::unordered_map<String, Argument*> mLongParsers;
    std::list<Argument*> mArgumentParsers;

    std::vector<String> mStandardArguments;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_ARGUMENTPARSER_H
