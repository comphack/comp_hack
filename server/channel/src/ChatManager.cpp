/**
 * @file server/channel/src/ChatManager.cpp
 * @ingroup channel
 *
 * @author HikaruM
 *
 * @brief Manages Chat Messages and GM Commands
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#include "ChatManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <Character.h>
#include <Demon.h>
#include <EntityStats.h>
#include <MiDevilData.h>
#include <MiNPCBasicData.h>

// channel Includes
#include <ChannelServer.h>
#include <ClientState.h>

using namespace channel;

ChatManager::ChatManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

ChatManager::~ChatManager()
{
}

bool ChatManager::SendChatMessage(const std::shared_ptr<
    ChannelClientConnection>& client, ChatType_t chatChannel,
    libcomp::String message)
{
    if(message.IsEmpty())
    {
        return false;
    }

    auto character = client->GetClientState()->GetCharacterState()
        ->GetCharacter();
    libcomp::String sentFrom = character->GetName();

    ChatVis_t visibility = ChatVis_t::CHAT_VIS_SELF;
    switch(chatChannel)
    {
        case ChatType_t::CHAT_PARTY:
            visibility = ChatVis_t::CHAT_VIS_PARTY;
            LOG_INFO(libcomp::String("[Party]:  %1: %2\n.").Arg(sentFrom)
                .Arg(message));
            break;
        case ChatType_t::CHAT_SHOUT:
            visibility = ChatVis_t::CHAT_VIS_ZONE;
            LOG_INFO(libcomp::String("[Shout]:  %1: %2\n.").Arg(sentFrom)
                .Arg(message));
            break;
        case ChatType_t::CHAT_SAY:
            visibility = ChatVis_t::CHAT_VIS_RANGE;
            LOG_INFO(libcomp::String("[Say]:  %1: %2\n.").Arg(sentFrom)
                .Arg(message));
            break;
        default:
            return false;
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
    reply.WriteU16Little((uint16_t)chatChannel);
    reply.WriteString16Little(client->GetClientState()
            ->GetClientStringEncoding(), sentFrom, true);
    reply.WriteU16Little((uint16_t)(message.Size() + 1));
    reply.WriteArray(message.C(), (uint32_t)(message.Size()));
    reply.WriteBlank((uint32_t)(81 - message.Size()));

    switch(visibility)
    {
        case ChatVis_t::CHAT_VIS_SELF:
            client->SendPacket(reply);
            break;
        case ChatVis_t::CHAT_VIS_PARTY:
        case ChatVis_t::CHAT_VIS_ZONE:
        case ChatVis_t::CHAT_VIS_RANGE:
        case ChatVis_t::CHAT_VIS_KLAN:
        case ChatVis_t::CHAT_VIS_TEAM:
        case ChatVis_t::CHAT_VIS_GLOBAL:
        case ChatVis_t::CHAT_VIS_GMS:
        default:
            return false;
    }

    return true;
}

bool ChatManager::ExecuteGMCommand(const std::shared_ptr<
    channel::ChannelClientConnection>& client, GMCommand_t cmd,
    const std::list<libcomp::String>& args)
{
    switch(cmd)
    {
        case GMCommand_t::GM_COMMAND_CONTRACT:
            return GMCommand_Contract(client, args);
        case GMCommand_t::GM_COMMAND_EXPERTISE_UPDATE:
            return GMCommand_ExpertiseUpdate(client, args);
        case GMCommand_t::GM_COMMAND_LEVEL_UP:
            return GMCommand_LevelUp(client, args);
        case GMCommand_t::GM_COMMAND_LNC:
            return GMCommand_LNC(client, args);
        case GMCommand_t::GM_COMMAND_XP:
            return GMCommand_XP(client, args);
        default:
            break;
    }

    return false;
}

bool ChatManager::GMCommand_Contract(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    int16_t demonID;
    if(!GetIntegerArg<int16_t>(demonID, argsCopy))
    {
        libcomp::String name;
        if(!GetStringArg(name, argsCopy, libcomp::Convert::Encoding_t::ENCODING_CP932))
        {
            return false;
        }

        auto devilData = definitionManager->GetDevilData(name);
        if(devilData == nullptr)
        {
            return false;
        }
        demonID = (int16_t)devilData->GetBasic()->GetID();
    }

    auto state = client->GetClientState();
    auto character = state->GetCharacterState()
        ->GetCharacter();

    auto demon = characterManager->ContractDemon(character.Get(),
        definitionManager->GetDevilData((uint32_t)demonID),
        nullptr);
    if(nullptr == demon)
    {
        return false;
    }

    state->SetObjectID(demon->GetUUID(),
        server->GetNextObjectID());

    int8_t slot = -1;
    for(size_t i = 0; i < 10; i++)
    {
        if(character->GetCOMP(i).Get() == demon)
        {
            slot = (int8_t)i;
            break;
        }
    }

    characterManager->SendCOMPDemonData(client, 0, slot,
        state->GetObjectID(demon->GetUUID()));

    return true;
}

bool ChatManager::GMCommand_ExpertiseUpdate(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();

    uint32_t skillID;
    if(!GetIntegerArg<uint32_t>(skillID, argsCopy))
    {
        return false;
    }

    server->GetCharacterManager()->UpdateExpertise(client, skillID);

    return true;
}

bool ChatManager::GMCommand_LevelUp(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    int8_t lvl;
    if(GetIntegerArg<int8_t>(lvl, argsCopy))
    {
        if(lvl > 99 || lvl < 1)
        {
            return false;
        }
    }
    else
    {
        // Increase by 1
        lvl = -1;
    }

    auto state = client->GetClientState();

    libcomp::String target;
    bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
    int32_t entityID;
    int8_t currentLevel;
    if(isDemon)
    {
        entityID = state->GetDemonState()->GetEntityID();
        currentLevel = state->GetDemonState()->GetDemon()
            ->GetCoreStats()->GetLevel();
    }
    else
    {
        entityID = state->GetCharacterState()->GetEntityID();
        currentLevel = state->GetCharacterState()->GetCharacter()
            ->GetCoreStats()->GetLevel();
    }

    if(lvl == -1 && lvl != 99)
    {
        lvl = static_cast<int8_t>(currentLevel + 1);
    }
    else if(currentLevel >= lvl)
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->LevelUp(client, lvl, entityID);

    return true;
}

bool ChatManager::GMCommand_LNC(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    int16_t lnc;
    if(!GetIntegerArg<int16_t>(lnc, argsCopy))
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->UpdateLNC(client, lnc);

    return true;
}

bool ChatManager::GMCommand_XP(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    uint64_t xpGain;
    if(!GetIntegerArg<uint64_t>(xpGain, argsCopy))
    {
        return false;
    }

    auto state = client->GetClientState();

    libcomp::String target;
    bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
    auto entityID = isDemon ? state->GetDemonState()->GetEntityID()
        : state->GetCharacterState()->GetEntityID();

    mServer.lock()->GetCharacterManager()->ExperienceGain(client, xpGain, entityID);

    return true;
}

bool ChatManager::GetStringArg(libcomp::String& outVal,
    std::list<libcomp::String>& args,
    libcomp::Convert::Encoding_t encoding) const
{
    if(args.size() == 0)
    {
        return false;
    }

    outVal = args.front();
    args.pop_front();

    if(encoding != libcomp::Convert::Encoding_t::ENCODING_UTF8)
    {
        auto convertedBytes = libcomp::Convert::ToEncoding(
            encoding, outVal, false);
        outVal = libcomp::String (std::string(convertedBytes.begin(),
            convertedBytes.end()));
    }

    return true;
}
