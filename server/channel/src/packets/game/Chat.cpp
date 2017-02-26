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

static std::unordered_map<libcomp::String, GMCommand_t> gmands;

static void SetupGMCommands()
{
    if(!gmands.empty())
    {
        return;
    }

    gmands["contract"] = GMCommand_t::GM_COMMAND_CONTRACT;
    gmands["levelup"] = GMCommand_t::GM_COMMAND_LEVEL_UP;
    gmands["lnc"] = GMCommand_t::GM_COMMAND_LNC;
    gmands["xp"] = GMCommand_t::GM_COMMAND_XP;
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
        auto account = state->GetAccountLogin()->GetAccount();
        if(!account->GetIsGM())
        {
            // Don't process the message but don't fail
            LOG_DEBUG(libcomp::String("Non-GM account attempted to execute a GM"
                " command: %1\n").Arg(account->GetUUID().ToString()));
            return true;
        }

        libcomp::String sentFrom = state->GetCharacterState()->GetCharacter()
            ->GetName();
        LOG_INFO(libcomp::String("[GM] %1: %2\n").Arg(sentFrom).Arg(line));

        // Make sure the gmands map is setup 
        SetupGMCommands();

        libcomp::String command(match[1]);
        libcomp::String args(match.max_size() > 2 ? match[2].str() : "");

        command = command.ToLower();

        std::list<libcomp::String> argsList;
        if(!args.IsEmpty())
        {
            argsList = args.Split(" ");
        }
        argsList.remove_if([](const libcomp::String& value) { return value.IsEmpty(); });

        auto iter = gmands.find(command);
        if(iter != gmands.end())
        {
            server->QueueWork([](ChatManager* pChatManager,
                const std::shared_ptr<ChannelClientConnection> pClient,
                GMCommand_t pCmd, libcomp::String pCommandString,
                const std::list<libcomp::String> pArgsList)
            {
                if(!pChatManager->ExecuteGMCommand(pClient, pCmd, pArgsList))
                {
                    LOG_WARNING(libcomp::String("GM command could not be"
                        " processed: %1\n").Arg(pCommandString));
                }
            }, server->GetChatManager(), client, iter->second, command, argsList);
        }
        else
        {
            LOG_WARNING(libcomp::String("Unknown GM command encountered: %1\n")
                .Arg(command));
        }
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
