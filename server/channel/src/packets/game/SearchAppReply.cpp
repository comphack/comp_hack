/**
 * @file server/channel/src/packets/game/SearchAppReply.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to perform an action based off
 *  a search entry application.
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
#include <ManagerPacket.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "ManagerConnection.h"

using namespace channel;

bool Parsers::SearchAppReply::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 16)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto syncManager = server->GetChannelSyncManager();
    auto worldDB = server->GetWorldDatabase();

    int32_t parentType = p.ReadS32Little();
    int32_t parentEntryID = p.ReadS32Little();
    int32_t replyEntryID = p.ReadS32Little();
    int32_t actionType = p.ReadS32Little();

    std::shared_ptr<objects::SearchEntry> parent;
    for(auto e : syncManager->GetSearchEntries(
        (objects::SearchEntry::Type_t)parentType))
    {
        if(e->GetEntryID() == parentEntryID)
        {
            parent = e;
            break;
        }
    }

    std::shared_ptr<objects::SearchEntry> replyEntry;
    for(auto e : syncManager->GetSearchEntries(
        (objects::SearchEntry::Type_t)(parentType + 1)))
    {
        if(e->GetEntryID() == replyEntryID)
        {
            replyEntry = e;
            break;
        }
    }

    bool success = false;
    if(parent && replyEntry &&
        parent->GetSourceCID() != replyEntry->GetSourceCID())
    {
        switch((objects::SearchEntry::Type_t)parentType)
        {
        case objects::SearchEntry::Type_t::PARTY_JOIN:
            if(actionType == 0)
            {
                auto target = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, replyEntry->GetRelatedTo());

                if(target)
                {
                    auto member = state->GetPartyCharacter(true);

                    libcomp::Packet request;
                    request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
                    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES);
                    member->SavePacket(request, false);
                    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                        target->GetName(), true);
                    request.WriteU32Little(0);  // Unknown party ID, let the world decide what to do

                    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

                    success = true;
                }
            }
            break;
        case objects::SearchEntry::Type_t::PARTY_RECRUIT:
            if(actionType == 0)
            {
                auto target = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, replyEntry->GetRelatedTo());

                if(target)
                {
                    auto member = state->GetPartyCharacter(true);

                    libcomp::Packet request;
                    request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
                    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_YN_REQUEST);
                    member->SavePacket(request, false);
                    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                        target->GetName(), true);

                    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

                    success = true;
                }
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_JOIN:
            if(actionType == 0 && !state->GetClanID())
            {
                auto target = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, replyEntry->GetRelatedTo());

                if(target)
                {
                    libcomp::Packet request;
                    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
                    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES);
                    request.WriteS32Little(state->GetWorldCID());
                    request.WriteS32Little(0);
                    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                        target->GetName(), true);

                    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

                    success = true;
                }
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_RECRUIT:
            if(actionType == 0 && state->GetClanID())
            {
                if(p.Left() == (uint32_t)(2 + p.PeekU16Little()))
                {
                    libcomp::String targetName = p.ReadString16Little(
                        libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                    libcomp::Packet request;
                    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
                    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_YN_REQUEST);
                    request.WriteS32Little(state->GetWorldCID());
                    request.WriteS32Little(state->GetClanID());
                    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
                        targetName, true);

                    server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);

                    success = true;
                }
            }
            break;
        case objects::SearchEntry::Type_t::TRADE_SELLING:
        case objects::SearchEntry::Type_t::TRADE_BUYING:
        case objects::SearchEntry::Type_t::FREE_RECRUIT:
            // These are just tells and clear requests
            break;
        default:
            LogGeneralError([&]()
            {
                return libcomp::String("Invalid SearchAppReply type "
                    "encountered: %1\n").Arg(parentType);
            });

            break;
        }

        if(actionType == 1)
        {
            // Clear request
            auto entry = std::make_shared<objects::SearchEntry>(*replyEntry);
            entry->SetLastAction(objects::SearchEntry::LastAction_t::REMOVE_MANUAL);

            success = syncManager->SyncRecordRemoval(entry, "SearchEntry");
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_APPLICATION_REPLY);
    reply.WriteS32Little(parentType);
    reply.WriteS32Little(parentEntryID);
    reply.WriteS32Little(replyEntryID);
    reply.WriteS32Little(actionType);
    reply.WriteS32Little(success ? 0 : -1);

    client->SendPacket(reply);

    return true;
}
