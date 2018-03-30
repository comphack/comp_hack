/**
 * @file libcomp/src/Log.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Routines to log messages to the console and/or a file.
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

#ifndef LIBCOMP_SRC_LOG_H
#define LIBCOMP_SRC_LOG_H

#include "CString.h"

#include <list>
#include <mutex>
#include <fstream>
#include <functional>
#include <unordered_map>

namespace libcomp
{

/**
 * Logging interface capable of logging messages to the terminal or a file.
 * The Log class is implemented as a singleton. The constructor should not be
 * called and is protected because of this. Instead, the first call to
 * @ref GetSingletonPtr() will construct the object. Subsequent calls will
 * simply return a pointer to the existing object. The object should only be
 * deleted once at the end of the application or not at all. The method
 * @ref SetLogPath() will open and initialize the log file. Initialization of
 * the log subsystem can be done with the following code:
 *
 * @code
 * Log::GetSingletonPtr()->SetLogPath("/var/log/my.log");
 * @endcode
 *
 * There is currently only one log file created. There is no compression or
 * rotation of log files. The logging subsystem consists of five different log
 * levels. Each level has a macro that saves typing of GetSingletonPtr to log a
 * simple message. These macros are @ref LOG_CRITICAL, @ref LOG_ERROR,
 * @ref LOG_WARNING, @ref LOG_INFO, and @ref LOG_DEBUG. Each log level can be
 * omitted from the log file by setting @ref SetLogLevelEnabled.
 *
 * Log hooks can be implemented to process log messages differently. All hooks
 * must conform to the @ref Log::Hook_t function prototype. This consists of a
 * log level, the message, and the user data provided by the @ref AddLogHook
 * method. For more information on the function prototype, see the docs for
 * @ref Log::Hook_t and @ref AddLogHook.
 */
class Log
{
public:
    /**
     * All valid log levels.
     */
    typedef enum
    {
        LOG_LEVEL_DEBUG = 0,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_CRITICAL,
        LOG_LEVEL_COUNT,
    } Level_t;

    /**
     * Prototype of a function to be called when a log event occurs. When a log
     * message is generated, @em level describes the error level of the message,
     * @em msg contains the data, and @em pUserData is optional user defined
     * data passed to @ref Log::AddLogHook along with the function. See
     * @ref Level_t for all possible log levels.
     */
    typedef void (*Hook_t)(Level_t level, const String& msg, void *pUserData);

    /**
     * Deconstruct and delete the Log singleton.
     */
    ~Log();

    /**
     * Return a pointer to the Log singleton. If the singleton has not been
     * created, this method will create the singleton first.
     * @returns Pointer to the Log singleton. This method should never return
     * a null pointer.
     */
    static Log* GetSingletonPtr();

    /**
     * Log a message.
     * @param level Logging level of the message.
     * @param msg The message to log.
     */
    void LogMessage(Level_t level, const String& msg);

    /**
     * Get the path to the log file.
     * @returns Path to the log file.
     */
    String GetLogPath() const;

    /**
     * Set the path to the log file. This will open the log file and optionally
     * truncate.
     * @param path Path to the log file.
     * @param truncate true if the file should be truncated, false if it should
     * append to the existing file.
     */
    void SetLogPath(const String& path, bool truncate);

    /**
     * Add a log hook to the logging subsystem. The log hook @em func will be
     * called for each new log message that is enabled through
     * @ref SetLogLevelEnabled. The log level, message, and user data passed to
     * this method as @em pUserData will be provided to the log hook function.
     * @param func Log hook function to call.
     * @param pUserData User defined data to pass to the log hook function.
     */
    void AddLogHook(Hook_t func, void *pUserData = 0);

    /**
     * Add a log hook to the logging subsystem. The log hook @em func will be
     * called for each new log message that is enabled through
     * @ref SetLogLevelEnabled. The log level and message will be provided to
     * the log hook function.
     * @param func Log hook function to call.
     */
    void AddLogHook(const std::function<void(Level_t level,
        const String& msg)>& func);

    /**
     * Add the built-in hook to log to standard output.
     */
    void AddStandardOutputHook();

    /**
     * Remove all log hooks.
     */
    void ClearHooks();

    /**
     * Get if the specified logging level is enabled.
     * @param level A logging level.
     * @returns true if logging for the level is enabled.
     */
    bool GetLogLevelEnabled(Level_t level) const;

    /**
     * Set if the specified logging level is enabled.
     * @param level A logging level.
     * @param enabled If logging for the level is enabled.
     */
    void SetLogLevelEnabled(Level_t level, bool enabled);

    /**
     * Get if the log file timestamps are enabled.
     * @return true if file logging timestamps are enabled.
     */
    bool GetLogFileTimestampsEnabled() const;

    /**
     * Set if the log file timestamps are enabled.
     * @param enabled If file logging timestamps are enabled.
     */
    void SetLogFileTimestampsEnabled(bool enabled);

protected:
    /**
     * @internal
     * Construct a Log object. This constructor is protected because it should
     * not be called directly. Instead, call @ref GetSingletonPtr and let the
     * method construct the object if it doesn't already exist. This enforces
     * the singleton design pattern.
     * @sa GetSingletonPtr
     */
    Log();

    /**
     * @internal
     * Path to the log file.
     */
    String mLogPath;

    /**
     * @internal
     * Whether to log messages for each level.
     */
    bool mLogEnables[LOG_LEVEL_COUNT];

    /**
     * @internal
     * Whether log file messages will contain a timestamp.
     */
    bool mLogFileTimestampEnabled;

    /**
     * @internal
     * Log file object that messages will be written to.
     */
    std::ofstream *mLogFile;

    /**
     * @internal
     * Mapping of log hooks and their associated user data.
     */
    std::unordered_map<Hook_t, void*> mHooks;

    /**
     * @internal
     * List of log hooks.
     */
    std::list<std::function<void(Level_t level,
        const String& msg)>> mLambdaHooks;

    /**
     * @internal
     * Mutex to make the log thread safe.
     */
    std::mutex mLock;

#ifdef _WIN32
    /**
     * @internal
     * Windows standard output console color attributes.
     */
    uint16_t mConsoleAttributes;
#endif
};

} // namespace libcomp

/**
 * %Log a critical error message.
 * @param msg The message to log.
 * @sa Log::LogMessage
 * @relates Log
 */
#define LOG_CRITICAL(msg) libcomp::Log::GetSingletonPtr()->LogMessage( \
    libcomp::Log::LOG_LEVEL_CRITICAL, msg)

/**
 * %Log an error message.
 * @param msg The message to log.
 * @sa Log::LogMessage
 * @relates Log
 */
#define LOG_ERROR(msg)    libcomp::Log::GetSingletonPtr()->LogMessage( \
    libcomp::Log::LOG_LEVEL_ERROR, msg)

/**
 * %Log a warning message.
 * @param msg The message to log.
 * @sa Log::LogMessage
 * @relates Log
 */
#define LOG_WARNING(msg)  libcomp::Log::GetSingletonPtr()->LogMessage( \
    libcomp::Log::LOG_LEVEL_WARNING, msg)

/**
 * %Log a informational message.
 * @param msg The message to log.
 * @sa Log::LogMessage
 * @relates Log
 */
#define LOG_INFO(msg)     libcomp::Log::GetSingletonPtr()->LogMessage( \
    libcomp::Log::LOG_LEVEL_INFO, msg)

/**
 * %Log a debug message.
 * @param msg The message to log.
 * @sa Log::LogMessage
 * @relates Log
 */
#define LOG_DEBUG(msg)    libcomp::Log::GetSingletonPtr()->LogMessage( \
    libcomp::Log::LOG_LEVEL_DEBUG, msg)

#endif // LIBCOMP_SRC_LOG_H
