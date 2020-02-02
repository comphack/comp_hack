/**
 * @file server/channel/src/packets/internal/FriendsUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all friend list focused actions between the
 *  world and the channel.
 *
 * This file is part of the Channel Server (channel).
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

#include "Packets.h"

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <FriendSettings.h>

// channel Includes
#include "ChannelServer.h"
#include "ManagerConnection.h"

using namespace channel;

void SendFriendInfo(std::shared_ptr<ChannelServer> server,
    std::shared_ptr<ChannelClientConnection> client,
    std::vector<std::shared_ptr<objects::CharacterLogin>> friendLogins,
    ChannelToClientPacketCode_t packetCode)
{
    auto worldDB = server->GetWorldDatabase();

    int8_t friendCount = (int8_t)friendLogins.size();
    for(int8_t i = 0; i < friendCount; i++)
    {
        auto login = friendLogins[(size_t)i];
        auto character = login->GetCharacter();
        if(!character.Get(worldDB))
        {
            LogFriendError([&]()
            {
                return libcomp::String("Character failed to load: %1\n")
                    .Arg(character.GetUUID().ToString());
            });

            return;
        }

        auto fSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, character->GetUUID());
        if(!fSettings)
        {
            LogFriendError([&]()
            {
                return libcomp::String("Character friend settings failed to "
                    "load: %1\n").Arg(character.GetUUID().ToString());
            });

            return;
        }

        libcomp::Packet reply;
        reply.WritePacketCode(packetCode);
        if(packetCode == ChannelToClientPacketCode_t::PACKET_FRIEND_INFO)
        {
            reply.WriteS8(friendCount);
            reply.WriteS8(i);
        }
        else
        {
            reply.WriteS8(1);
        }

        reply.WriteS32Little(login->GetWorldCID());
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            character->GetName(), true);
        reply.WriteU32Little((uint32_t)login->GetWorldCID());
        reply.WriteS8(0);   // Unknown
        reply.WriteS8((int8_t)login->GetStatus());
        reply.WriteS32Little((int32_t)login->GetZoneID());
        reply.WriteS8(login->GetChannelID());
        reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
            fSettings->GetFriendMessage(), true);

        client->QueuePacket(reply);
    }

    client->FlushOutgoing();
}

bool Parsers::FriendsUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)connection;

    if(p.Size() < 6)
    {
        LogFriendErrorMsg("Invalid response received for CharacterLogin.\n");

        return false;
    }

    uint8_t mode = p.ReadU8();
    int32_t cid = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = server->GetManagerConnection()->GetEntityClient(cid, true);
    if(!client)
    {
        // Character is not here anymore, exit now
        return true;
    }

    auto cState = client->GetClientState()->GetCharacterState();
    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LIST:
        {
            int8_t loginCount = p.ReadS8();

            // Pull all the logins
            std::vector<std::shared_ptr<objects::CharacterLogin>> logins;
            for(int8_t i = 0; i < loginCount; i++)
            {
                auto login = std::make_shared<objects::CharacterLogin>();
                if(!login->LoadPacket(p, false))
                {
                    LogFriendErrorMsg("Invalid character info received for "
                        "CharacterLogin.\n");

                    return false;
                }

                logins.push_back(login);
            }

            if(logins.size() == 0)
            {
                // Nothing to send
                return true;
            }

            server->QueueWork(SendFriendInfo, server, client, logins,
                ChannelToClientPacketCode_t::PACKET_FRIEND_INFO);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_YN_REQUEST:
        {
            libcomp::Packet request;
            request.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FRIEND_REQUESTED);

            // Send the requesting character name
            request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true),
                true);

            client->SendPacket(request);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES:
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO:
        {
            auto charName = p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

            bool success = (InternalPacketAction_t)mode
                == InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES;
            if(success)
            {
                // Reload the updated friends info
                objects::FriendSettings::LoadFriendSettingsByCharacter(
                    server->GetWorldDatabase(), cState->GetEntity()->GetUUID());

                if(charName == cState->GetEntity()->GetName())
                {
                    // No need to send the success to the player who accepted
                    return true;
                }
            }

            libcomp::Packet reply;
            reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FRIEND_REQUEST);

            // Send the reqeusted character name back
            reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                charName, true);
            reply.WriteS32Little(success ? 0 : -1);
            client->SendPacket(reply);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_ADD:
    case InternalPacketAction_t::PACKET_ACTION_REMOVE:
        {
            bool added = (InternalPacketAction_t)mode
                == InternalPacketAction_t::PACKET_ACTION_ADD;

            // Reload the updated friends info
            objects::FriendSettings::LoadFriendSettingsByCharacter(
                server->GetWorldDatabase(), cState->GetEntity()->GetUUID());

            if(added)
            {
                auto login = std::make_shared<objects::CharacterLogin>();
                if(!login->LoadPacket(p, false))
                {
                    LogFriendErrorMsg("Invalid character info received for "
                        "CharacterLogin.\n");

                    return false;
                }

                std::vector<std::shared_ptr<objects::CharacterLogin>> logins;
                logins.push_back(login);
                server->QueueWork(SendFriendInfo, server, client, logins,
                    ChannelToClientPacketCode_t::PACKET_FRIEND_ADD_REMOVE);
            }
            else
            {
                int32_t removedCID = p.ReadS32Little();

                libcomp::Packet reply;
                reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FRIEND_ADD_REMOVE);
                reply.WriteS8(0);
                reply.WriteS32Little(removedCID);

                client->SendPacket(reply);
            }
        }
        break;
    default:
        break;
    }

    return true;
}
