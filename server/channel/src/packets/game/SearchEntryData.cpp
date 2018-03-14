/**
 * @file server/channel/src/packets/game/SearchEntryData.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client for detailed information about a
 *  specific search entry.
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

// object Includes
#include <Clan.h>
#include <ClanMember.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::SearchEntryData::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 8)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto syncManager = server->GetChannelSyncManager();
    auto worldDB = server->GetWorldDatabase();

    int32_t type = p.ReadS32Little();
    int32_t entryID = p.ReadS32Little();

    std::shared_ptr<objects::SearchEntry> entry;
    for(auto e : syncManager->GetSearchEntries((objects::SearchEntry::Type_t)type))
    {
        if(e->GetEntryID() == entryID)
        {
            entry = e;
            break;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_DATA);
    reply.WriteS32Little(type);
    reply.WriteS32Little(entryID);
    
    if(entry)
    {
        reply.WriteS32Little(0);    // Success

        switch((objects::SearchEntry::Type_t)type)
        {
        case objects::SearchEntry::Type_t::PARTY_JOIN:
            {
                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo().GetUUID());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS32Little((int32_t)entry->GetPostTime());

                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));
                reply.WriteBlank(5);    // Unknown
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_LOCATION));
            }
            break;
        case objects::SearchEntry::Type_t::PARTY_RECRUIT:
            {
                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo().GetUUID());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS32Little((int32_t)entry->GetPostTime());

                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));

                reply.WriteBlank(5);    // Unknown
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_LOCATION));
                reply.WriteBlank(7);    // Unknown
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PARTY_SIZE));
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_JOIN:
            {
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_SERIES));
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON));
                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo().GetUUID());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS32Little(ChannelServer::GetExpirationInSeconds(
                    entry->GetExpirationTime()));

                reply.WriteS8(0);   // Login state
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_FROM));
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_TO));
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON_RACE));
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_RECRUIT:
            {
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_SERIES));
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON));
                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_FROM));
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_TIME_TO));
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PREF_DEMON_RACE));
                
                auto clan = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Clan>(worldDB, entry->GetRelatedTo().GetUUID());

                int8_t averageLevel = 0;
                if(clan)
                {
                    std::shared_ptr<objects::ClanMember> masterMember;
                    std::list<libobjgen::UUID> memberUIDs;
                    for(auto member : objects::ClanMember::LoadClanMemberListByClan(worldDB,
                        clan))
                    {
                        if(member->GetMemberType() == objects::ClanMember::MemberType_t::MASTER)
                        {
                            masterMember = member;
                        }
                        memberUIDs.push_back(member->GetCharacter().GetUUID());
                    }

                    auto master = masterMember
                        ? libcomp::PersistentObject::LoadObjectByUUID<objects::Character>(
                            worldDB, masterMember->GetCharacter().GetUUID()) : nullptr;

                    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        clan->GetName(), true);
                    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        master ? master->GetName() : "", true);

                    uint8_t memberCount = 0;
                    uint16_t levelSum = 0;
                    for(libobjgen::UUID& uuid : memberUIDs)
                    {
                        auto stats = objects::EntityStats::LoadEntityStatsByEntity(worldDB,
                            uuid);
                        if(stats)
                        {
                            memberCount++;
                            levelSum = (uint16_t)(levelSum + stats->GetLevel());
                        }
                    }

                    averageLevel = (int8_t)((levelSum * 1.f) / (memberCount * 1.f));
                }
                else
                {
                    reply.WriteBlank(4);
                }

                reply.WriteS32Little(ChannelServer::GetExpirationInSeconds(
                    entry->GetExpirationTime()));

                reply.WriteS8(0);   // Connection status
                reply.WriteS32Little((int32_t)(clan ? clan->GetBaseZoneID() : 0));
                reply.WriteS8(averageLevel);
            }
            break;
        case objects::SearchEntry::Type_t::TRADE_SELLING:
            {
                reply.WriteS32Little(0);   // Unknown
                reply.WriteS32Little(entry->GetData(SEARCH_IDX_LOCATION));

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo().GetUUID());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS32Little((int32_t)entry->GetPostTime());

                reply.WriteS16Little((int16_t)(entry->GetData(SEARCH_IDX_TAROT)));
                reply.WriteS16Little((int16_t)(entry->GetData(SEARCH_IDX_SOUL)));
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_MAX_DURABILITY));
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_IDX_DURABILITY));
                reply.WriteS32Little(entry->GetData(SEARCH_IDX_PRICE));
                reply.WriteS16Little(0);    // Unknown

                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT));
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 1));
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 2));
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 3));
                reply.WriteS16Little((int16_t)entry->GetData(SEARCH_BASE_MOD_SLOT + 4));

                reply.WriteS32Little(entry->GetData(SEARCH_IDX_BASIC_EFFECT));
                reply.WriteS32Little(entry->GetData(SEARCH_IDX_SPECIAL_EFFECT));
            }
            break;
        case objects::SearchEntry::Type_t::TRADE_BUYING:
            {
                reply.WriteS32Little(0);   // Unknown
                reply.WriteS32Little(entry->GetData(SEARCH_IDX_LOCATION));

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo().GetUUID());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS32Little((int32_t)entry->GetPostTime());

                reply.WriteS32Little(entry->GetData(SEARCH_IDX_PRICE));
            }
            break;
        case objects::SearchEntry::Type_t::FREE_RECRUIT:
            {
                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo().GetUUID());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS32Little((int32_t)entry->GetPostTime());

                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));
            }
            break;
        case objects::SearchEntry::Type_t::PARTY_JOIN_APP:
        case objects::SearchEntry::Type_t::PARTY_RECRUIT_APP:
        case objects::SearchEntry::Type_t::CLAN_JOIN_APP:
        case objects::SearchEntry::Type_t::CLAN_RECRUIT_APP:
        case objects::SearchEntry::Type_t::TRADE_SELLING_APP:
        case objects::SearchEntry::Type_t::TRADE_BUYING_APP:
            {
                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo().GetUUID());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS32Little((int32_t)entry->GetPostTime());

                reply.WriteS32Little(0);    // Unknown
                reply.WriteS32Little(0);    // (Unused) response zone ID
            }
            break;
        default:
            break;
        }
    }
    else
    {
        reply.WriteS32Little(-1);   // Failure
    }

    connection->SendPacket(reply);

    return true;
}
