/**
 * @file libclient/src/MessageCharacterList.h
 * @ingroup libcomp
 *
 * @author Aaron M Brown
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

#ifndef LIBCLIENT_SRC_MESSAGECHARACTERLIST_H
#define LIBCLIENT_SRC_MESSAGECHARACTERLIST_H

// libcomp Includes
#include <CString.h>
#include <EnumUtils.h>
#include <ErrorCodes.h>
#include <MessageClient.h>

// packet Includes
#include <PacketLobbyCharacterList.h>

namespace logic {
/**
 * Message containing characterlist data
 */
class MessageCharacterList : public libcomp::Message::MessageClient {
 public:
  /**
   * Create the message.
   */
  MessageCharacterList(
      const std::shared_ptr<packets::PacketLobbyCharacterList>& mPacket)
      : libcomp::Message::MessageClient(), mPayload(mPacket) {}

  /**
   * Cleanup the message.
   */
  ~MessageCharacterList() override {}

  /**
   * Get the specific client message type.
   * @return The message's client message type
   */
  libcomp::Message::MessageClientType GetMessageClientType() const override {
    return libcomp::Message::MessageClientType::CHARACTER_LIST_UPDATE;
  }

  /**
   * Getter for packet object containing character list data.
   * @return Packet object containing character list data.
   */
  std::shared_ptr<packets::PacketLobbyCharacterList> GetPayload() const {
    return mPayload;
  }

  /**
   * Dump the message for logging.
   * @return String representation of the message.
   */
  libcomp::String Dump() const override {
    return "Message: Character List Update Request\n";
  }

 private:
  std::shared_ptr<packets::PacketLobbyCharacterList> mPayload;
};
}  // namespace logic

#endif  // LIBCLIENT_SRC_MESSAGECHARACTERLIST_H
