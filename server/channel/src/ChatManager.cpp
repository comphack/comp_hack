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
#include <AccountLogin.h>
#include <Character.h>
#include <MiItemData.h>
#include <MiDevilData.h>
#include <MiNPCBasicData.h>
#include <MiSkillItemStatusCommonData.h>

// channel Includes
#include <ChannelServer.h>
#include <ClientState.h>
#include <ZoneManager.h>

// Standard C Includes
#include <cstdlib>

using namespace channel;

ChatManager::ChatManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mGMands["contract"] = &ChatManager::GMCommand_Contract;
    mGMands["crash"] = &ChatManager::GMCommand_Crash;
    mGMands["expertiseup"] = &ChatManager::GMCommand_ExpertiseUpdate;
    mGMands["item"] = &ChatManager::GMCommand_Item;
    mGMands["levelup"] = &ChatManager::GMCommand_LevelUp;
    mGMands["lnc"] = &ChatManager::GMCommand_LNC;
    mGMands["pos"] = &ChatManager::GMCommand_Position;
    mGMands["xp"] = &ChatManager::GMCommand_XP;
    mGMands["zone"] =&ChatManager::GMCommand_Zone;
}

ChatManager::~ChatManager()
{
}

bool ChatManager::SendChatMessage(const std::shared_ptr<
    ChannelClientConnection>& client, ChatType_t chatChannel,
    libcomp::String message)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    if(message.IsEmpty())
    {
        return false;
    }

    auto state = client->GetClientState();
    
    auto encodedMessage = libcomp::Convert::ToEncoding(
        state->GetClientStringEncoding(), message, false);

    auto character = state->GetCharacterState()->GetEntity();
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
        case ChatType_t::CHAT_SELF:
            visibility = ChatVis_t::CHAT_VIS_SELF;
            LOG_INFO(libcomp::String("[Self]:  %1: %2\n.").Arg(sentFrom)
                .Arg(message));
            break;
        default:
            return false;
    }

    // Make sure the message is clamped to the max size to prevent
    // bad math on the zero'd section of the packet. This may not
    // react well to multi-byte characters (CP932).
    if(MAX_MESSAGE_LENGTH < encodedMessage.size())
    {
        encodedMessage.resize(MAX_MESSAGE_LENGTH);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
    reply.WriteU16Little((uint16_t)chatChannel);
    reply.WriteString16Little(state->GetClientStringEncoding(),
        sentFrom, true);
    reply.WriteU16Little((uint16_t)(encodedMessage.size() + 1));
    reply.WriteArray(encodedMessage);
    reply.WriteBlank((uint32_t)(MAX_MESSAGE_LENGTH + 1 -
        encodedMessage.size()));

    switch(visibility)
    {
        case ChatVis_t::CHAT_VIS_SELF:
            client->SendPacket(reply);
            break;
        case ChatVis_t::CHAT_VIS_ZONE:
            zoneManager->BroadcastPacket(client, reply, true);
            break;
        case ChatVis_t::CHAT_VIS_RANGE:
            /// @todo: Figure out how to force it to use a radius for range.
            zoneManager->SendToRange(client, reply, true);
            break;
        case ChatVis_t::CHAT_VIS_PARTY:
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
    channel::ChannelClientConnection>& client, const libcomp::String& cmd,
    const std::list<libcomp::String>& args)
{
    auto it = mGMands.find(cmd);

    if(it != mGMands.end())
    {
        return it->second(*this, client, args);
    }

    LOG_WARNING(libcomp::String("Unknown GM command encountered: %1\n").Arg(
        cmd));

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
    auto character = state->GetCharacterState()->GetEntity();

    auto demon = characterManager->ContractDemon(character,
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

bool ChatManager::GMCommand_Crash(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    (void)client;
    (void)args;

    abort();
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

bool ChatManager::GMCommand_Item(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    uint32_t itemID;
    if(!GetIntegerArg<uint32_t>(itemID, argsCopy))
    {
        libcomp::String name;
        if(!GetStringArg(name, argsCopy, libcomp::Convert::Encoding_t::ENCODING_CP932))
        {
            return false;
        }

        auto itemData = definitionManager->GetItemData(name);
        if(itemData == nullptr)
        {
            return false;
        }
        itemID = itemData->GetCommon()->GetID();
    }

    uint16_t stackSize;
    if(!GetIntegerArg<uint16_t>(stackSize, argsCopy))
    {
        stackSize = 1;
    }

    return server->GetCharacterManager()
        ->AddRemoveItem(client, itemID, stackSize, true);
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
        currentLevel = state->GetDemonState()->GetEntity()
            ->GetCoreStats()->GetLevel();
    }
    else
    {
        entityID = state->GetCharacterState()->GetEntityID();
        currentLevel = state->GetCharacterState()->GetEntity()
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

bool ChatManager::GMCommand_Position(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    ServerTime stopTime;
    ServerTime startTime;
    startTime = stopTime = server->GetServerTime();
    ClientTime start = client->GetClientState()->ToClientTime(startTime);
    ClientTime stop = client->GetClientState()->ToClientTime(stopTime);

    std::list<libcomp::String> argsCopy = args;

    if(!cState)
    {
        LOG_DEBUG("Bad character state.\n");

        return false;
    }

    if (!args.empty() && args.size() == 2)
    {
        //LOG_DEBUG("Argument list is not empty.\n");
        float destX = 0.0;
        float destY = 0.0;
        float ratePerSec = 0.0; //hardcoded for testing purposes
        GetDecimalArg<float>(destX, argsCopy);
        GetDecimalArg<float>(destY, argsCopy);
        cState->SetOriginX(destX);
        cState->SetOriginY(destY);
        cState->SetOriginTicks(startTime);
        cState->SetDestinationX(destX);
        cState->SetDestinationY(destY);
        cState->SetDestinationTicks(stopTime);
        
        libcomp::Packet reply;
        libcomp::Packet dreply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE);
        dreply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MOVE);
        //uint32_t resetPos = reply.Size();
        reply.WriteS32Little(cState->GetEntityID());
        reply.WriteFloat(destX);
        reply.WriteFloat(destY);
        reply.WriteFloat(destX);
        reply.WriteFloat(destY);
        reply.WriteFloat(ratePerSec);
        reply.WriteFloat(start);
        reply.WriteFloat(stop);
        zoneManager->BroadcastPacket(client, reply, true);
        if (dState->GetEntity())
        {
            dState->SetOriginX(destX);
            dState->SetOriginY(destY);
            dState->SetOriginTicks(startTime);
            dState->SetDestinationX(destX);
            dState->SetDestinationY(destY);
            dState->SetDestinationTicks(stopTime);

           // reply.Seek(resetPos);
            dreply.WriteS32Little(dState->GetEntityID());
            dreply.WriteFloat(destX);
            dreply.WriteFloat(destY);
            dreply.WriteFloat(destX);
            dreply.WriteFloat(destY);
            dreply.WriteFloat(ratePerSec);
            dreply.WriteFloat(start);
            dreply.WriteFloat(stop);
            zoneManager->BroadcastPacket(client, dreply, true);
        }
        
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Destination Set: (%1, %2)").Arg(cState->GetDestinationX()).Arg(
                cState->GetDestinationY()));
    }

    if(!args.empty() && args.size() != 2)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@pos requires zero or two args"));
    }

    /// @todo We may need to use the time to calculate where between the origin
    /// and destination the character is.
    return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "Position: (%1, %2)").Arg(cState->GetDestinationX()).Arg(
        cState->GetDestinationY()));
}

bool ChatManager::GMCommand_Zone(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    uint32_t zoneID = 0;
    float xCoord = 0.00;
    float yCoord = 0.00;
    float rotation =0.00;
    std::list<libcomp::String> argsCopy = args;

    if(args.empty())
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Error: @Zone requires at least a zoneID, or a zoneID and (x,y) coordinates."));
    }
    else
    {
        GetIntegerArg<uint32_t>(zoneID, argsCopy);
        auto zoneDefinition = server->GetServerDataManager()->GetZoneData(zoneID);
        if(!zoneDefinition)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String("ERROR: INVALID ZONE ID.  Please enter a proper zoneID and try again."));
        }
        zoneManager->LeaveZone(client);
        //copy zoneID from args.
        if(args.size() == 3)
        {
            //pull x coord
            bool xTrue = GetDecimalArg<float>(xCoord, argsCopy);
            //pull y coord
            bool yTrue = GetDecimalArg<float>(yCoord, argsCopy);
            if(!xTrue||!yTrue)
            {
                return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String("ERROR: One of the inputs is not a number.  Please re-enter the command with proper inputs."));
            }
        }
        zoneManager->EnterZone(client, zoneID, xCoord, yCoord, rotation);
        
        return true;
    }
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
