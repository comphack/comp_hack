/**
 * @file libhack/src/Log.h
 * @ingroup libhack
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Routines to log messages to the console and/or a file.
 *
 * This file is part of the COMP_hack Library (libhack).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBHACK_SRC_LOG_H
#define LIBHACK_SRC_LOG_H

#include "BaseLog.h"

namespace libhack {

/**
 * Log components a log message may belong to.
 */
enum class LogComponent_t : libcomp::GenericLogComponent_t {
  AccountManager = LOG_SERVER_SPECIFIC_START_ID,
  ActionManager,
  AIManager,
  Barter,
  Bazaar,
  CharacterManager,
  ChatManager,
  Clan,
  DefinitionManager,
  Demon,
  EventManager,
  Friend,
  FusionManager,
  Item,
  MatchManager,
  Party,
  ServerConstants,
  ServerDataManager,
  SkillManager,
  Team,
  TokuseiManager,
  Trade,
  WebAPI,
  ZoneManager,
};

/**
 * Convert a string into a log component.
 * @param comp String to convert.
 * @returns Log component the string represents.
 */
LogComponent_t StringToLogComponent(const libcomp::String& comp);

/**
 * Convert a log component into a string.
 * @param comp Log component to convert.
 * @returns String representation of the log component.
 */
libcomp::String LogComponentToString(libcomp::GenericLogComponent_t comp);

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
class Log : public libcomp::BaseLog {
 public:
  /**
   * Deconstruct and delete the Log singleton.
   */
  virtual ~Log();

  /**
   * Return a pointer to the Log singleton. If the singleton has not been
   * created, this method will create the singleton first.
   * @returns Pointer to the Log singleton. This method should never return
   * a null pointer.
   */
  static libcomp::BaseLog* GetSingletonPtr();

  /**
   * Convert a string into a log component.
   * @param comp String to convert.
   * @returns Log component the string represents.
   */
  libcomp::GenericLogComponent_t StringToLogComponent(
      const libcomp::String& comp) const override;

  /**
   * Convert a log component into a string.
   * @param comp Log component to convert.
   * @returns String representation of the log component.
   */
  libcomp::String LogComponentToString(
      libcomp::GenericLogComponent_t comp) const override;

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
};

}  // namespace libhack

/**
 * Macro to create a log function
 * @param name Name of the function
 * @param comp Component the functions logs
 * @param level Log level the function logs
 */
#define LOG_FUNCTION(name, comp, level)                                        \
  static inline void name(const std::function<libcomp::String(void)>& fun) {   \
    auto log = libhack::Log::GetSingletonPtr();                                \
                                                                               \
    if (log->ShouldLog(to_underlying(libhack::LogComponent_t::comp), level)) { \
      auto msg = new libcomp::LogMessageFixed(                                 \
          to_underlying(libhack::LogComponent_t::comp), level, fun());         \
      log->LogMessage(msg);                                                    \
    }                                                                          \
  }                                                                            \
                                                                               \
  template <typename Function, typename... Args>                               \
  static inline void name##Delayed(Function&& f, Args&&... args) {             \
    auto log = libhack::Log::GetSingletonPtr();                                \
                                                                               \
    if (log->ShouldLog(to_underlying(libhack::LogComponent_t::comp), level)) { \
      auto msg = new libcomp::LogMessageImpl<Args...>(                         \
          to_underlying(libhack::LogComponent_t::comp), level,                 \
          std::forward<Function>(f), std::forward<Args>(args)...);             \
      log->LogMessage(msg);                                                    \
    }                                                                          \
  }                                                                            \
                                                                               \
  static inline void name##Msg(const libcomp::String& _msg) {                  \
    auto log = libhack::Log::GetSingletonPtr();                                \
                                                                               \
    if (log->ShouldLog(to_underlying(libhack::LogComponent_t::comp), level)) { \
      auto msg = new libcomp::LogMessageFixed(                                 \
          to_underlying(libhack::LogComponent_t::comp), level, _msg);          \
      log->LogMessage(msg);                                                    \
    }                                                                          \
  }

/**
 * Macro to create a set of log functions for a component
 * @param comp Component to create the functions for
 */
#define LOG_FUNCTIONS(comp)                                                   \
  LOG_FUNCTION(Log##comp##Debug, comp, libcomp::BaseLog::LOG_LEVEL_DEBUG)     \
  LOG_FUNCTION(Log##comp##Info, comp, libcomp::BaseLog::LOG_LEVEL_INFO)       \
  LOG_FUNCTION(Log##comp##Warning, comp, libcomp::BaseLog::LOG_LEVEL_WARNING) \
  LOG_FUNCTION(Log##comp##Error, comp, libcomp::BaseLog::LOG_LEVEL_ERROR)     \
  LOG_FUNCTION(Log##comp##Critical, comp, libcomp::BaseLog::LOG_LEVEL_CRITICAL)

// Add a log function set for each component here!
LOG_FUNCTIONS(AccountManager)
LOG_FUNCTIONS(ActionManager)
LOG_FUNCTIONS(AIManager)
LOG_FUNCTIONS(Barter)
LOG_FUNCTIONS(Bazaar)
LOG_FUNCTIONS(CharacterManager)
LOG_FUNCTIONS(ChatManager)
LOG_FUNCTIONS(Clan)
LOG_FUNCTIONS(DefinitionManager)
LOG_FUNCTIONS(Demon)
LOG_FUNCTIONS(EventManager)
LOG_FUNCTIONS(Friend)
LOG_FUNCTIONS(FusionManager)
LOG_FUNCTIONS(Item)
LOG_FUNCTIONS(MatchManager)
LOG_FUNCTIONS(Party)
LOG_FUNCTIONS(ServerConstants)
LOG_FUNCTIONS(ServerDataManager)
LOG_FUNCTIONS(SkillManager)
LOG_FUNCTIONS(Team)
LOG_FUNCTIONS(TokuseiManager)
LOG_FUNCTIONS(Trade)
LOG_FUNCTIONS(WebAPI)
LOG_FUNCTIONS(ZoneManager)

#endif  // LIBHACK_SRC_LOG_H
