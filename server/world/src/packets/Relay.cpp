/**
 * @file server/world/src/packets/Relay.cpp
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Packet relay handler for packets being sent from one channel
 *  to another or sent from the world itself. Bouncebacks will be attempted
 *  if the target players switch from one channel to another after it is
 *  sent.
 *
 * This file is part of the World Server (world).
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

// libcomp Includes
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <Character.h>
#include <CharacterLogin.h>

// world Includes
#include "WorldServer.h"

using namespace world;

bool Parsers::Relay::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 5)
    {
        return false;
    }

    auto channelConnection = std::dynamic_pointer_cast<libcomp::InternalConnection>(
        connection);
    auto server = std::dynamic_pointer_cast<WorldServer>(pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();

    int32_t sourceCID = p.ReadS32Little();
    PacketRelayMode_t mode = (PacketRelayMode_t)p.ReadU8();

    bool reportOffline = false;
    std::list<libcomp::String> reportFailed;
    std::list<std::shared_ptr<objects::CharacterLogin>> targetLogins;
    switch(mode)
    {
    case PacketRelayMode_t::RELAY_FAILURE:
    case PacketRelayMode_t::RELAY_CIDS:
        {
            // The only difference between a CID relay and a failure is the
            // retry check
            auto registeredChannel = mode == PacketRelayMode_t::RELAY_FAILURE
                ? server->GetChannel(channelConnection) : nullptr;

            uint16_t cidCount = p.ReadU16Little();

            std::list<int32_t> cids;
            for(uint16_t i = 0; i < cidCount; i++)
            {
                cids.push_back(p.ReadS32Little());
            }

            for(auto cid : cids)
            {
                auto login = characterManager->GetCharacterLogin(cid);
                if(login)
                {
                    if(login->GetChannelID() >= 0 && (!registeredChannel ||
                        login->GetChannelID() != registeredChannel->GetID()))
                    {
                        // Either first send or we tried to relay the message
                        // but the client changed channels before receiving it
                        targetLogins.push_back(login);
                    }
                    else
                    {
                        reportFailed.push_back(login->GetCharacter()->GetName());
                    }
                }
                else
                {
                    // CID isn't valid, ignore
                }
            }
        }
        break;
    case PacketRelayMode_t::RELAY_ACCOUNT:
        {
            reportOffline = true;

            auto accountUIDStr = p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

            libobjgen::UUID accountUID(accountUIDStr.C());
            auto account = std::dynamic_pointer_cast<objects::Account>(
                libcomp::PersistentObject::GetObjectByUUID(accountUID));

            auto login = account ? server->GetAccountManager()->GetUserLogin(
                account->GetUsername()) : nullptr;
            auto targetLogin = login ? login->GetCharacterLogin() : nullptr;
            if(targetLogin)
            {
                targetLogins.push_back(targetLogin);
            }
            else
            {
                // Account either doesn't exist or hasn't logged in, report failure
                reportFailed.push_back(accountUIDStr);
            }
        }
        break;
    case PacketRelayMode_t::RELAY_CHARACTER:
        {
            reportOffline = true;

            libcomp::String targetName = p.ReadString16Little(
                libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
            auto targetLogin = characterManager->GetCharacterLogin(targetName);
            if(targetLogin)
            {
                targetLogins.push_back(targetLogin);
            }
            else
            {
                // Character either doesn't exist or hasn't logged in, report failure
                reportFailed.push_back(targetName);
            }
        }
        break;
    case PacketRelayMode_t::RELAY_PARTY:
        {
            uint32_t partyID = p.ReadU32Little();
            bool includeSource = p.ReadU8() == 1;

            auto party = characterManager->GetParty(partyID);
            for(auto cid : party->GetMemberIDs())
            {
                // Party members
                auto targetLogin = characterManager->GetCharacterLogin(cid);
                if(targetLogin && (includeSource || cid != sourceCID))
                {
                    targetLogins.push_back(targetLogin);
                }
            }
        }
        break;
    case PacketRelayMode_t::RELAY_CLAN:
        {
            int32_t clanID = p.ReadS32Little();
            bool includeSource = p.ReadU8() == 1;

            auto clanInfo = characterManager->GetClan(clanID);
            for(auto mPair : clanInfo->GetMemberMap())
            {
                // Clan members
                auto targetLogin = characterManager->GetCharacterLogin(mPair.first);
                if(targetLogin && (includeSource || mPair.first != sourceCID))
                {
                    targetLogins.push_back(targetLogin);
                }
            }
        }
        break;
    case PacketRelayMode_t::RELAY_TEAM:
        /// @todo: implement team relaying
        break;
    default:
        LOG_ERROR("Invalid relay mode specified\n");
        return false;
    }

    // Read the actual packet data
    auto packetData = p.ReadArray(p.Left());

    if(targetLogins.size() > 0)
    {
        std::unordered_map<int8_t, std::list<int32_t>> channelMap;
        for(auto c : targetLogins)
        {
            int8_t channelID = c->GetChannelID();
            if(channelID >= 0)
            {
                channelMap[channelID].push_back(c->GetWorldCID());
            }
            else if(reportOffline)
            {
                reportFailed.push_back(c->GetCharacter()->GetName());
            }
        }

        if(channelMap.size() > 0)
        {
            for(auto pair : channelMap)
            {
                auto channel = server->GetChannelConnectionByID(pair.first);

                if(!channel) continue;

                libcomp::Packet relay;
                WorldServer::GetRelayPacket(relay, pair.second, sourceCID);
                relay.WriteArray(packetData);
        
                channel->SendPacket(relay);
            }
        }
    }

    if(reportFailed.size() > 0)
    {
        // If anyone could not have the packet delivered, tell the sender
        auto sourceLogin = characterManager->GetCharacterLogin(sourceCID);
        if(sourceLogin && sourceLogin->GetChannelID() >= 0)
        {
            auto channel = server->GetChannelConnectionByID(
                sourceLogin->GetChannelID());

            if(!channel)
            {
                // Not valid anymore, nothing to do
                return true;
            }
            
            libcomp::Packet failure;
            failure.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
            failure.WriteS32Little(sourceCID);

            failure.WriteU8((uint8_t)PacketRelayMode_t::RELAY_FAILURE);
            failure.WriteU16Little((uint16_t)reportFailed.size());
            for(auto failedName : reportFailed)
            {
                failure.WriteString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_UTF8, failedName, true);
            }

            failure.WriteArray(packetData);

            channel->SendPacket(failure);
        }

    }

    return true;
}
