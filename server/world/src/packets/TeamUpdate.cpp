/**
 * @file server/world/src/packets/TeamUpdate.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Parser to handle all team focused actions between the world
 *  and the channels.
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
#include <ErrorCodes.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <CharacterLogin.h>

// world Includes
#include "CharacterManager.h"
#include "WorldServer.h"

using namespace world;

void TeamForm(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> requestConnection,
    std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t teamID, int8_t type)
{
    auto characterManager = server->GetCharacterManager();

    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
    if(cLogin->GetPartyID())
    {
        errorCode = (int8_t)TeamErrorCodes_t::IN_PARTY;
    }
    else if(!cLogin->GetTeamID())
    {
        teamID = characterManager->AddToTeam(cLogin->GetWorldCID(), 0);
        if(teamID)
        {
            errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
        }
    }

    auto team = characterManager->GetTeam(teamID);
    if(team)
    {
        // Define new team and send update
        if(type >= (int8_t)objects::Team::Category_t::CATHEDRAL)
        {
            team->SetCategory(objects::Team::Category_t::CATHEDRAL);
        }
        else if(type >= (int8_t)objects::Team::Category_t::DIASPORA)
        {
            team->SetCategory(objects::Team::Category_t::DIASPORA);
        }
        else
        {
            team->SetCategory(objects::Team::Category_t::PVP);
        }

        team->SetType(type);

        characterManager->SendTeamInfo(teamID);
    }

    libcomp::Packet reply;
    WorldServer::GetRelayPacket(reply, cLogin->GetWorldCID());
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_TEAM_FORM);
    reply.WriteS32Little(teamID ? teamID : -1);
    reply.WriteS8(errorCode);
    reply.WriteS8(type);

    requestConnection->QueuePacket(reply);

    if(team)
    {
        // Send initial character add notification
        libcomp::Packet relay;
        auto cidOffset = WorldServer::GetRelayPacket(relay);
        relay.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TEAM_MEMBER_ADD);
        relay.WriteS32Little(teamID);
        relay.WriteS32Little(cLogin->GetWorldCID());
        relay.WriteString16Little(
            libcomp::Convert::Encoding_t::ENCODING_CP932,
            cLogin->GetCharacter()->GetName(), true);

        characterManager->SendToRelatedCharacters(relay, cLogin->GetWorldCID(),
            cidOffset, RELATED_TEAM, true);
    }

    requestConnection->FlushOutgoing();
}

void TeamInvite(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> requestConnection,
    std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t teamID, const libcomp::String targetName)
{
    auto characterManager = server->GetCharacterManager();
    auto targetLogin = characterManager->GetCharacterLogin(targetName);
    auto targetChar = targetLogin->GetCharacter().Get();

    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
    if(cLogin && targetLogin && targetChar && targetLogin->GetChannelID() >= 0)
    {
        auto team = cLogin->GetTeamID()
            ? characterManager->GetTeam(cLogin->GetTeamID()) : nullptr;
        if(!team)
        {
            errorCode = (int8_t)TeamErrorCodes_t::NO_TEAM;
        }
        else if(team->GetID() != teamID)
        {
            errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
        }
        else if(team->GetLeaderCID() != cLogin->GetWorldCID())
        {
            errorCode = (int8_t)TeamErrorCodes_t::LEADER_REQUIRED;
        }
        else if(targetLogin->GetTeamID())
        {
            errorCode = (int8_t)TeamErrorCodes_t::OTHER_TEAM;
        }
        else if(targetLogin->GetPartyID())
        {
            errorCode = (int8_t)TeamErrorCodes_t::TARGET_IN_PARTY;
        }
        else if(team->MemberIDsCount() >=
            characterManager->GetTeamMaxSize((int8_t)team->GetCategory()))
        {
            errorCode = (int8_t)TeamErrorCodes_t::TEAM_FULL;
        }
        else
        {
            auto channel = server->GetChannelConnectionByID(targetLogin
                ->GetChannelID());
            if(channel)
            {
                auto inviterChar = cLogin->GetCharacter().Get();

                libcomp::Packet relay;
                WorldServer::GetRelayPacket(relay, targetLogin->GetWorldCID());
                relay.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_TEAM_INVITED);
                relay.WriteS32Little(teamID);
                relay.WriteString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932,
                    inviterChar ? inviterChar->GetName() : "", true);
                relay.WriteS8(0);
                relay.WriteS8(team->GetType());

                channel->SendPacket(relay);

                errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
            }
        }
    }

    libcomp::Packet relay;
    WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_INVITE);
    relay.WriteS32Little(teamID);
    relay.WriteS8(errorCode);

    requestConnection->SendPacket(relay);
}

void TeamCancel(std::shared_ptr<WorldServer> server,
    std::shared_ptr<libcomp::TcpConnection> requestConnection,
    std::shared_ptr<objects::CharacterLogin> cLogin, int32_t teamID)
{
    auto characterManager = server->GetCharacterManager();
    auto team = characterManager->GetTeam(teamID);
    auto leaderLogin = team
        ? characterManager->GetCharacterLogin(team->GetLeaderCID()) : nullptr;

    if(leaderLogin && leaderLogin->GetChannelID() >= 0 &&
        teamID == leaderLogin->GetTeamID())
    {
        auto channel = server->GetChannelConnectionByID(leaderLogin
            ->GetChannelID());
        if(channel)
        {
            auto targetChar = cLogin->GetCharacter().Get();

            libcomp::Packet relay;
            WorldServer::GetRelayPacket(relay, leaderLogin->GetWorldCID());
            relay.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TEAM_ANSWERED);
            relay.WriteS32Little(teamID);
            relay.WriteS8(0);   // No error
            relay.WriteString16Little(
                libcomp::Convert::Encoding_t::ENCODING_CP932,
                targetChar ? targetChar->GetName() : "", true);
            relay.WriteS8(team->GetType());

            channel->SendPacket(relay);
        }
    }

    libcomp::Packet relay;
    WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_ANSWER);
    relay.WriteS32Little(teamID);
    relay.WriteS8(0);   // Rejected
    relay.WriteS8(0);   // No error

    requestConnection->QueuePacket(relay);
}

bool Parsers::TeamUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 9)
    {
        LogTeamErrorMsg("Invalid packet data sent to TeamUpdate\n");

        return false;
    }

    int8_t mode = p.ReadS8();
    int32_t teamID = p.ReadS32Little();
    int32_t cid = p.ReadS32Little();

    auto server = std::dynamic_pointer_cast<WorldServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    auto cLogin = characterManager->GetCharacterLogin(cid);
    if(!cLogin)
    {
        LogTeamError([&]()
        {
            return libcomp::String("Invalid world CID sent to TeamUpdate: %1\n")
                .Arg(cid);
        });

        return false;
    }

    switch((InternalPacketAction_t)mode)
    {
    case InternalPacketAction_t::PACKET_ACTION_ADD:
        {
            // Request to form a team
            if(p.Left() < 1)
            {
                LogTeamErrorMsg("Team form request encountered without type"
                    " specified\n");

                return false;
            }

            int8_t type = p.ReadS8();
            TeamForm(server, connection, cLogin, teamID, type);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_YN_REQUEST:
        {
            // Request to invite a new team member
            if(p.Left() < 2 || (p.Left() < (uint32_t)(2 + p.PeekU16Little())))
            {
                LogTeamError([&]()
                {
                    return libcomp::String("Missing target name parameter"
                        " for command %1\n").Arg(mode);
                });

                return false;
            }

            libcomp::String targetName = p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

            TeamInvite(server, connection, cLogin, teamID, targetName);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES:
        {
            // Accept team invite
            characterManager->TeamJoin(cid, teamID, connection);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_RESPONSE_NO:
        {
            // Reject team invite
            TeamCancel(server, connection, cLogin, teamID);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LIST:
        {
            // Send team members
            auto team = characterManager->GetTeam(cLogin->GetTeamID());

            libcomp::Packet reply;
            WorldServer::GetRelayPacket(reply, cLogin->GetWorldCID());
            reply.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_TEAM_MEMBER_LIST);

            if(team && team->GetID() == teamID)
            {
                reply.WriteS32Little(teamID);
                reply.WriteS8((int8_t)TeamErrorCodes_t::SUCCESS);

                auto memberIDs = team->GetMemberIDs();
                reply.WriteS8((int8_t)memberIDs.size());
                for(int32_t worldCID : memberIDs)
                {
                    auto memberLogin = characterManager->GetCharacterLogin(
                        worldCID);
                    auto memberChar = memberLogin
                        ? memberLogin->GetCharacter().Get() : nullptr;
                    reply.WriteS32Little(worldCID);
                    reply.WriteString16Little(
                        libcomp::Convert::Encoding_t::ENCODING_CP932,
                        memberChar ? memberChar->GetName() : "", true);
                }
            }
            else
            {
                reply.WriteS32Little(teamID);
                reply.WriteS8((int8_t)TeamErrorCodes_t::INVALID_TEAM);
            }

            connection->SendPacket(reply);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE:
        {
            // Leave current team
            characterManager->TeamLeave(cLogin);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE:
        {
            // Update team leader
            if(p.Left() < 4)
            {
                LogTeamError([&]()
                {
                    return libcomp::String("Missing target CID parameter"
                        " for command %1\n").Arg(mode);
                });

                return false;
            }

            int32_t targetCID = p.ReadS32Little();
            characterManager->TeamLeaderUpdate(teamID, cid, connection,
                targetCID);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_GROUP_KICK:
        {
            // Kick a member
            if(p.Left() < 4)
            {
                LogTeamError([&]()
                {
                    return libcomp::String("Missing target CID parameter"
                        " for command %1\n").Arg(mode);
                });

                return false;
            }

            int32_t targetCID = p.ReadS32Little();
            characterManager->TeamKick(cLogin, targetCID, teamID);
        }
        break;
    case InternalPacketAction_t::PACKET_ACTION_TEAM_ZIOTITE:
        {
            // Update team ziotite
            if(p.Left() < 5)
            {
                LogTeamError([&]()
                {
                    return libcomp::String("Missing ziotite amount parameters"
                        " for command %1\n").Arg(mode);
                });

                return false;
            }

            int32_t sZiotite = p.ReadS32Little();
            int8_t lZiotite = p.ReadS8();

            characterManager->TeamZiotiteUpdate(teamID, cLogin, sZiotite,
                lZiotite);
        }
        break;
    default:
        break;
    }

    return true;
}
