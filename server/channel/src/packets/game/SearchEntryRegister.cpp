/**
 * @file server/channel/src/packets/game/SearchEntryRegister.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to register a new search entry.
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
#include <Constants.h>
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <Clan.h>
#include <SearchEntry.h>

// channel Includes
#include "ChannelServer.h"
#include "ChannelSyncManager.h"

using namespace channel;

bool Parsers::SearchEntryRegister::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() < 4)
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto state = client->GetClientState();
    auto syncManager = server->GetChannelSyncManager();

    int32_t type = p.ReadS32Little();

    // Find any existing conflicting records (skip check if odd type signifying
    // an application as well as buying/selling which can have multiple)
    std::shared_ptr<objects::SearchEntry> existing;
    if(type % 2 != 1 &&
        type != (int32_t)objects::SearchEntry::Type_t::TRADE_BUYING &&
        type != (int32_t)objects::SearchEntry::Type_t::TRADE_SELLING)
    {
        for(auto ePair : syncManager->GetSearchEntries())
        {
            // Ignore applications
            if((int8_t)ePair.first % 2 == 1) continue;

            // Only one non-trade, non-clan recruit entry is allowed at one time
            if(ePair.first != objects::SearchEntry::Type_t::TRADE_BUYING &&
                ePair.first != objects::SearchEntry::Type_t::TRADE_SELLING &&
                ((type == (int32_t)objects::SearchEntry::Type_t::CLAN_JOIN) ==
                    (ePair.first == objects::SearchEntry::Type_t::CLAN_JOIN)) &&
                ((type == (int32_t)objects::SearchEntry::Type_t::CLAN_RECRUIT) ==
                    (ePair.first == objects::SearchEntry::Type_t::CLAN_RECRUIT)))
            {
                for(auto entry : ePair.second)
                {
                    if(entry->GetSourceCID() == state->GetWorldCID())
                    {
                        existing = entry;
                        break;
                    }
                }
            }
        }
    }

    auto entry = std::make_shared<objects::SearchEntry>();
    entry->SetSourceCID(state->GetWorldCID());
    entry->SetRelatedTo(state->GetCharacterState()->GetEntity()->GetUUID());
    entry->SetType((objects::SearchEntry::Type_t)type);
    entry->SetLastAction(objects::SearchEntry::LastAction_t::ADD);
    entry->SetPostTime((uint32_t)std::time(0));

    bool success = true;
    if(existing)
    {
        if(existing->GetType() != entry->GetType())
        {
            LogGeneralError([&]()
            {
                return libcomp::String("SearchEntryRegister request "
                    "encountered while conflicting entry of a different "
                    "type exists: %1\n").Arg(type);
            });

            success = false;
        }
        else
        {
            // Replace the old record
            entry->SetEntryID(existing->GetEntryID());
        }
    }

    if(success)
    {
        success = false;
        switch((objects::SearchEntry::Type_t)type)
        {
        case objects::SearchEntry::Type_t::PARTY_JOIN:
            if(p.Left() >= 8)
            {
                int8_t goal = p.ReadS8();
                int8_t location = p.ReadS8();
                int16_t unknown1 = p.ReadS16Little();
                int16_t unknown2 = p.ReadS16Little();

                if(p.Left() < (uint32_t)(2 + p.PeekU16Little()))
                {
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                (void)unknown1;
                (void)unknown2;

                entry->SetData(SEARCH_IDX_GOAL, goal);
                entry->SetData(SEARCH_IDX_LOCATION, location);
                entry->SetTextData(SEARCH_IDX_COMMENT, comment);

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::PARTY_RECRUIT:
            if(p.Left() >= 9)
            {
                int8_t goal = p.ReadS8();
                int8_t location = p.ReadS8();
                int16_t unknown1 = p.ReadS16Little();
                int16_t unknown2 = p.ReadS16Little();

                if(p.Left() < (uint32_t)(3 + p.PeekU16Little()))
                {
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);
                int8_t partySize = p.ReadS8();

                (void)unknown1;
                (void)unknown2;

                entry->SetData(SEARCH_IDX_GOAL, goal);
                entry->SetData(SEARCH_IDX_LOCATION, location);
                entry->SetData(SEARCH_IDX_PARTY_SIZE, partySize);
                entry->SetTextData(SEARCH_IDX_COMMENT, comment);

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_JOIN:
            if(p.Left() >= 10)
            {
                int8_t playStyle = p.ReadS8();
                int16_t timeFrom = p.ReadS16Little();
                int16_t timeTo = p.ReadS16Little();
                int8_t preferredSeries = p.ReadS8();
                int8_t preferredDemon = p.ReadS8();

                if(p.Left() < (uint32_t)(3 + p.PeekU16Little()))
                {
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);
                int8_t preferredDemonRace = p.ReadS8();

                entry->SetData(SEARCH_IDX_PLAYSTYLE, playStyle);
                entry->SetData(SEARCH_IDX_TIME_FROM, timeFrom);
                entry->SetData(SEARCH_IDX_TIME_TO, timeTo);
                entry->SetData(SEARCH_IDX_PREF_SERIES, preferredSeries);
                entry->SetData(SEARCH_IDX_PREF_DEMON, preferredDemon);
                entry->SetData(SEARCH_IDX_PREF_DEMON_RACE, preferredDemonRace);
                entry->SetTextData(SEARCH_IDX_COMMENT, comment);

                // Expires after 10 days if not cancelled before
                entry->SetExpirationTime((uint32_t)(entry->GetPostTime() + 864000));

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::CLAN_RECRUIT:
            if(p.Left() >= 13)
            {
                auto cState = state->GetCharacterState();
                auto clan = cState->GetEntity()->GetClan().Get();

                if(!clan)
                {
                    LogGeneralErrorMsg("SearchEntryRegister request encountered"
                        " for clan recruitment when the requestor is not "
                        "in a clan\n");

                    break;
                }

                int8_t playStyle = p.ReadS8();
                int16_t timeFrom = p.ReadS16Little();
                int16_t timeTo = p.ReadS16Little();
                int8_t preferredSeries = p.ReadS8();
                int8_t preferredDemon = p.ReadS8();

                if(p.Left() < (uint32_t)(6 + p.PeekU16Little()))
                {
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);
                int8_t preferredDemonRace = p.ReadS8();

                if(p.Left() < (uint32_t)(3 + p.PeekU16Little()))
                {
                    break;
                }

                libcomp::String catchphrase = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);
                int8_t image = p.ReadS8();

                entry->SetData(SEARCH_IDX_PLAYSTYLE, playStyle);
                entry->SetData(SEARCH_IDX_TIME_FROM, timeFrom);
                entry->SetData(SEARCH_IDX_TIME_TO, timeTo);
                entry->SetData(SEARCH_IDX_PREF_SERIES, preferredSeries);
                entry->SetData(SEARCH_IDX_PREF_DEMON, preferredDemon);
                entry->SetData(SEARCH_IDX_PREF_DEMON_RACE, preferredDemonRace);
                entry->SetData(SEARCH_IDX_CLAN_IMAGE, image);
                entry->SetTextData(SEARCH_IDX_COMMENT, comment);
                entry->SetTextData(SEARCH_IDX_CLAN_CATCHPHRASE, catchphrase);
                entry->SetRelatedTo(state->GetCharacterState()->GetEntity()->
                    GetClan().GetUUID());

                // Pull out the base for event view filtering
                entry->SetData(SEARCH_IDX_LOCATION, (int32_t)clan->GetBaseZoneID());

                // Expires after 3 weeks if not cancelled before
                entry->SetExpirationTime((uint32_t)(entry->GetPostTime() + 1814400));

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::TRADE_SELLING:
            if(p.Left() >= 46)
            {
                int8_t unknown1 = p.ReadS8();
                int8_t subCategory = p.ReadS8();
                int16_t tarot = p.ReadS16Little();
                int16_t soul = p.ReadS16Little();
                int32_t itemType = p.ReadS32Little();
                int8_t maxDurability = p.ReadS8();
                int32_t price = p.ReadS32Little();
                int32_t unknown4 = p.ReadS32Little();
                int32_t location = p.ReadS32Little();

                if(p.Left() < (uint32_t)(23 + p.PeekU16Little()))
                {
                    break;
                }

                auto itemData = server->GetDefinitionManager()->GetItemData(
                    (uint32_t)itemType);
                if(!itemData)
                {
                    // Invalid item supplied, garbage sent or malformed
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                int16_t durability = p.ReadS16Little();

                uint16_t modSlots[5];
                modSlots[0] = p.ReadU16Little();
                modSlots[1] = p.ReadU16Little();
                modSlots[2] = p.ReadU16Little();
                modSlots[3] = p.ReadU16Little();
                modSlots[4] = p.ReadU16Little();

                int8_t mainCategory = p.ReadS8();
                int32_t basicEffect = p.ReadS32Little();
                int32_t specialEffect = p.ReadS32Little();

                (void)unknown1;
                (void)unknown4;

                entry->SetData(SEARCH_IDX_ITEM_TYPE, itemType);
                entry->SetData(SEARCH_IDX_MAIN_CATEGORY, mainCategory);
                entry->SetData(SEARCH_IDX_SUB_CATEGORY, subCategory);
                entry->SetData(SEARCH_IDX_PRICE, price);
                entry->SetData(SEARCH_IDX_LOCATION, location);
                entry->SetData(SEARCH_IDX_DURABILITY, durability);
                entry->SetData(SEARCH_IDX_MAX_DURABILITY, maxDurability);
                entry->SetData(SEARCH_IDX_TAROT, tarot);
                entry->SetData(SEARCH_IDX_SOUL, soul);
                entry->SetData(SEARCH_BASE_MOD_SLOT, (int32_t)modSlots[0]);
                entry->SetData(SEARCH_BASE_MOD_SLOT + 1, (int32_t)modSlots[1]);
                entry->SetData(SEARCH_BASE_MOD_SLOT + 2, (int32_t)modSlots[2]);
                entry->SetData(SEARCH_BASE_MOD_SLOT + 3, (int32_t)modSlots[3]);
                entry->SetData(SEARCH_BASE_MOD_SLOT + 4, (int32_t)modSlots[4]);
                entry->SetData(SEARCH_IDX_BASIC_EFFECT,
                    basicEffect > 0 ? basicEffect : -1);
                entry->SetData(SEARCH_IDX_SPECIAL_EFFECT,
                    specialEffect > 0 ? specialEffect : -1);
                entry->SetTextData(SEARCH_IDX_COMMENT, comment);

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::TRADE_BUYING:
            if(p.Left() >= 10)
            {
                int8_t unknown1 = p.ReadS8();
                int8_t subCategory = p.ReadS8();
                int32_t itemType = p.ReadS32Little();
                int32_t price = p.ReadS32Little();
                int32_t unknown2 = p.ReadS32Little();
                int32_t location = p.ReadS32Little();

                if(p.Left() < (uint32_t)(4 + p.PeekU16Little()))
                {
                    break;
                }

                auto itemData = server->GetDefinitionManager()->GetItemData(
                    (uint32_t)itemType);
                if(!itemData)
                {
                    // Invalid item supplied, garbage sent or malformed
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                int8_t slotCount = p.ReadS8();
                int8_t mainCategory = p.ReadS8();

                (void)unknown1;
                (void)unknown2;

                entry->SetData(SEARCH_IDX_ITEM_TYPE, itemType);
                entry->SetData(SEARCH_IDX_MAIN_CATEGORY, mainCategory);
                entry->SetData(SEARCH_IDX_SUB_CATEGORY, subCategory);
                entry->SetData(SEARCH_IDX_PRICE, price);
                entry->SetData(SEARCH_IDX_LOCATION, location);
                entry->SetData(SEARCH_IDX_SLOT_COUNT, slotCount);
                entry->SetTextData(SEARCH_IDX_COMMENT, comment);

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::FREE_RECRUIT:
            if(p.Left() >= 3)
            {
                int8_t goal = p.ReadS8();

                if(p.Left() < (uint32_t)(2 + p.PeekU16Little()))
                {
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                entry->SetData(SEARCH_IDX_GOAL, goal);
                entry->SetTextData(SEARCH_IDX_COMMENT, comment);

                success = true;
            }
            break;
        case objects::SearchEntry::Type_t::PARTY_JOIN_APP:
        case objects::SearchEntry::Type_t::PARTY_RECRUIT_APP:
        case objects::SearchEntry::Type_t::CLAN_JOIN_APP:
        case objects::SearchEntry::Type_t::CLAN_RECRUIT_APP:
        case objects::SearchEntry::Type_t::TRADE_SELLING_APP:
        case objects::SearchEntry::Type_t::TRADE_BUYING_APP:
            if(p.Left() >= 6)
            {
                if(p.Left() != (uint32_t)(6 + p.PeekU16Little()))
                {
                    break;
                }

                libcomp::String comment = p.ReadString16Little(
                    libcomp::Convert::Encoding_t::ENCODING_CP932, true);

                int32_t parentID = p.ReadS32Little();

                // Make sure a reply to the same parent does not exist
                for(auto entry2 : syncManager->GetSearchEntries(
                    (objects::SearchEntry::Type_t)(type)))
                {
                    if(entry2->GetSourceCID() == state->GetWorldCID() &&
                        entry2->GetParentEntryID() == parentID)
                    {
                        existing = entry2;
                        break;
                    }
                }

                if(existing)
                {
                    LogGeneralErrorMsg("SearchEntryRegister request encountered"
                        " for application with duplicate parent ID\n");

                    break;
                }

                // Make sure the parent exists and is valid
                std::shared_ptr<objects::SearchEntry> parent;
                for(auto entry2 : syncManager->GetSearchEntries(
                    (objects::SearchEntry::Type_t)(type - 1)))
                {
                    if(entry2->GetEntryID() == parentID)
                    {
                        parent = entry2;
                        break;
                    }
                }

                if(parent)
                {
                    entry->SetParentEntryID(parentID);

                    entry->SetTextData(SEARCH_IDX_COMMENT, comment);

                    success = true;
                }
                else
                {
                    LogGeneralErrorMsg("SearchEntryRegister request encountered"
                        " for an application to an invalid parent ID\n");
                }
            }
            break;
        default:
            LogGeneralErrorMsg(libcomp::String("Invalid SearchEntryRegister "
                "type encountered: %1\n").Arg(type));

            break;
        }
    }

    if(success)
    {
        success = syncManager->SyncRecordUpdate(entry, "SearchEntry");
    }
    else
    {
        LogGeneralErrorMsg(libcomp::String("Invalid SearchEntryRegister "
            "request encountered: %1\n").Arg(type));
    }

    if(!success)
    {
        // If it succeeds, the reply will be sent when the callback returns
        // from the world server
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REGISTER);
        reply.WriteS32Little(type);
        reply.WriteS32Little(-1);

        client->SendPacket(reply);
    }

    return true;
}
