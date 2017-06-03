/**
 * @file libcomp/src/Exception.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the base Exception class.
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

#ifndef LIBCOMP_SRC_EXCEPTION_H
#define LIBCOMP_SRC_EXCEPTION_H

#include "CString.h"

namespace libcomp
{

/**
 * Base exception class. Any application specific exceptions should be derived
 * from this class. If an exception is not derived from this class (e.g. the
 * bad_alloc exception), it should be caught and either handled or thrown again
 * as an application exception using this class or any of it's derived classes.
 * When constructing an exception, use the @ref EXCEPTION macro for automatic
 * insertation of the file and line the exception occured on.
 */
class Exception
{
public:
    /**
     * Construct a basic exception. Pass __FILE__ as the @em file argument and
     * __LINE__ as the @em line argument. It is recommended you use the
     * @ref EXCEPTION macro instead.
     * @param msg Message describing the exception.
     * @param file File the exception occured in.
     * @param line Line the exception occured on.
     */
    Exception(const String& msg, const String& file, int line);

    /**
     * Retrieve the line number the exception occured on.
     * @returns Line number the exception was thrown from.
     */
    int Line() const;

    /**
     * Retrieve the file the exception occured in.
     * @returns File the exception was thrown from.
     */
    String File() const;

    /**
     * Retrieve the message describing the exception.
     * @returns Message describing the exception.
     */
    String Message() const;

    /**
     * Retrieve a backtrace of the exception.
     * @returns Backtrace of the exception.
     */
    std::list<String> Backtrace() const;

    /**
     * Generate a log message for the exception. Derived exception classes
     * should re-implement this function if they have more information related
     * to the exception.
     */
    virtual void Log() const;

    /**
     * Register the SIGSEGV signal handler to catch crashes.
     */
    static void RegisterSignalHandler();

private:
    /// Line number the exception occured on.
    int mLine;

    /// File the exception occured in.
    String mFile;

    /// Message describing the exception.
    String mMessage;

    /// Backtrace information for the exception.
    std::list<String> mBacktrace;
};

} // namespace libcomp

/**
 * Construct and throw a generic exception.
 * @param msg Message describing the exception.
 */
#define EXCEPTION(msg) throw libcomp::Exception(msg, __FILE__, __LINE__)

#endif // LIBCOMP_SRC_EXCEPTION_H
