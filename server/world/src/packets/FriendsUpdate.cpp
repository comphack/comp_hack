/**
 * @file server/world/src/packets/FriendsUpdate.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all friend list focused actions between the
 *  world and the channels.
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
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>
#include <FriendSettings.h>

// world Includes
#include "CharacterManager.h"
#include "WorldServer.h"

using namespace world;

void FriendRequestCancel(const std::shared_ptr<libcomp::TcpConnection>& connection,
    const libcomp::String otherName,
    std::shared_ptr<objects::CharacterLogin> cLogin)
{
    libcomp::Packet reply;
    reply.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
    reply.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO);
    reply.WriteS32Little(cLogin->GetWorldCID());
    reply.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
        otherName, true);

    connection->SendPacket(reply);
}

void FriendList(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> connection,
    std::shared_ptr<objects::CharacterLogin> cLogin)
{
    auto characterManager = server->GetCharacterManager();

    auto fLogins = characterManager->GetRelatedCharacterLogins(
        cLogin, RELATED_FRIENDS);
    if(fLogins.size() == 0)
    {
        // No friend info to send
        return;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
    reply.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LIST);
    reply.WriteS32Little(cLogin->GetWorldCID());
    reply.WriteS8((int8_t)fLogins.size());
    for(auto fLogin : fLogins)
    {
        fLogin->SavePacket(reply);
    }

    connection->SendPacket(reply);
}

void FriendRequest(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection,
    std::shared_ptr<objects::CharacterLogin> cLogin,
    const libcomp::String sourceName, const libcomp::String targetName)
{
    auto characterManager = server->GetCharacterManager();
    auto targetLogin = characterManager->GetCharacterLogin(targetName);

    // Check that the target character exists and is online
    bool failed = !targetLogin || targetLogin->GetChannelID() < 0;
    if(!failed)
    {
        auto worldDB = server->GetWorldDatabase();
        auto sourceFSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, cLogin->GetCharacter().GetUUID());
        for(auto f : sourceFSettings->GetFriends())
        {
            if(f == targetLogin->GetCharacter().GetUUID())
            {
                // Already in the friends list
                failed = true;
            }
        }

        auto channel = server->GetChannelConnectionByID(targetLogin->GetChannelID());
        if(!failed && channel)
        {
            libcomp::Packet request;
            request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
            request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_YN_REQUEST);
            request.WriteS32Little(targetLogin->GetWorldCID());
            request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                sourceName, true);

            channel->SendPacket(request);
        }
        else
        {
            failed = true;
        }
    }

    if(failed)
    {
        FriendRequestCancel(sourceConnection, targetName, cLogin);
    }
}

void FriendRequestAccepted(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection,
    std::shared_ptr<objects::CharacterLogin> cLogin,
    const libcomp::String sourceName, const libcomp::String targetName)
{
    auto characterManager = server->GetCharacterManager();
    auto targetLogin = characterManager->GetCharacterLogin(targetName);

    // Check that the target character exists and is online
    bool failed = !targetLogin || targetLogin->GetChannelID() < 0;
    if(!failed)
    {
        auto worldDB = server->GetWorldDatabase();
        auto sourceFSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, cLogin->GetCharacter().GetUUID());
        auto targetFSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, targetLogin->GetCharacter().GetUUID());

        if(sourceFSettings && targetFSettings)
        {
            if(sourceFSettings->FriendsCount() >= MAX_FRIEND_COUNT ||
                targetFSettings->FriendsCount() >= MAX_FRIEND_COUNT)
            {
                failed = true;
            }
            else
            {
                sourceFSettings->AppendFriends(targetLogin->GetCharacter().GetUUID());
                targetFSettings->AppendFriends(cLogin->GetCharacter().GetUUID());
                failed = !sourceFSettings->Update(worldDB) ||
                    !targetFSettings->Update(worldDB);
            }
        }
        else
        {
            failed = true;
        }

        auto channel = server->GetChannelConnectionByID(targetLogin->GetChannelID());
        if(!failed)
        {
            libcomp::Packet request;
            request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
            request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES);
            request.WriteS32Little(targetLogin->GetWorldCID());
            request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                sourceName, true);

            sourceConnection->QueuePacket(request);

            request.Clear();
            request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
            request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
            request.WriteS32Little(cLogin->GetWorldCID());
            targetLogin->SavePacket(request);

            sourceConnection->SendPacket(request);

            if(channel)
            {
                request.Clear();
                request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
                request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
                request.WriteS32Little(targetLogin->GetWorldCID());
                cLogin->SavePacket(request);

                channel->SendPacket(request);
            }
            else
            {
                failed = true;
            }
        }
        else
        {
            failed = true;
            if(channel)
            {
                // Inform the other player too
                FriendRequestCancel(channel, sourceName, targetLogin);
            }
        }
    }

    if(failed)
    {
        FriendRequestCancel(sourceConnection, targetName, cLogin);
    }
}

void FriendRequestCancelled(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection,
    std::shared_ptr<objects::CharacterLogin> cLogin,
    const libcomp::String sourceName, const libcomp::String targetName)
{
    auto characterManager = server->GetCharacterManager();
    auto targetLogin = characterManager->GetCharacterLogin(targetName);

    // Check that the target character exists and is online
    bool failed = !targetLogin || targetLogin->GetChannelID() < 0;
    if(!failed)
    {
        auto channel = server->GetChannelConnectionByID(targetLogin->GetChannelID());
        if(channel)
        {
            FriendRequestCancel(channel, sourceName, targetLogin);
        }
        else
        {
            failed = true;
        }
    }

    if(failed)
    {
        FriendRequestCancel(sourceConnection, targetName, cLogin);
    }
}

void FriendRemoved(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection,
    std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t targetCID)
{
    auto targetLogin = server->GetCharacterManager()->GetCharacterLogin(targetCID);

    // Check that the target character exists
    bool failed = !targetLogin;
    if(!failed)
    {
        auto sourceUUID = cLogin->GetCharacter().GetUUID();
        auto targetUUID = targetLogin->GetCharacter().GetUUID();

        auto worldDB = server->GetWorldDatabase();
        auto sourceFSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, sourceUUID);
        auto targetFSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, targetUUID);

        if(sourceFSettings && targetFSettings)
        {
            for(size_t i = 0; i < sourceFSettings->FriendsCount(); i++)
            {
                auto f = sourceFSettings->GetFriends(i);
                if(f == targetUUID)
                {
                    sourceFSettings->RemoveFriends(i);
                    break;
                }
            }

            for(size_t i = 0; i < targetFSettings->FriendsCount(); i++)
            {
                auto f = targetFSettings->GetFriends(i);
                if(f == sourceUUID)
                {
                    targetFSettings->RemoveFriends(i);
                    break;
                }
            }

            failed = !sourceFSettings->Update(worldDB) ||
                !targetFSettings->Update(worldDB);
        }
        else
        {
            failed = true;
        }

        if(!failed)
        {
            libcomp::Packet request;
            request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
            request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_REMOVE);
            request.WriteS32Little(cLogin->GetWorldCID());
            request.WriteS32Little(targetLogin->GetWorldCID());

            sourceConnection->SendPacket(request);

            auto channel = server->GetChannelConnectionByID(targetLogin->GetChannelID());
            if(channel)
            {
                request.Clear();
                request.WritePacketCode(InternalPacketCode_t::PACKET_FRIENDS_UPDATE);
                request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_REMOVE);
                request.WriteS32Little(targetLogin->GetWorldCID());
                request.WriteS32Little(cLogin->GetWorldCID());

                channel->SendPacket(request);
            }
        }
    }

    // Nothing to send to the client on failure
}

bool Parsers::FriendsUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 5)
    {
        LogFriendErrorMsg("Invalid packet data sent to FriendsUpdate\n");

        return false;
    }

    uint8_t mode = p.ReadU8();
    int32_t cid = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
    auto cLogin = server->GetCharacterManager()->GetCharacterLogin(cid);
    if(!cLogin)
    {
        LogFriendError([&]()
        {
            return libcomp::String("Invalid world CID sent to "
                "FriendsUpdate: %1\n").Arg(cid);
        });

        return false;
    }

    if((InternalPacketAction_t)mode == InternalPacketAction_t::PACKET_ACTION_GROUP_LIST)
    {
        server->QueueWork(FriendList, server, connection, cLogin);
    }
    else if((InternalPacketAction_t)mode == InternalPacketAction_t::PACKET_ACTION_REMOVE)
    {
        if(p.Left() < 4)
        {
            LogFriendError([&]()
            {
                return libcomp::String("Missing target CID parameter"
                    " for command %1\n").Arg(mode);
            });

            return false;
        }

        int32_t targetCID = p.ReadS32Little();
        server->QueueWork(FriendRemoved, server, connection, cLogin, targetCID);
    }
    else
    {
        if(p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
        {
            LogFriendError([&]()
            {
                return libcomp::String("Missing source name parameter"
                    " for command %1\n").Arg(mode);
            });

            return false;
        }

        libcomp::String sourceName = p.ReadString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

        if(p.Left() < 2 || (p.Left() != (uint32_t)(2 + p.PeekU16Little())))
        {
            LogGeneralError([&]()
            {
                return libcomp::String("Missing target name parameter"
                    " for command %1\n").Arg(mode);
            });

            return false;
        }

        libcomp::String targetName = p.ReadString16Little(
            libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

        switch((InternalPacketAction_t)mode)
        {
        case InternalPacketAction_t::PACKET_ACTION_YN_REQUEST:
            server->QueueWork(FriendRequest, server, connection, cLogin,
                sourceName, targetName);
            break;
        case InternalPacketAction_t::PACKET_ACTION_ADD:
        case InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES:
            server->QueueWork(FriendRequestAccepted, server, connection, cLogin,
                sourceName, targetName);
            break;
        case InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO:
            server->QueueWork(FriendRequestCancelled, server, connection, cLogin,
                sourceName, targetName);
            break;
        default:
            LogGeneralError([&]()
            {
                return libcomp::String("Unknown mode sent to "
                    "FriendsUpdate: %1\n").Arg(mode);
            });

            return false;
            break;
        }
    }

    return true;
}
