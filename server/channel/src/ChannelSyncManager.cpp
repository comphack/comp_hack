/**
 * @file server/channel/src/ChannelSyncManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel specific implementation of the DataSyncManager in
 * charge of performing server side update operations.
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

#include "ChannelSyncManager.h"

// libcomp Includes
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

ChannelSyncManager::ChannelSyncManager()
{
}

ChannelSyncManager::ChannelSyncManager(const std::weak_ptr<
    ChannelServer>& server) : mServer(server)
{
}

ChannelSyncManager::~ChannelSyncManager()
{
}

bool ChannelSyncManager::Initialize()
{
    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();

    // Build the configs
    auto cfg = std::make_shared<ObjectConfig>(
        "SearchEntry", false);
    cfg->BuildHandler = &DataSyncManager::New<objects::SearchEntry>;
    cfg->UpdateHandler = &DataSyncManager::Update<ChannelSyncManager,
        objects::SearchEntry>;

    mRegisteredTypes["SearchEntry"] = cfg;

    cfg = std::make_shared<ObjectConfig>("Account", false, lobbyDB);

    mRegisteredTypes["Account"] = cfg;

    // Add the world connection
    const std::set<std::string> worldTypes = { "Account", "SearchEntry" };

    auto worldConnection = server->GetManagerConnection()
        ->GetWorldConnection();
    return RegisterConnection(worldConnection, worldTypes);
}

libcomp::EnumMap<objects::SearchEntry::Type_t,
    std::list<std::shared_ptr<objects::SearchEntry>>> ChannelSyncManager::GetSearchEntries() const
{
    return mSearchEntries;
}

std::list<std::shared_ptr<objects::SearchEntry>> ChannelSyncManager::GetSearchEntries(
    objects::SearchEntry::Type_t type)
{
    std::lock_guard<std::mutex> lock(mLock);

    auto it = mSearchEntries.find(type);
    return it != mSearchEntries.end() ? it->second : std::list<std::shared_ptr<objects::SearchEntry>>();
}

namespace channel
{
template<>
int8_t ChannelSyncManager::Update<objects::SearchEntry>(const libcomp::String& type,
    const std::shared_ptr<libcomp::Object>& obj, bool isRemove,
    const libcomp::String& source)
{
    (void)type;
    (void)source;

    bool success = false;

    auto entry = std::dynamic_pointer_cast<objects::SearchEntry>(obj);

    auto& entryList = mSearchEntries[entry->GetType()];

    auto it = entryList.begin();
    while(it != entryList.end())
    {
        if((*it)->GetEntryID() == entry->GetEntryID())
        {
            it = entryList.erase(it);
            if(!isRemove)
            {
                // Replace the existing element
                entryList.insert(it, entry);
            }

            success = true;
            break;
        }

        it++;
    }

    if(!success)
    {
        if(isRemove)
        {
            LOG_WARNING(libcomp::String("No SearchEntry with ID '%1' found"
                " for sync removal\n").Arg(entry->GetEntryID()));
        }
        else
        {
            entryList.push_front(entry);

            // Re-sort by entry ID, highest first
            entryList.sort([](
                const std::shared_ptr<objects::SearchEntry>& a,
                const std::shared_ptr<objects::SearchEntry>& b)
                {
                    return a->GetEntryID() > b->GetEntryID();
                });

            success = true;
        }
    }

    if(success)
    {
        bool isApp = (int8_t)entry->GetType() % 2 == 1;

        std::shared_ptr<objects::SearchEntry> parent;
        if(isApp)
        {
            auto parentType = (objects::SearchEntry::Type_t)(
                (int8_t)entry->GetType() - 1);
            for(auto entry2 : mSearchEntries[parentType])
            {
                if(entry2->GetEntryID() == entry->GetParentEntryID())
                {
                    parent = entry2;
                    break;
                }
            }
        }

        // If an app is being removed, inform both characters involved, otherwise
        // just inform the source character
        std::set<int32_t> cids = { entry->GetSourceCID() };
        if(isRemove && parent)
        {
            cids.insert(parent->GetSourceCID());
        }

        auto server = mServer.lock();
        for(int32_t cid : cids)
        {
            auto client = server->GetManagerConnection()->GetEntityClient(
                cid, true);
            if(client)
            {
                int32_t removeReason = 0;
                auto packetCode = ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REGISTER;
                switch(entry->GetLastAction())
                {
                case objects::SearchEntry::LastAction_t::UPDATE:
                    // Only clan search entries are ever actually updated, all others are re-registered
                    if(entry->GetType() != objects::SearchEntry::Type_t::CLAN_JOIN &&
                        entry->GetType() != objects::SearchEntry::Type_t::CLAN_RECRUIT)
                    {
                        packetCode = ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_UPDATE;
                    }
                    break;
                case objects::SearchEntry::LastAction_t::REMOVE_MANUAL:
                    packetCode = ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REMOVE;
                    break;
                case objects::SearchEntry::LastAction_t::REMOVE_LOGOFF:
                    packetCode = ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REMOVE;
                    removeReason = 6;
                    break;
                case objects::SearchEntry::LastAction_t::REMOVE_EXPIRE:
                    packetCode = ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REMOVE;
                    removeReason = 1;
                    break;
                case objects::SearchEntry::LastAction_t::REMOVE_SPECIAL:
                    packetCode = ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REMOVE;
                    removeReason = 2;
                    break;
                case objects::SearchEntry::LastAction_t::ADD:
                default:
                    break;
                }

                libcomp::Packet reply;
                reply.WritePacketCode(packetCode);
                reply.WriteS32Little((int32_t)entry->GetType());

                if(packetCode == ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REGISTER)
                {
                    reply.WriteS32Little(0);    // Success
                    reply.WriteS32Little(entry->GetEntryID());
                }
                else
                {
                    reply.WriteS32Little(entry->GetEntryID());
                    reply.WriteS32Little(0);    // Success

                    if(packetCode == ChannelToClientPacketCode_t::PACKET_SEARCH_ENTRY_REMOVE)
                    {
                        reply.WriteS32Little(removeReason);
                    }
                }

                client->SendPacket(reply);
            }
        }

        // If the type is odd, it is an application so we need to notify
        // the person being replied to if they are on this channel
        if(isApp && entry->GetLastAction() == objects::SearchEntry::LastAction_t::ADD &&
            parent)
        {
            auto client = parent ? server->GetManagerConnection()->GetEntityClient(
                parent->GetSourceCID(), true) : nullptr;
            if(client)
            {
                libcomp::Packet notify;
                notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SEARCH_APPLICATION);
                notify.WriteS32Little((int32_t)parent->GetType());
                notify.WriteS32Little(parent->GetEntryID());
                notify.WriteS32Little(entry->GetEntryID());

                client->SendPacket(notify);
            }
        }
    }

    return success ? SYNC_UPDATED : SYNC_FAILED;
}
}
