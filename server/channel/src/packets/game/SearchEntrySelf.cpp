/**
 * @file server/channel/src/packets/game/SearchEntrySelf.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for the player's search entries of
 *  a specified type.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

bool Parsers::SearchEntrySelf::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 4)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto syncManager = server->GetChannelSyncManager();
    auto worldCID = state->GetWorldCID();

    int32_t type = p.ReadS32Little();

    auto entries = syncManager->GetSearchEntries((objects::SearchEntry::Type_t)type);
    entries.remove_if([worldCID](
        const std::shared_ptr<objects::SearchEntry>& entry)
        {
            return entry->GetSourceCID() != worldCID;
        });

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_SELF);
    reply.WriteS32Little(type);
    reply.WriteS32Little(0);    // Success

    int32_t previousPageIndexID = -1;
    int32_t nextPageIndexID = -1;

    switch((objects::SearchEntry::Type_t)type)
    {
    case objects::SearchEntry::Type_t::PARTY_JOIN:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_LOCATION));

            reply.WriteS32Little((int32_t)entry->GetPostTime());

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);
        }
        break;
    case objects::SearchEntry::Type_t::PARTY_RECRUIT:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_LOCATION));

            reply.WriteS32Little((int32_t)entry->GetPostTime());

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PARTY_SIZE));
        }
        break;
    case objects::SearchEntry::Type_t::CLAN_JOIN:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PLAYSTYLE));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_FROM));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_TO));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_SERIES));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON));

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON_RACE));

            reply.WriteS32Little(ChannelServer::GetExpirationInSeconds(
                entry->GetExpirationTime()));
        }
        break;
    case objects::SearchEntry::Type_t::CLAN_RECRUIT:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PLAYSTYLE));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_FROM));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_TO));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_SERIES));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON));

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON_RACE));

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_CLAN_CATCHPHRASE), true);

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_CLAN_IMAGE));

            reply.WriteS32Little(ChannelServer::GetExpirationInSeconds(
                entry->GetExpirationTime()));
        }
        break;
    case objects::SearchEntry::Type_t::TRADE_SELLING:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());
            reply.WriteS8(0);   // Unknown
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_SUB_CATEGORY));

            reply.WriteS16Little(0);   // Unknown
            reply.WriteS16Little(0);   // Unknown

            reply.WriteS32Little(entry->GetData(SEARCH_IDX_ITEM_TYPE));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_DURABILITY));
            reply.WriteS32Little(entry->GetData(SEARCH_IDX_PRICE));
            reply.WriteS32Little(0);    // Unknown
            reply.WriteS32Little(entry->GetData(SEARCH_IDX_LOCATION));

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);

            reply.WriteS16Little((int16_t)(entry->GetData(SEARCH_IDX_DURABILITY) * 1000));

            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 1));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 2));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 3));
            reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 4));

            reply.WriteS32Little((int32_t)entry->GetPostTime());
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_MAIN_CATEGORY));

            reply.WriteS32Little(-1);    // Unknown
            reply.WriteS32Little(-1);    // Unknown
        }
        break;
    case objects::SearchEntry::Type_t::TRADE_BUYING:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());
            reply.WriteS8(0);   // Unknown
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_SUB_CATEGORY));

            reply.WriteS32Little(entry->GetData(SEARCH_IDX_ITEM_TYPE));
            reply.WriteS32Little(entry->GetData(SEARCH_IDX_PRICE));
            reply.WriteS32Little(0);    // Unknown
            reply.WriteS32Little(entry->GetData(SEARCH_IDX_LOCATION));

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);

            reply.WriteS32Little((int32_t)entry->GetPostTime());
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_SLOT_COUNT));
            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_MAIN_CATEGORY));
        }
        break;
    case objects::SearchEntry::Type_t::FREE_RECRUIT:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());

            reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);

            reply.WriteS32Little((int32_t)entry->GetPostTime());
        }
        break;
    case objects::SearchEntry::Type_t::PARTY_JOIN_APP:
    case objects::SearchEntry::Type_t::PARTY_RECRUIT_APP:
    case objects::SearchEntry::Type_t::CLAN_JOIN_APP:
    case objects::SearchEntry::Type_t::CLAN_RECRUIT_APP:
    case objects::SearchEntry::Type_t::TRADE_SELLING_APP:
    case objects::SearchEntry::Type_t::TRADE_BUYING_APP:
        for(auto entry : entries)
        {
            reply.WriteS32Little(entry->GetEntryID());

            reply.WriteS32Little(entry->GetParentEntryID());

            reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                entry->GetTextData(SEARCH_IDX_COMMENT), true);
        }
        break;
    default:
        break;
    }

    reply.WriteS32Little(previousPageIndexID);
    reply.WriteS32Little(nextPageIndexID);

    connection->SendPacket(reply);

    return true;
}
