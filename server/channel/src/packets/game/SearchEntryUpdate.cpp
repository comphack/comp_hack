/**
 * @file server/channel/src/packets/game/SearchEntryUpdate.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to update an existing search entry.
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

using namespace channel;

bool Parsers::SearchEntryUpdate::Parse(libcomp::ManagerPacket *pPacketManager,
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

    int32_t type = p.ReadS32Little();
    int32_t entryID = p.ReadS32Little();

    std::shared_ptr<objects::SearchEntry> existing;
    for(auto entry : syncManager->GetSearchEntries((objects::SearchEntry::Type_t)type))
    {
        if(entry->GetEntryID() == entryID)
        {
            existing = entry;
            break;
        }
    }

    bool success = false;
    if(!existing)
    {
        LOG_ERROR(libcomp::String("SearchEntryUpdate with invalid entry ID"
            " encountered: %1\n").Arg(type));
    }
    else if(existing->GetSourceCID() != state->GetWorldCID())
    {
        LOG_ERROR(libcomp::String("SearchEntryUpdate request encountered"
            " with an entry ID associated to a different player: %1\n").Arg(type));
    }
    else
    {
        success = true;
    }

    if(success)
    {
        // Copy the existing record and let the callback update the
        // existing one
        auto entry = std::make_shared<objects::SearchEntry>(*existing);
        entry->SetLastAction(objects::SearchEntry::LastAction_t::UPDATE);

        switch((objects::SearchEntry::Type_t)type)
        {
        case objects::SearchEntry::Type_t::CLAN_JOIN:
        case objects::SearchEntry::Type_t::CLAN_RECRUIT:
            if(p.Left() >= 5)
            {
                // Play style specified
                uint8_t specified = p.ReadU8();
                if(specified == 1)
                {
                    if(p.Left() >= 1)
                    {
                        int8_t playStyle = p.ReadS8();
                        entry->SetData(SEARCH_IDX_PLAYSTYLE, playStyle);
                    }
                    else
                    {
                        break;
                    }
                }

                // Play time specified
                if(p.Left() == 0)
                {
                    break;
                }

                specified = p.ReadU8();
                if(specified == 1)
                {
                    if(p.Left() >= 4)
                    {
                        int16_t timeFrom = p.ReadS16Little();
                        int16_t timeTo = p.ReadS16Little();

                        entry->SetData(SEARCH_IDX_TIME_FROM, timeFrom);
                        entry->SetData(SEARCH_IDX_TIME_TO, timeTo);
                    }
                    else
                    {
                        break;
                    }
                }

                // Preferred series specified
                if(p.Left() == 0)
                {
                    break;
                }

                specified = p.ReadU8();
                if(specified == 1)
                {
                    if(p.Left() >= 1)
                    {
                        int8_t preferredSeries = p.ReadS8();

                        entry->SetData(SEARCH_IDX_PREF_SERIES, preferredSeries);
                    }
                    else
                    {
                        break;
                    }
                }

                // Preferred demon specified
                if(p.Left() == 0)
                {
                    break;
                }

                specified = p.ReadU8();
                if(specified == 1)
                {
                    if(p.Left() >= 2)
                    {
                        int8_t preferredDemonRace = p.ReadS8();
                        int8_t preferredDemon = p.ReadS8();

                        entry->SetData(SEARCH_IDX_PREF_DEMON, preferredDemon);
                        entry->SetData(SEARCH_IDX_PREF_DEMON_RACE, preferredDemonRace);
                    }
                    else
                    {
                        break;
                    }
                }

                // Comment specified
                if(p.Left() == 0)
                {
                    break;
                }

                specified = p.ReadU8();
                if(specified == 1)
                {
                    if(p.Left() >= (uint32_t)(2 + p.PeekU16Little()))
                    {
                        libcomp::String comment = p.ReadString16Little(
                            libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                        entry->SetTextData(SEARCH_IDX_COMMENT, comment);
                    }
                    else
                    {
                        break;
                    }
                }

                // Additional data specified for recruiting
                if(type == (int32_t)objects::SearchEntry::Type_t::CLAN_RECRUIT)
                {
                    // Catch phrase specified
                    if(p.Left() == 0)
                    {
                        break;
                    }

                    specified = p.ReadU8();
                    if(specified == 1)
                    {
                        if(p.Left() >= (uint32_t)(2 + p.PeekU16Little()))
                        {
                            libcomp::String catchphrase = p.ReadString16Little(
                                libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                            entry->SetTextData(SEARCH_IDX_CLAN_CATCHPHRASE, catchphrase);
                        }
                        else
                        {
                            break;
                        }
                    }

                    // Image specified
                    if(p.Left() == 0)
                    {
                        break;
                    }

                    specified = p.ReadU8();
                    if(specified == 1)
                    {
                        if(p.Left() >= 1)
                        {
                            int8_t image = p.ReadS8();

                            entry->SetData(SEARCH_IDX_CLAN_IMAGE, image);
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::PARTY_JOIN:
        case objects::SearchEntry::Type_t::PARTY_RECRUIT:
        case objects::SearchEntry::Type_t::TRADE_SELLING:
        case objects::SearchEntry::Type_t::TRADE_BUYING:
        case objects::SearchEntry::Type_t::FREE_RECRUIT:
            // The client re-registers instead of updates for these types
            LOG_ERROR(libcomp::String("Unsupported SearchEntryUpdate type"
                " encountered: %1\n").Arg(type));
            break;
        default:
            LOG_ERROR(libcomp::String("Invalid SearchEntryUpdate type"
                " encountered: %1\n").Arg(type));
            break;
        }
    
        if(success)
        {
            if(syncManager->UpdateRecord(entry, "SearchEntry"))
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
            LOG_ERROR(libcomp::String("Invalid SearchEntryUpdate request"
                " encountered: %1\n").Arg(type));
        }
    }

    if(!success)
    {
        // If it succeeds, the reply will be sent when the callback returns
        // from the world server
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_UPDATE);
        reply.WriteS32Little(type);
        reply.WriteS32Little(-1);
        reply.WriteS32Little(entryID);

        connection->SendPacket(reply);
    }

    return true;
}
