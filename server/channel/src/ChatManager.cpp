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
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <ServerZone.h>
#include <ChannelConfig.h>

// channel Includes
#include <ChannelServer.h>
#include <ClientState.h>
#include <Git.h>
#include <ZoneManager.h>

// Standard C Includes
#include <cstdlib>

using namespace channel;

ChatManager::ChatManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mGMands["announce"] = &ChatManager::GMCommand_Announce;
    mGMands["contract"] = &ChatManager::GMCommand_Contract;
    mGMands["crash"] = &ChatManager::GMCommand_Crash;
    mGMands["effect"] = &ChatManager::GMCommand_Effect;
    mGMands["enemy"] = &ChatManager::GMCommand_Enemy;
    mGMands["expertiseup"] = &ChatManager::GMCommand_ExpertiseUpdate;
    mGMands["familiarity"] = &ChatManager::GMCommand_Familiarity;
    mGMands["homepoint"] = &ChatManager::GMCommand_Homepoint;
    mGMands["item"] = &ChatManager::GMCommand_Item;
    mGMands["kill"] = &ChatManager::GMCommand_Kill;
    mGMands["levelup"] = &ChatManager::GMCommand_LevelUp;
    mGMands["lnc"] = &ChatManager::GMCommand_LNC;
    mGMands["map"] = &ChatManager::GMCommand_Map;
    mGMands["pos"] = &ChatManager::GMCommand_Position;
    mGMands["skill"] = &ChatManager::GMCommand_Skill;
    mGMands["speed"] = &ChatManager::GMCommand_Speed;
    mGMands["tickermessage"] = &ChatManager::GMCommand_TickerMessage;
    mGMands["version"] = &ChatManager::GMCommand_Version;
    mGMands["xp"] = &ChatManager::GMCommand_XP;
    mGMands["zone"] = &ChatManager::GMCommand_Zone;
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


bool ChatManager::GMCommand_Announce(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)

{
    std::list<libcomp::String> argsCopy = args;
    int8_t color = 0;

    if ((!GetIntegerArg<int8_t>(color,argsCopy)) || argsCopy.size() < 1) 
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@announce requires two arguments, <color> <message>"));
    }
    
    libcomp::String message = libcomp::String::Join(argsCopy," ");
    auto server = mServer.lock();
    server->SendSystemMessage(client, message, color, true);
    return true;
}


bool ChatManager::GMCommand_Contract(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    uint32_t demonID;
    if(!GetIntegerArg<uint32_t>(demonID, argsCopy))
    {
        libcomp::String name;
        if(!GetStringArg(name, argsCopy))
        {
            return false;
        }

        auto devilData = definitionManager->GetDevilData(name);
        if(devilData == nullptr)
        {
            return false;
        }
        demonID = devilData->GetBasic()->GetID();
    }

    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();

    auto demon = characterManager->ContractDemon(character,
        definitionManager->GetDevilData(demonID));
    if(nullptr == demon)
    {
        return false;
    }

    state->SetObjectID(demon->GetUUID(),
        server->GetNextObjectID());

    int8_t slot = demon->GetBoxSlot();
    characterManager->SendDemonData(client, 0, slot,
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

bool ChatManager::GMCommand_Effect(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    uint32_t effectID;
    if(!GetIntegerArg<uint32_t>(effectID, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@effect requires an effect ID\n"));
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetStatusData(effectID);
    if(!def)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid effect ID supplied: %1\n").Arg(effectID));
    }

    // If the next arg starts with a '+', mark as an add instead of replace
    bool isAdd = false;
    if(argsCopy.size() > 0)
    {
        libcomp::String& next = argsCopy.front();
        if(!next.IsEmpty() && next.C()[0] == '+')
        {
            isAdd = true;
            next = next.Right(next.Length() - 1);
        }
    }

    uint8_t stack;
    if(!GetIntegerArg<uint8_t>(stack, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@effect requires a stack count\n"));
    }

    auto state = client->GetClientState();

    libcomp::String target;
    bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
    auto eState = isDemon
        ? std::dynamic_pointer_cast<ActiveEntityState>(state->GetDemonState())
        : std::dynamic_pointer_cast<ActiveEntityState>(state->GetCharacterState());

    AddStatusEffectMap m;
    m[effectID] = std::pair<uint8_t, bool>(stack, !isAdd);
    eState->AddStatusEffects(m, definitionManager);

    server->GetCharacterManager()->RecalculateStats(client, eState->GetEntityID());

    return true;
}

bool ChatManager::GMCommand_Enemy(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    // Valid params: enemy, enemy+AI, enemy+AI+x+y, enemy+AI+x+y+rot,
    // enemy+x+y, enemy+x+y+rot
    if(argsCopy.size() == 0 || argsCopy.size() > 5)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@enemy requires one to five args"));
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto zoneManager = server->GetZoneManager();

    uint32_t demonID;
    if(!GetIntegerArg<uint32_t>(demonID, argsCopy))
    {
        libcomp::String name;
        if(!GetStringArg(name, argsCopy))
        {
            return false;
        }

        auto devilData = definitionManager->GetDevilData(name);
        if(devilData == nullptr)
        {
            return false;
        }
        demonID = devilData->GetBasic()->GetID();
    }

    /// @todo: Default to directly in front of the player
    float x = cState->GetOriginX();
    float y = cState->GetOriginY();
    float rot = cState->GetOriginRotation();

    // All optional params past this point
    libcomp::String aiType = "default";
    if(argsCopy.size() > 0)
    {
        // Check for a number for X first
        bool xParam = GetDecimalArg<float>(x, argsCopy);
        if(!xParam)
        {
            // Assume a non-number is an AI script type
            GetStringArg(aiType, argsCopy);
            xParam = GetDecimalArg<float>(x, argsCopy);
        }

        // X/Y optional but Y must be set if X is
        if(xParam)
        {
            if(!GetDecimalArg<float>(y, argsCopy))
            {
                return false;
            }

            //Rotation is optional
            if(!GetDecimalArg<float>(rot, argsCopy))
            {
                rot = 0.f;
            }
        }
    }

    auto def = definitionManager->GetDevilData(demonID);
    auto zone = zoneManager->GetZoneInstance(client);

    if(nullptr == def)
    {
        return false;
    }

    return zoneManager->SpawnEnemy(zone, demonID, x, y, rot, aiType);
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

bool ChatManager::GMCommand_Familiarity(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    uint16_t familiarity;
    if(!GetIntegerArg<uint16_t>(familiarity, argsCopy))
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->UpdateFamiliarity(
        client, familiarity);

    return true;
}

bool ChatManager::GMCommand_Homepoint(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    (void)args;

    /// @todo: replace the menu packet with a scripted binding
    auto server = mServer.lock();
    server->GetEventManager()->HandleEvent(client, "event_homepoint", 0);

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
        if(!GetStringArg(name, argsCopy))
        {
            return false;
        }

        if(name.ToLower() == "macca")
        {
            itemID = ITEM_MACCA;
        }
        else if(name.ToLower() == "mag")
        {
            itemID = ITEM_MAGNETITE;
        }
        else
        {
            auto itemData = definitionManager->GetItemData(name);
            if(itemData == nullptr)
            {
                return false;
            }
            itemID = itemData->GetCommon()->GetID();
        }
    }

    uint16_t stackSize;
    if(!GetIntegerArg<uint16_t>(stackSize, argsCopy))
    {
        stackSize = 1;
    }

    return server->GetCharacterManager()
        ->AddRemoveItem(client, itemID, stackSize, true);
}

bool ChatManager::GMCommand_Kill(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto targetState = cState;

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto zoneManager = server->GetZoneManager();

    libcomp::String name;
    if(GetStringArg(name, argsCopy))
    {
        targetState = nullptr;
        for(auto zConnection : zoneManager->GetZoneConnections(client, true))
        {
            auto zCharState = zConnection->GetClientState()->
                GetCharacterState();
            if(zCharState->GetEntity()->GetName() == name)
            {
                targetState = zCharState;
                break;
            }
        }

        if(!targetState)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
                "Invalid character name supplied for the current zone: %1\n").Arg(name));
        }
    }

    if(targetState->SetHPMP(0, -1, false, true))
    {
        // Send a generic non-combat damage skill report to kill the target
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_REPORTS);
        reply.WriteS32Little(cState->GetEntityID());
        reply.WriteU32Little(10);   // Any valid skill ID
        reply.WriteS8(-1);          // No activation ID
        reply.WriteU32Little(1);    // Number of targets
        reply.WriteS32Little(targetState->GetEntityID());
        reply.WriteS32Little(9999); // Damage 1
        reply.WriteU8(0);           // Damage 1 type (generic)
        reply.WriteS32Little(0);    // Damage 2
        reply.WriteU8(2);           // Damage 2 type (none)
        reply.WriteU16Little(1);    // Lethal flag
        reply.WriteBlank(48);

        zoneManager->BroadcastPacket(client, reply);

        std::set<std::shared_ptr<ActiveEntityState>> entities;
        entities.insert(targetState);
        characterManager->UpdateWorldDisplayState(entities);
    }

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

bool ChatManager::GMCommand_Map(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    size_t mapIndex;
    uint8_t mapValue;
    if(!GetIntegerArg<size_t>(mapIndex, argsCopy) ||
        !GetIntegerArg<uint8_t>(mapValue, argsCopy))
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->UpdateMapFlags(client,
        mapIndex, mapValue);

    return true;
}

bool ChatManager::GMCommand_Position(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    std::list<libcomp::String> argsCopy = args;
    if(!args.empty() && args.size() == 2)
    {
        float destX, destY;
        if(!GetDecimalArg<float>(destX, argsCopy) ||
            !GetDecimalArg<float>(destY, argsCopy))
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                "Invalid args supplied for @pos command");
        }

        auto dState = state->GetDemonState();

        zoneManager->Warp(client, cState, destX, destY,
            cState->GetDestinationRotation());
        if(dState->GetEntity())
        {
            zoneManager->Warp(client, dState, destX, destY,
                dState->GetDestinationRotation());
        }
        
        return true;
    }
    else if(!args.empty() && args.size() != 2)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@pos requires zero or two args");
    }

    cState->RefreshCurrentPosition(server->GetServerTime());
    return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "Position: (%1, %2)").Arg(cState->GetCurrentX()).Arg(
        cState->GetCurrentY()));
}

bool ChatManager::GMCommand_Skill(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto state = client->GetClientState();
    auto definitionManager = server->GetDefinitionManager();

    uint32_t skillID;
    if(!GetIntegerArg<uint32_t>(skillID, argsCopy))
    {
        return false;
    }

    auto skill = definitionManager->GetSkillData(skillID);
    if(skill == nullptr)
    {
        return false;
    }

    libcomp::String target;
    bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
    auto entityID = isDemon ? state->GetDemonState()->GetEntityID()
        : state->GetCharacterState()->GetEntityID();

    return mServer.lock()->GetCharacterManager()->LearnSkill(
        client, entityID, skillID);
}

bool ChatManager::GMCommand_Speed(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto state = client->GetClientState();

    float scaling = 1.f;
    GetDecimalArg<float>(scaling, argsCopy);

    libcomp::String target;
    bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
    auto entityID = isDemon ? state->GetDemonState()->GetEntityID()
        : state->GetCharacterState()->GetEntityID();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_RUN_SPEED);
    p.WriteS32Little(entityID);
    p.WriteFloat(static_cast<float>(300.0f * scaling));

    client->SendPacket(p);

    return true;
}

bool ChatManager::GMCommand_TickerMessage(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)

{
    auto server = mServer.lock();
    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(server->GetConfig());
    std::list<libcomp::String> argsCopy = args;
    int8_t mode = 0;

    if(!(GetIntegerArg(mode,argsCopy)) || argsCopy.size() < 1)   
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Syntax invalid, try @tickermessage <mode> <message>"));
    }
    libcomp::String message = libcomp::String::Join(argsCopy," ");

    if (mode == 1) 
    {
        server->SendSystemMessage(client,message,0,true);
    }  

    conf->SetSystemMessage(message);
    return true;
}


bool ChatManager::GMCommand_Version(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    (void)args;

    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "%1 on branch %2").Arg(szGitCommittish).Arg(szGitBranch));
    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "Commit by %1 <%2> on %3").Arg(szGitAuthor).Arg(
        szGitAuthorEmail).Arg(szGitDate));
    SendChatMessage(client, ChatType_t::CHAT_SELF, szGitDescription);

    return true;
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
    float xCoord = 0.0f;
    float yCoord = 0.0f;
    float rotation = 0.0f;
    std::list<libcomp::String> argsCopy = args;

    if(args.empty())
    {
        auto zone = cState->GetZone();
        auto zoneData = zone->GetDefinition();
        auto zoneDef = server->GetDefinitionManager()->GetZoneData(
            zoneData->GetID());

        if(zoneDef)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("You are in zone %1 (%2)").Arg(
                    zoneData->GetID()).Arg(zoneDef->GetBasic()->GetName()));
        }
        else
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("You are in zone %1").Arg(zoneData->GetID()));
        }
    }
    else if(1 == args.size() || 3 == args.size())
    {
        // Parse the zone ID.
        bool parseOk = GetIntegerArg<uint32_t>(zoneID, argsCopy);

        std::shared_ptr<objects::ServerZone> zoneData;

        // If the zone ID argument is right, look for the zone.
        if(parseOk)
        {
            zoneData = server->GetServerDataManager()->GetZoneData(zoneID);
        }

        // If the ID did not parse or the zone does not exist, stop here.
        if(!parseOk || !zoneData)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF, "ERROR: "
                "INVALID ZONE ID.  Please enter a proper zoneID and "
                "try again.");
        }

        if(1 == args.size())
        {
            // Load the defaults.
            xCoord = zoneData->GetStartingX();
            yCoord = zoneData->GetStartingY();
            rotation = zoneData->GetStartingRotation();
        }
        else if(!GetDecimalArg<float>(xCoord, argsCopy) ||
            !GetDecimalArg<float>(yCoord, argsCopy))
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF, "ERROR: "
                "One of the inputs is not a number.  Please re-enter the "
                "command with proper inputs.");
        }

        zoneManager->LeaveZone(client, false);
        zoneManager->EnterZone(client, zoneID, xCoord, yCoord, rotation);
        
        return true;
    }
    else
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "USAGE: @zone [ID [X Y]]");
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
