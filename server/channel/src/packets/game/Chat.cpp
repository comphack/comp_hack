/**
* @file server/channel/src/packets/game/Message.cpp
* @ingroup channel
*
* @author HikaruM
*
* @brief Handles GM Commands and Messages
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

#include "Packets.h"
#include "ChannelClientConnection.h"
#include "ChatManager.h"
#include "ChannelServer.h"

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


using namespace channel;

static std::unordered_map<libcomp::String,
    void(*)(ClientState*, std::list<libcomp::String>)> gmands;

//This function sets up the GM commands, including teleport, place npc,
//level up, etc.
static void SetupGMCommands()
{
    if (!gmands.empty())
    {
        return;
    }
    /// @todo: write GM commands, once stuff is functional
    else return;
}

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
    auto state = client->GetClientState();

    libcomp::String line = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    std::smatch match;
    std::string input = line.C();
    std::regex toFind("@([^\\s]+)(.*)");
    if((std::regex_match(input, match, toFind)) &&
        state->GetAccountLogin()->GetAccount()->GetIsGM())
    {
        libcomp::String sentFrom = state->GetCharacterState()->GetCharacter()
            ->GetName();
        LOG_INFO(libcomp::String("[GM] %1: %2\n").Arg(sentFrom).Arg(line));

        // Make sure the gmands map is setup 
        SetupGMCommands();

        libcomp::String command(match[0]);
        libcomp::String args(match[1]);

        command = command.ToLower();
        args = args.Trimmed();

        std::list<libcomp::String> argsList;
        if(!args.IsEmpty())
        {
            argsList = line.Split(" ");
        }

        auto iter = gmands.find(command);
        if(iter != gmands.end())
        {
            iter->second(state, argsList);
        }
        else
        {
            LOG_WARNING(libcomp::String("Unknown GM command encountered: %1\n")
                .Arg(command));
        }
    }
    else
    {
        auto server = std::dynamic_pointer_cast<ChannelServer>(
            pPacketManager->GetServer());
        auto ChatManager = server->GetChatManager();
        if(!ChatManager->SendChatMessage(client, (ChatType_t)chatchannel, line))
        {
            LOG_ERROR("Chat message could not be sent.\n");
        }
    }

    return true;
}
