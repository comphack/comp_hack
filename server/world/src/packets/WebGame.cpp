/**
 * @file server/world/src/packets/WebGame.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle web-game setup and relay between the channel
 *  and lobby servers.
 *
 * This file is part of the World Server (world).
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <Character.h>
#include <CharacterProgress.h>
#include <WebGameSession.h>

// world Includes
#include "AccountManager.h"
#include "CharacterManager.h"
#include "ManagerConnection.h"
#include "WorldServer.h"

using namespace world;

bool Parsers::WebGame::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 1)
    {
        return false;
    }

    uint8_t mode = p.ReadU8();

    auto server = std::dynamic_pointer_cast<WorldServer>(
        pPacketManager->GetServer());
    auto lobbyConnection = server->GetLobbyConnection();
    bool fromLobby = connection == lobbyConnection;

    // Handle game end requests
    if((InternalPacketAction_t)mode ==
        InternalPacketAction_t::PACKET_ACTION_REMOVE)
    {
        libcomp::String username;
        if(fromLobby)
        {
            // Username sent from lobby
            if(p.Left() < 2 ||
                (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
            {
                LogGeneralErrorMsg("WebGame request from lobby did not supply"
                    " a source username\n");
                return false;
            }

            username = p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
        }
        else
        {
            // Convert world CID to account username
            if(p.Left() < 4)
            {
                LogGeneralErrorMsg("WebGame request from channel did not"
                    " supply a source world CID\n");
                return false;
            }

            int32_t worldCID = p.ReadS32Little();
            auto cLogin = server->GetCharacterManager()->GetCharacterLogin(
                worldCID);
            if(cLogin)
            {
                auto character = cLogin->GetCharacter().Get(
                    server->GetWorldDatabase());
                auto account = character ? std::dynamic_pointer_cast<objects::Account>(
                    libcomp::PersistentObject::GetObjectByUUID(character->GetAccount()))
                    : nullptr;
                if(account)
                {
                    username = account->GetUsername();
                }
            }
        }

        if(!username.IsEmpty())
        {
            server->GetAccountManager()->EndWebGameSession(username,
                !fromLobby, fromLobby);
        }

        return true;
    }

    if(fromLobby)
    {
        // Lobby requests always contain username and session ID
        if(p.Left() < 2 ||
            (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
        {
            LogGeneralErrorMsg("WebGame request from lobby did not supply"
                " a source username\n");
            return false;
        }

        auto username = p.ReadString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
        
        if(p.Left() < 2 ||
            (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
        {
            LogGeneralError([username]()
            {
                return libcomp::String("WebGame request from lobby did not"
                    " supply a source session ID from account: %1.\n")
                    .Arg(username);
            });
            return false;
        }

        auto sessionID = p.ReadString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

        auto gameSession = server->GetAccountManager()->GetGameSession(
            username);
        if(!gameSession || gameSession->GetSessionID() != sessionID)
        {
            // End the game session (if one exists)
            server->GetAccountManager()->EndWebGameSession(username,
                true, true);
        }

        switch((InternalPacketAction_t)mode)
        {
        case InternalPacketAction_t::PACKET_ACTION_ADD:
            {
                // Lobby has accepted game session, notify the channel
                auto cLogin = server->GetCharacterManager()->GetCharacterLogin(
                    gameSession->GetWorldCID());
                auto channel = cLogin ? server->GetChannelConnectionByID(
                    cLogin->GetChannelID()) : nullptr;
                if(channel)
                {
                    libcomp::Packet notify;
                    notify.WritePacketCode(InternalPacketCode_t::PACKET_WEB_GAME);
                    notify.WriteU8((uint8_t)
                        InternalPacketAction_t::PACKET_ACTION_ADD);
                    notify.WriteS32Little(cLogin->GetWorldCID());
                    notify.WriteString16Little(
                        libcomp::Convert::Encoding_t::ENCODING_UTF8, sessionID,
                        true);
                    gameSession->SavePacket(notify);

                    channel->SendPacket(notify);
                }
                else
                {
                    // End the game session for the lobby
                    server->GetAccountManager()->EndWebGameSession(username,
                        true, false);
                }
            }
            break;
        default:
            break;
        }
    }
    else
    {
        switch((InternalPacketAction_t)mode)
        {
        case InternalPacketAction_t::PACKET_ACTION_ADD:
            {
                // Channel has requested game session
                auto gameSession = std::make_shared<objects::WebGameSession>();
                if(!gameSession->LoadPacket(p, false))
                {
                    LogGeneralErrorMsg("Channel requested WebGame session"
                        " supplied invalid game session data\n");
                    return false;
                }

                server->GetAccountManager()->StartWebGameSession(gameSession);
            }
            break;
        default:
            break;
        }
    }

    return true;
}
