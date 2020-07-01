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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <PacketLobbyCharacterList.h>
#include <PacketLobbyCharacterListEntry.h>
#include <PacketLobbyCharacterListEquippedVAEntry.h>
#include <PacketLobbyRequestStartGame.h>
#include <PacketLobbyStartGame.h>

// logic messages
#include <MessageCharacterList.h>
#include <MessageConnected.h>
#include <MessageConnectionInfo.h>
#include <MessageStartGame.h>

using namespace logic;

using libcomp::Message::MessageClientType;
using libcomp::Message::MessageType;

LobbyManager::LobbyManager(LogicWorker *pLogicWorker,
    const std::weak_ptr<libcomp::MessageQueue<libcomp::Message::Message *>>
        &messageQueue) :
    libcomp::Manager(),
    mLogicWorker(pLogicWorker), mMessageQueue(messageQueue)
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
        MessageType::MESSAGE_TYPE_CLIENT,
    };
}

bool LobbyManager::ProcessMessage(const libcomp::Message::Message *pMessage)
{
    switch(to_underlying(pMessage->GetType()))
    {
        case to_underlying(MessageType::MESSAGE_TYPE_PACKET):
            return ProcessPacketMessage(
                (const libcomp::Message::Packet *)pMessage);
        case to_underlying(MessageType::MESSAGE_TYPE_CLIENT):
            return ProcessClientMessage(
                (const libcomp::Message::MessageClient *)pMessage);
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
        case to_underlying(LobbyToClientPacketCode_t::PACKET_CHARACTER_LIST):
            return HandlePacketLobbyCharacterList(p);
        case to_underlying(LobbyToClientPacketCode_t::PACKET_START_GAME):
            return HandlePacketLobbyStartGame(p);
        default:
            break;
    }

    return false;
}

bool LobbyManager::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage)
{
    switch(to_underlying(pMessage->GetMessageClientType()))
    {
        case to_underlying(MessageClientType::REQUEST_START_GAME):
        {
            auto pReqStartGame =
                reinterpret_cast<const MessageRequestStartGame *>(pMessage);

            // Send the start game request packet.
            auto p = std::make_shared<packets::PacketLobbyRequestStartGame>();
            p->SetPacketCode(to_underlying(ClientToLobbyPacketCode_t::PACKET_START_GAME));
            p->SetCharacterID(pReqStartGame->GetCharacterID());
            p->SetUnknown(0);

            mLogicWorker->SendObject(p);

            return true;
        }
        default:
            break;
    }

    return false;
}

bool LobbyManager::HandlePacketLobbyCharacterList(libcomp::ReadOnlyPacket &p)
{
    auto obj = std::make_shared<packets::PacketLobbyCharacterList>();

    if(!obj->LoadPacket(p, false) || p.Left())
    {
        return false;
    }

    bool changed = !mCharacterList || obj->CharactersCount() != mCharacterList->CharactersCount();

    if(!changed)
    {
        auto originalList = mCharacterList->GetCharacters();
        auto originalIterator = originalList.begin();

        for(auto character : obj->GetCharacters())
        {
            auto original = *originalIterator;
            changed |= original->GetCharacterID() != character->GetCharacterID() ||
                original->GetName() != character->GetName() ||
                original->GetWorldID() != character->GetWorldID() ||
                original->GetKillTime() != character->GetKillTime();

            if(changed)
            {
                break;
            }

            originalIterator++;
        }

    }

    mCharacterList = obj;


    if(changed)
    {
        std::stringstream ss;

        auto characterObjClone = std::make_shared<packets::PacketLobbyCharacterList>();

        if(obj->Save(ss))
        {
            if(characterObjClone->Load(ss))
            {
                mLogicWorker->SendToGame(new MessageCharacterList(characterObjClone));
            }
        }
    }

    return true;
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

bool LobbyManager::HandlePacketLobbyStartGame(libcomp::ReadOnlyPacket &p)
{
    auto obj = std::make_shared<packets::PacketLobbyStartGame>();

    if(!obj->LoadPacket(p, false) || p.Left())
    {
        return false;
    }

    auto serverComponents = obj->GetServer().Split(":");

    if(2 != serverComponents.size())
    {
        return false;
    }

    bool ok = false;

    auto serverAddr = serverComponents.front();
    auto serverPort = serverComponents.back().ToInteger<uint16_t>(&ok);

    if(!ok)
    {
        return false;
    }

    /// @todo Name the connection with the GUI?
    mLogicWorker->SendToLogic(new MessageConnectToChannel(
        obj->GetSessionKey(), "channel@1", serverAddr, serverPort));

    return true;
}
