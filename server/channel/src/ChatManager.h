/**
 * @file server/channel/src/ChatManager.h
 * @ingroup channel
 *
 * @author HikaruM
 *
 * @brief Manages Chat Messages and GM Commands
 *
 * This file is part of the Channel Server (channel).
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

#ifndef SERVER_CHANNEL_SRC_CHATMANAGER_H
#define SERVER_CHANNEL_SRC_CHATMANAGER_H

#include "ChannelClientConnection.h"

namespace channel
{

class ChannelServer;

enum ChatType_t : uint16_t
{
    CHAT_PARTY = 41,
	CHAT_SHOUT = 44,
	CHAT_SAY = 45,
//    CHAT_KLAN = ???,
//    CHAT_TEAM = ???,
//    CHAT_TELL = ???,
};

enum ChatVis_t : uint16_t
{
    CHAT_VIS_SELF,
    CHAT_VIS_PARTY,
    CHAT_VIS_ZONE,
    CHAT_VIS_RANGE,
    CHAT_VIS_KLAN,
    CHAT_VIS_TEAM,
    CHAT_VIS_GLOBAL,
    CHAT_VIS_GMS,
};

class ChatManager
{
public:
    ChatManager();
    ~ChatManager();

   bool SendChatMessage(const std::shared_ptr<channel::ChannelClientConnection>& client, ChatType_t chatChannel, libcomp::String message);
	
private:

};

}
#endif