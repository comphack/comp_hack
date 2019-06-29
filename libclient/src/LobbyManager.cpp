/**
 * @file libcomp/src/LobbyManager.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Manages the active client connection to the server.
 *
 * This file is part of the COMP_hack Client Library (libclient).
 *
 * Copyright (C) 2012-2019 COMP_hack Team <compomega@tutanota.com>
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

#include "LobbyManager.h"

// libclient Includes
#include "LogicWorker.h"

// libcomp Includes
#include <EnumUtils.h>
#include <Log.h>
#include <PacketCodes.h>

// packets Includes
#include <PacketLobbyWorldList.h>
#include <PacketLobbyWorldListChannelEntry.h>
#include <PacketLobbyWorldListEntry.h>

using namespace logic;

using libcomp::Message::MessageType;

LobbyManager::LobbyManager(LogicWorker *pLogicWorker,
    const std::weak_ptr<libcomp::MessageQueue<libcomp::Message::Message *>>
        &messageQueue) :
    libcomp::Manager(),
    /*mLogicWorker(pLogicWorker),*/ mMessageQueue(messageQueue)
{
    (void)pLogicWorker;
}

LobbyManager::~LobbyManager()
{
}

std::list<libcomp::Message::MessageType> LobbyManager::GetSupportedTypes() const
{
    return {
        MessageType::MESSAGE_TYPE_PACKET,
    };
}

bool LobbyManager::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    switch(to_underlying(pMessage->GetType()))
    {
        case to_underlying(MessageType::MESSAGE_TYPE_PACKET):
            return ProcessPacketMessage(
                (const libcomp::Message::Packet *)pMessage);
        default:
            break;
    }

    return false;
}

bool LobbyManager::ProcessPacketMessage(
    const libcomp::Message::Packet *pMessage)
{
    libcomp::ReadOnlyPacket p(pMessage->GetPacket());

    switch(pMessage->GetCommandCode())
    {
        case to_underlying(LobbyToClientPacketCode_t::PACKET_WORLD_LIST):
            return HandlePacketLobbyWorldList(p);
        default:
            break;
    }

    return false;
}

bool LobbyManager::HandlePacketLobbyWorldList(libcomp::ReadOnlyPacket &p)
{
    auto obj = std::make_shared<packets::PacketLobbyWorldList>();

    if(!obj->LoadPacket(p, false) || p.Left())
    {
        return false;
    }

    bool changed =
        !mWorldList || obj->WorldsCount() != mWorldList->WorldsCount();

    if(!changed)
    {
        auto originalList = mWorldList->GetWorlds();
        auto originalIterator = originalList.begin();

        for(auto world : obj->GetWorlds())
        {
            auto original = *originalIterator;
            changed |= original->GetID() != world->GetID() ||
                       original->GetName() != world->GetName() ||
                       original->ChannelsCount() != world->ChannelsCount();

            if(!changed)
            {
                auto originalChannelList = original->GetChannels();
                auto originalChannelIterator = originalChannelList.begin();

                for(auto channel : world->GetChannels())
                {
                    auto originalChannel = *originalChannelIterator;

                    changed |=
                        originalChannel->GetName() != channel->GetName() ||
                        originalChannel->GetVisibility() !=
                            channel->GetVisibility();
                }
            }

            if(changed)
            {
                break;
            }

            originalIterator++;
        }
    }

    // Save the world list now that we know if it changed or not.
    mWorldList = obj;

    if(changed)
    {
        /// @todo Send an update to refresh the list
    }
    else
    {
        /// @todo Send an update for latency
    }

    return true;
}
