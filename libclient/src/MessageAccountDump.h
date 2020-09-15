/**
 * @file libclient/src/MessageAccountDump.h
 * @ingroup libclient
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Client message.
 *
 * This file is part of the COMP_hack Client Library (libclient).
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

#ifndef LIBCLIENT_SRC_MESSAGEACCOUNTDUMP_H
#define LIBCLIENT_SRC_MESSAGEACCOUNTDUMP_H

// libobjgen Includes
#include <UUID.h>

// libcomp Includes
#include <CString.h>
#include <MessageClient.h>

namespace logic {

/**
 * Message requesting to start an account dump.
 */
class MessageAccountDump : public libcomp::Message::MessageClient {
 public:
  /**
   * Create the message.
   * @param path Path to save the account to.
   */
  MessageAccountDump(const libcomp::String &path)
      : libcomp::Message::MessageClient(), mPath(path) {}

  /**
   * Cleanup the message.
   */
  ~MessageAccountDump() override {}

  /**
   * Get the path to save the account to.
   * @returns Path to save the account to.
   */
  libcomp::String GetPath() const { return mPath; }

  /**
   * Get the specific client message type.
   * @return The message's client message type
   */
  libcomp::Message::MessageClientType GetMessageClientType() const override {
    return libcomp::Message::MessageClientType::REQUEST_ACCOUNT_DUMP;
  }

  /**
   * Dump the message for logging.
   * @return String representation of the message.
   */
  libcomp::String Dump() const override {
    return libcomp::String("Message: Account dump\nPath: %1").Arg(mPath);
  }

 protected:
  /// Path to save the account to.
  libcomp::String mPath;
};

/**
 * Message indicating the status of the account dump.
 */
class MessageAccountDumpStatus : public libcomp::Message::MessageClient {
 public:
  /**
   * Create the message.
   * @param success If the account dump was a success.
   */
  MessageAccountDumpStatus(bool success)
      : libcomp::Message::MessageClient(), mSuccess(success) {}

  /**
   * Cleanup the message.
   */
  ~MessageAccountDumpStatus() override {}

  /**
   * Get if the account dump was a success.
   * @returns true if the account dump was a success; false otherwise.
   */
  bool IsSuccess() const { return mSuccess; }

  /**
   * Get the specific client message type.
   * @return The message's client message type
   */
  libcomp::Message::MessageClientType GetMessageClientType() const override {
    return libcomp::Message::MessageClientType::ACCOUNT_DUMP_STATUS;
  }

  /**
   * Dump the message for logging.
   * @return String representation of the message.
   */
  libcomp::String Dump() const override {
    return libcomp::String("Message: Account dump status\nResult: %1")
        .Arg(mSuccess ? "OK" : "FAILED");
  }

 protected:
  /// If the account dump was a success.
  bool mSuccess;
};

}  // namespace logic

#endif  // LIBCLIENT_SRC_MESSAGEACCOUNTDUMP_H
