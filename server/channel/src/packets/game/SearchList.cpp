/**
 * @file server/channel/src/packets/game/SearchList.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to get a list of search entries of
 *  a specified type that can be filtered.
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

// object Includes
#include <Clan.h>
#include <ClanMember.h>
#include <EventInstance.h>
#include <EventState.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"

using namespace channel;

bool Parsers::SearchList::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 12)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto syncManager = server->GetChannelSyncManager();
    auto worldDB = server->GetWorldDatabase();

    int32_t type = p.ReadS32Little();
    int32_t pageID = p.ReadS32Little();
    int32_t unused = p.ReadS32Little(); // Always zero?
    (void)unused;

    auto entries = syncManager->GetSearchEntries((objects::SearchEntry::Type_t)type);

    bool success = false;

    // Verify the filters and apply to the list of entries
    bool clanEventView = false;
    size_t maxPageSize = 8;
    switch((objects::SearchEntry::Type_t)type)
    {
    case objects::SearchEntry::Type_t::PARTY_JOIN:
    case objects::SearchEntry::Type_t::PARTY_RECRUIT:
        if(p.Left() == 1)
        {
            int8_t filter = p.ReadS8();

            if(filter != 0)
            {
                entries.remove_if([filter](
                    const std::shared_ptr<objects::SearchEntry>& entry)
                    {
                        return entry->GetData(SEARCH_IDX_GOAL) != filter;
                    });
            }

            success = true;
        }
        break;
    case objects::SearchEntry::Type_t::CLAN_JOIN:
        if(p.Left() == 2)
        {
            int8_t filter = p.ReadS8();
            int8_t viewMode = p.ReadS8();

            if(filter != 0)
            {
                entries.remove_if([filter](
                    const std::shared_ptr<objects::SearchEntry>& entry)
                    {
                        return entry->GetData(SEARCH_IDX_GOAL) != filter;
                    });
            }

            clanEventView = viewMode == 0;

            if(clanEventView)
            {
                maxPageSize = 16;
            }

            success = true;
        }
        break;
    case objects::SearchEntry::Type_t::CLAN_RECRUIT:
        if(p.Left() == 2)
        {
            int8_t filter = p.ReadS8();
            int8_t viewMode = p.ReadS8();

            if(filter != 0)
            {
                entries.remove_if([filter](
                    const std::shared_ptr<objects::SearchEntry>& entry)
                    {
                        return entry->GetData(SEARCH_IDX_GOAL) != filter;
                    });
            }

            clanEventView = viewMode == 0;
            if(clanEventView)
            {
                // Filter out zones that are not in the current event zone
                auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
                    connection);
                auto state = client->GetClientState();
                auto current = state->GetEventState()->GetCurrent();
                int32_t eventZoneID = state->GetCurrentMenuShopID();
                if(eventZoneID != 0)
                {
                    entries.remove_if([eventZoneID](
                        const std::shared_ptr<objects::SearchEntry>& entry)
                        {
                            return entry->GetData(SEARCH_IDX_LOCATION) != eventZoneID;
                        });
                }

                maxPageSize = 4;
            }

            success = true;
        }
        break;
    case objects::SearchEntry::Type_t::TRADE_SELLING:
    case objects::SearchEntry::Type_t::TRADE_BUYING:
        if(p.Left() == 6)
        {
            int8_t subCategory = p.ReadS8();
            int32_t itemType = p.ReadS32Little();
            int8_t mainCategory = p.ReadS8();

            entries.remove_if([itemType, mainCategory, subCategory](
                const std::shared_ptr<objects::SearchEntry>& entry)
                {
                    return (itemType != 0 &&
                            entry->GetData(SEARCH_IDX_ITEM_TYPE) != itemType) ||
                        (mainCategory != 0 &&
                            entry->GetData(SEARCH_IDX_MAIN_CATEGORY) != mainCategory) ||
                        (subCategory != 0 &&
                            entry->GetData(SEARCH_IDX_SUB_CATEGORY) != subCategory);
                });

            maxPageSize = 10;

            success = true;
        }
        break;
    case objects::SearchEntry::Type_t::FREE_RECRUIT:
        if(p.Left() == 4)
        {
            int32_t filter = p.ReadS32Little();

            if(filter != 0)
            {
                entries.remove_if([filter](
                    const std::shared_ptr<objects::SearchEntry>& entry)
                    {
                        return entry->GetData(SEARCH_IDX_GOAL) != filter;
                    });
            }

            success = true;
        }
        break;
    case objects::SearchEntry::Type_t::PARTY_JOIN_APP:
    case objects::SearchEntry::Type_t::PARTY_RECRUIT_APP:
    case objects::SearchEntry::Type_t::CLAN_JOIN_APP:
    case objects::SearchEntry::Type_t::CLAN_RECRUIT_APP:
    case objects::SearchEntry::Type_t::TRADE_SELLING_APP:
    case objects::SearchEntry::Type_t::TRADE_BUYING_APP:
        if(p.Left() == 4)
        {
            int32_t parentID = p.ReadS32Little();

            if(parentID != 0)
            {
                entries.remove_if([parentID](
                    const std::shared_ptr<objects::SearchEntry>& entry)
                    {
                        return entry->GetParentEntryID() != parentID;
                    });
            }

            maxPageSize = 10;

            success = true;
        }
        break;
    default:
        LogGeneralError([&]()
        {
            return libcomp::String("Invalid SearchList type encountered: %1\n")
                .Arg(type);
        });
        break;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_LIST);
    reply.WriteS32Little(type);

    if(success)
    {
        reply.WriteS32Little(0);    // Success

        std::list<std::shared_ptr<objects::SearchEntry>> current;
        std::shared_ptr<objects::SearchEntry> prev;
        std::shared_ptr<objects::SearchEntry> next;
        for(auto entry : entries)
        {
            // If page ID is not zero, current starts after that value
            if(current.size() >= maxPageSize)
            {
                next = entry;
                break;
            }
            else if(current.size() > 0 || pageID == 0)
            {
                current.push_back(entry);
            }
            else if(entry->GetEntryID() >= pageID)
            {
                prev = entry;
            }
            else
            {
                current.push_back(entry);
            }
        }

        // Write previous (or first) entry ID
        if(!prev && current.size() > 0)
        {
            reply.WriteS32Little(current.front()->GetEntryID());
        }
        else
        {
            reply.WriteS32Little(prev ? prev->GetEntryID() : -1);
        }

        switch((objects::SearchEntry::Type_t)type)
        {
        case objects::SearchEntry::Type_t::PARTY_JOIN:
        case objects::SearchEntry::Type_t::PARTY_RECRUIT:
            for(auto entry : current)
            {
                reply.WriteS32Little(entry->GetEntryID());
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_LOCATION));

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo());

                if(character)
                {
                    auto stats = libcomp::PersistentObject::LoadObjectByUUID<
                        objects::EntityStats>(worldDB, character->GetCoreStats().GetUUID());

                    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        character->GetName(), true);

                    reply.WriteS8(stats ? stats->GetLevel() : 0);
                }
                else
                {
                    reply.WriteBlank(3);
                }

                reply.WriteS8(0);   // Unknown

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_JOIN:
            for(auto entry : current)
            {
                reply.WriteS32Little(entry->GetEntryID());
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PLAYSTYLE));

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo());

                if(character)
                {
                    auto stats = libcomp::PersistentObject::LoadObjectByUUID<
                        objects::EntityStats>(worldDB, character->GetCoreStats().GetUUID());

                    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        character->GetName(), true);

                    reply.WriteS8(stats ? stats->GetLevel() : 0);
                }
                else
                {
                    reply.WriteBlank(3);
                }

                reply.WriteS8(2);   // Unknown

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);   // Comment

                if(clanEventView)
                {
                    /// @todo: Not sure what the difference is, adding 1-3 additional
                    /// bytes does not break it but adding a 4th does. Maybe some data
                    /// in the same structure should be different? Either way this seems
                    /// to be the cause for the event views not working for your own
                    /// search entries
                }
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_RECRUIT:
            for(auto entry : current)
            {
                reply.WriteS32Little(entry->GetEntryID());
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_PLAYSTYLE));

                auto clan = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Clan>(worldDB, entry->GetRelatedTo());

                if(clan)
                {
                    std::shared_ptr<objects::ClanMember> masterMember;
                    for(auto member : objects::ClanMember::LoadClanMemberListByClan(worldDB,
                        clan->GetUUID()))
                    {
                        if(member->GetMemberType() == objects::ClanMember::MemberType_t::MASTER)
                        {
                            masterMember = member;
                            break;
                        }
                    }

                    auto master = masterMember
                        ? libcomp::PersistentObject::LoadObjectByUUID<objects::Character>(
                            worldDB, masterMember->GetCharacter()) : nullptr;
                    auto masterStats = master
                        ? libcomp::PersistentObject::LoadObjectByUUID<objects::EntityStats>(
                            worldDB, master->GetCoreStats().GetUUID()) : nullptr;

                    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        clan->GetName(), true);
                    reply.WriteS32Little((int32_t)clan->MembersCount());
                    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        entry->GetTextData(SEARCH_IDX_CLAN_CATCHPHRASE), true);

                    reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_CLAN_IMAGE));

                    reply.WriteS8(clan->GetLevel());
                    reply.WriteS8(masterStats ? masterStats->GetLevel() : 0);

                    reply.WriteU8(clan->GetEmblemBase());
                    reply.WriteU8(clan->GetEmblemSymbol());
                    reply.WriteU8(clan->GetEmblemColorR1());
                    reply.WriteU8(clan->GetEmblemColorG1());
                    reply.WriteU8(clan->GetEmblemColorB1());
                    reply.WriteU8(clan->GetEmblemColorR2());
                    reply.WriteU8(clan->GetEmblemColorG2());
                    reply.WriteU8(clan->GetEmblemColorB2());
                }
                else
                {
                    reply.WriteBlank(19);
                }

                if(clanEventView)
                {
                    /// @todo: Same issue as clan join comment
                }
            }
            break;
        case objects::SearchEntry::Type_t::TRADE_SELLING:
            for(auto entry : current)
            {
                reply.WriteS32Little(entry->GetEntryID());
                reply.WriteS8(0);   // Unknown

                reply.WriteS32Little(entry->GetData(SEARCH_IDX_ITEM_TYPE));

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_SUB_CATEGORY));
                reply.WriteS8(0);   // Unknown
                reply.WriteS8(0);   // Unknown
                reply.WriteS32Little(entry->GetData(SEARCH_IDX_PRICE));
            }
            break;
        case objects::SearchEntry::Type_t::TRADE_BUYING:
            for(auto entry : current)
            {
                reply.WriteS32Little(entry->GetEntryID());
                reply.WriteS8(0);   // Unknown

                reply.WriteS32Little(entry->GetData(SEARCH_IDX_ITEM_TYPE));

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_SUB_CATEGORY));
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_SLOT_COUNT));
                reply.WriteS32Little(entry->GetData(SEARCH_IDX_PRICE));
            }
            break;
        case objects::SearchEntry::Type_t::FREE_RECRUIT:
            for(auto entry : current)
            {
                reply.WriteS32Little(entry->GetEntryID());
                reply.WriteS8((int8_t)entry->GetData(SEARCH_IDX_GOAL));

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo());

                if(character)
                {
                    auto stats = libcomp::PersistentObject::LoadObjectByUUID<
                        objects::EntityStats>(worldDB, character->GetCoreStats().GetUUID());

                    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                        character->GetName(), true);

                    reply.WriteS8(stats ? stats->GetLevel() : 0);
                }
                else
                {
                    reply.WriteBlank(3);
                }

                reply.WriteS8(0);   // Unknown

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    entry->GetTextData(SEARCH_IDX_COMMENT), true);
            }
            break;
        case objects::SearchEntry::Type_t::PARTY_JOIN_APP:
        case objects::SearchEntry::Type_t::PARTY_RECRUIT_APP:
        case objects::SearchEntry::Type_t::CLAN_JOIN_APP:
        case objects::SearchEntry::Type_t::CLAN_RECRUIT_APP:
        case objects::SearchEntry::Type_t::TRADE_SELLING_APP:
        case objects::SearchEntry::Type_t::TRADE_BUYING_APP:
            for(auto entry : current)
            {
                reply.WriteS32Little(entry->GetEntryID());

                auto character = libcomp::PersistentObject::LoadObjectByUUID<
                    objects::Character>(worldDB, entry->GetRelatedTo());

                reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                    character ? character->GetName() : "", true);

                reply.WriteBlank(3);    // Padding?
            }
            break;
        default:
            break;
        }

        reply.WriteS32Little(-1);   // End marker
        reply.WriteS32Little(next ? next->GetEntryID() : -1);
    }
    else
    {
        LogGeneralError([&]()
        {
            return libcomp::String("SearchList with type '%1' request was "
                "not valid\n").Arg(type);
        });

        reply.WriteS32Little(-1);
    }

    connection->SendPacket(reply);

    return true;
}
