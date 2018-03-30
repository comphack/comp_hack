/**
 * @file server/channel/src/packets/game/SearchEntryRemove.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to remove an existing search entry.
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
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"

using namespace channel;

bool Parsers::SearchEntryRemove::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 8)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto syncManager = server->GetChannelSyncManager();
    int32_t worldCID = state->GetWorldCID();

    int32_t type = p.ReadS32Little();
    int32_t entryID = p.ReadS32Little();

    std::shared_ptr<objects::SearchEntry> existing;
    for(auto entry : syncManager->GetSearchEntries((objects::SearchEntry::Type_t)type))
    {
        if(entry->GetEntryID() == entryID ||
            (entryID == 0 && worldCID == entry->GetSourceCID()))
        {
            existing = entry;
            break;
        }
    }

    bool success = false;
    if(!existing)
    {
        LOG_ERROR(libcomp::String("SearchEntryRemove with invalid entry ID"
            " encountered: %1\n").Arg(type));
    }
    else if(existing->GetSourceCID() != worldCID)
    {
        LOG_ERROR(libcomp::String("SearchEntryRemove request encountered"
            " with an entry ID associated to a different player: %1\n").Arg(type));
    }
    else
    {
        success = true;
    }
    
    if(success)
    {
        // Copy the existing record and let the callback update the
        // existing synced data
        auto entry = std::make_shared<objects::SearchEntry>(*existing);
        entry->SetLastAction(objects::SearchEntry::LastAction_t::REMOVE_MANUAL);

        switch((objects::SearchEntry::Type_t)type)
        {
        case objects::SearchEntry::Type_t::PARTY_JOIN:
        case objects::SearchEntry::Type_t::PARTY_RECRUIT:
        case objects::SearchEntry::Type_t::CLAN_JOIN:
        case objects::SearchEntry::Type_t::CLAN_RECRUIT:
        case objects::SearchEntry::Type_t::TRADE_SELLING:
        case objects::SearchEntry::Type_t::TRADE_BUYING:
        case objects::SearchEntry::Type_t::FREE_RECRUIT:
        case objects::SearchEntry::Type_t::PARTY_JOIN_APP:
        case objects::SearchEntry::Type_t::PARTY_RECRUIT_APP:
        case objects::SearchEntry::Type_t::CLAN_JOIN_APP:
        case objects::SearchEntry::Type_t::CLAN_RECRUIT_APP:
        case objects::SearchEntry::Type_t::TRADE_SELLING_APP:
        case objects::SearchEntry::Type_t::TRADE_BUYING_APP:
            success = true;
            break;
        default:
            LOG_ERROR(libcomp::String("Invalid SearchEntryRemove type"
                " encountered: %1\n").Arg(type));
            break;
        }
    
        if(success)
        {
            if(syncManager->RemoveRecord(entry, "SearchEntry"))
            {
                syncManager->SyncOutgoing();
            }
            else
            {
                success = false;
            }
        }
        else
        {
            LOG_ERROR(libcomp::String("Invalid SearchEntryRemove request"
                " encountered: %1\n").Arg(type));
        }
    }

    if(!success)
    {
        // If it succeeds, the reply will be sent when the callback returns
        // from the world server
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REMOVE);
        reply.WriteS32Little(type);
        reply.WriteS32Little(entryID);
        reply.WriteS32Little(-1);
        reply.WriteS32Little(0);

        connection->SendPacket(reply);
    }

    return true;
}
