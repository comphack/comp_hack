/**
* @file server/channel/src/packets/game/Chat.cpp
* @ingroup channel
*
* @author HikaruM
*
* @brief Handles GM Commands and Messages
*
* This file is part of the Channel Server (channel).
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

#include "Packets.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <Packet.h>
#include <ManagerPacket.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>
#include <Convert.h>
#include <ClientState.h>
#include <Character.h>
#include <AccountLogin.h>
#include <Account.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ChannelServer.h"
#include "ChatManager.h"

using namespace channel;

bool Parsers::Chat::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 5)
    {
        return false;
    }

    uint16_t chatchannel = p.ReadU16Little();

    if(p.Left() != (uint16_t)(2 + p.PeekU16Little()))
    {
        return false;
    }

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto state = client->GetClientState();

    libcomp::String line = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    std::smatch match;
    std::string input = line.C();
    std::regex toFind("@([^\\s]+)(.*)");
    if(std::regex_match(input, match, toFind))
    {
        if(state->GetUserLevel() == 0)
        {
            // Don't process the message but don't fail
            LOG_DEBUG(libcomp::String("Non-GM account attempted to execute a GM"
                " command: %1\n").Arg(state->GetAccountUID().ToString()));
            return true;
        }

        libcomp::String sentFrom = state->GetCharacterState()->GetEntity()
            ->GetName();
        LOG_INFO(libcomp::String("[GM] %1: %2\n").Arg(sentFrom).Arg(line));

        libcomp::String command(match[1]);
        libcomp::String args(match.max_size() > 2 ? match[2].str() : "");

        command = command.ToLower();

        std::list<libcomp::String> argsList;
        if(!args.IsEmpty())
        {
            argsList = args.Split(" ");
        }
        argsList.remove_if([](const libcomp::String& value) { return value.IsEmpty(); });

        server->QueueWork([](ChatManager* pChatManager,
            const std::shared_ptr<ChannelClientConnection>& cmdClient,
            const libcomp::String& cmd,
            const std::list<libcomp::String>& cmdArgs)
        {
            if(!pChatManager->ExecuteGMCommand(cmdClient, cmd, cmdArgs))
            {
                LOG_WARNING(libcomp::String("GM command could not be"
                    " processed: %1\n").Arg(cmd));
            }
        }, server->GetChatManager(), client, command, argsList);
    }
    else
    {
        auto chatManager = server->GetChatManager();
        if(!chatManager->SendChatMessage(client, (ChatType_t)chatchannel, line))
        {
            LOG_ERROR("Chat message could not be sent.\n");
        }
    }

    return true;
}
