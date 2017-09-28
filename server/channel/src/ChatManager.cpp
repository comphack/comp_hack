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
#include <ServerConstants.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <Character.h>
#include <MiItemData.h>
#include <MiDevilData.h>
#include <MiNPCBasicData.h>
#include <MiQuestData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <PostItem.h>
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
    mGMands["ban"] = &ChatManager::GMCommand_Ban;
    mGMands["contract"] = &ChatManager::GMCommand_Contract;
    mGMands["crash"] = &ChatManager::GMCommand_Crash;
    mGMands["effect"] = &ChatManager::GMCommand_Effect;
    mGMands["enemy"] = &ChatManager::GMCommand_Enemy;
    mGMands["expertiseup"] = &ChatManager::GMCommand_ExpertiseUpdate;
    mGMands["familiarity"] = &ChatManager::GMCommand_Familiarity;
    mGMands["homepoint"] = &ChatManager::GMCommand_Homepoint;
    mGMands["item"] = &ChatManager::GMCommand_Item;
    mGMands["kick"] = &ChatManager::GMCommand_Kick;
    mGMands["kill"] = &ChatManager::GMCommand_Kill;
    mGMands["levelup"] = &ChatManager::GMCommand_LevelUp;
    mGMands["lnc"] = &ChatManager::GMCommand_LNC;
    mGMands["map"] = &ChatManager::GMCommand_Map;
    mGMands["pos"] = &ChatManager::GMCommand_Position;
    mGMands["post"] = &ChatManager::GMCommand_Post;
    mGMands["quest"] = &ChatManager::GMCommand_Quest;
    mGMands["skill"] = &ChatManager::GMCommand_Skill;
    mGMands["spawn"] = &ChatManager::GMCommand_Spawn;
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
    const libcomp::String& message)
{
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    if(message.IsEmpty())
    {
        return false;
    }

    auto state = client->GetClientState();
    auto character = state->GetCharacterState()->GetEntity();
    libcomp::String sentFrom = character->GetName();

    ChatVis_t visibility = ChatVis_t::CHAT_VIS_SELF;

    uint32_t relayID = 0;
    PacketRelayMode_t relayMode = PacketRelayMode_t::RELAY_FAILURE;
    switch(chatChannel)
    {
        case ChatType_t::CHAT_PARTY:
            visibility = ChatVis_t::CHAT_VIS_PARTY;
            LOG_INFO(libcomp::String("[Party]:  %1: %2\n.").Arg(sentFrom)
                .Arg(message));
            if(state->GetPartyID())
            {
                relayID = state->GetPartyID();
                relayMode = PacketRelayMode_t::RELAY_PARTY;
            }
            else
            {
                LOG_ERROR(libcomp::String("Party chat attempted by"
                    " character not in a party: %1\n.").Arg(sentFrom));
                return false;
            }
            break;
        case ChatType_t::CHAT_CLAN:
            visibility = ChatVis_t::CHAT_VIS_CLAN;
            LOG_INFO(libcomp::String("[Clan]:  %1: %2\n.").Arg(sentFrom)
                .Arg(message));
            if(state->GetClanID())
            {
                relayID = (uint32_t)state->GetClanID();
                relayMode = PacketRelayMode_t::RELAY_CLAN;
            }
            else
            {
                LOG_ERROR(libcomp::String("Clan chat attempted by"
                    " character not in a clan: %1\n.").Arg(sentFrom));
                return false;
            }
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

    libcomp::Packet p;
    if(relayMode != PacketRelayMode_t::RELAY_FAILURE)
    {
        // Route the message through the world via packet relay
        p.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
        p.WriteS32Little(state->GetWorldCID());
        p.WriteU8((uint8_t)relayMode);
        p.WriteU32Little(relayID);
        p.WriteU8(1);   // Include self
    }

    if(chatChannel == ChatType_t::CHAT_CLAN)
    {
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_CHAT);
        p.WriteS32Little(state->GetClanID());
        p.WriteString16Little(state->GetClientStringEncoding(),
            sentFrom, true);
        p.WriteString16Little(state->GetClientStringEncoding(),
            message, true);
    }
    else if(chatChannel == ChatType_t::CHAT_TEAM)
    {
        /// @todo: team chat
    }
    else
    {
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
        p.WriteU16Little((uint16_t)chatChannel);
        p.WriteString16Little(state->GetClientStringEncoding(),
            sentFrom, true);
        p.WriteString16Little(state->GetClientStringEncoding(),
            message, true);
    }

    switch(visibility)
    {
        case ChatVis_t::CHAT_VIS_SELF:
            client->SendPacket(p);
            break;
        case ChatVis_t::CHAT_VIS_ZONE:
            zoneManager->BroadcastPacket(client, p, true);
            break;
        case ChatVis_t::CHAT_VIS_RANGE:
            zoneManager->SendToRange(client, p, true);
            break;
        case ChatVis_t::CHAT_VIS_PARTY:
        case ChatVis_t::CHAT_VIS_CLAN:
        case ChatVis_t::CHAT_VIS_TEAM:
            mServer.lock()->GetManagerConnection()->GetWorldConnection()
                ->SendPacket(p);
            break;
        default:
            return false;
    }

    return true;
}

bool ChatManager::SendTellMessage(const std::shared_ptr<
    ChannelClientConnection>& client, const libcomp::String& message,
    const libcomp::String& targetName)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    // Relay a packet by target name
    libcomp::Packet relay;
    relay.WritePacketCode(InternalPacketCode_t::PACKET_RELAY);
    relay.WriteS32Little(state->GetWorldCID());
    relay.WriteU8((uint8_t)PacketRelayMode_t::RELAY_CHARACTER);
    relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
        targetName, true);

    // Write the normal packet to relay
    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHAT);
    relay.WriteU16Little((uint16_t)ChatType_t::CHAT_TELL);

    // Clients should be using the same encoding
    relay.WriteString16Little(state->GetClientStringEncoding(),
        character->GetName(), true);
    relay.WriteString16Little(state->GetClientStringEncoding(),
        message, true);

    mServer.lock()->GetManagerConnection()->GetWorldConnection()
        ->SendPacket(relay);

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

    if(!GetIntegerArg<int8_t>(color, argsCopy) || argsCopy.size() < 1)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@announce requires two arguments, <color> <message>"));
    }

    libcomp::String message = libcomp::String::Join(argsCopy, " ");
    auto server = mServer.lock();
    server->SendSystemMessage(client, message, color, true);

    return true;
}

bool ChatManager::GMCommand_Ban(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;
    libcomp::String bannedPlayer;

    if (!GetStringArg(bannedPlayer, argsCopy) || argsCopy.size() > 1)
    {
         return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@ban requires one argument, <username>"));
    }
    
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();
    auto lobbyDB = server->GetLobbyDatabase();
    auto target = objects::Character::LoadCharacterByName(worldDB, bannedPlayer);
    auto targetAccount = target ? target->GetAccount().Get() : nullptr;
    auto targetClient = targetAccount ? server->GetManagerConnection()->GetClientConnection(
        targetAccount->GetUsername()) : nullptr;

    if(targetClient != nullptr) 
    {
        targetAccount->SetIsBanned(true);
        targetAccount->Update(lobbyDB);
        targetClient->Close();

        return true;
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
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Invalid demon name: %1").Arg(name));
        }
        demonID = devilData->GetBasic()->GetID();
    }

    auto devilData = definitionManager->GetDevilData(demonID);
    if(devilData == nullptr)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("Invalid demon ID: %1").Arg(demonID));
    }

    auto demon = characterManager->ContractDemon(client, devilData, 0);
    return demon != nullptr || SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Demon could not be contracted");
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
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@effect requires an effect ID");
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetStatusData(effectID);

    if(!def)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid effect ID supplied: %1").Arg(effectID));
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
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@effect requires a stack count");
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
    if(argsCopy.empty() || argsCopy.size() > 5)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@enemy requires one to five args");
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
    cState->RefreshCurrentPosition(ChannelServer::GetServerTime());

    float x = cState->GetCurrentX();
    float y = cState->GetCurrentY();
    float rot = cState->GetCurrentRotation();

    // All optional params past this point
    libcomp::String aiType = "";

    if(!argsCopy.empty())
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
        else
        {
            x = cState->GetCurrentX();
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

    float multiplier = 1.0f;
    GetDecimalArg<float>(multiplier, argsCopy);
    if(multiplier <= 0.f)
    {
        // Don't bother with an error, just reset
        multiplier = 1.0f;
    }

    server->GetCharacterManager()->UpdateExpertise(client, skillID,
        multiplier);

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

    auto server = mServer.lock();
    server->GetEventManager()->HandleEvent(client,
        SVR_CONST.EVENT_MENU_HOMEPOINT, 0);

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
            itemID = SVR_CONST.ITEM_MACCA;
        }
        else if(name.ToLower() == "mag")
        {
            itemID = SVR_CONST.ITEM_MAGNETITE;
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

    uint32_t stackSize;

    if(!GetIntegerArg<uint32_t>(stackSize, argsCopy))
    {
        stackSize = 1;
    }

    std::unordered_map<uint32_t, uint32_t> itemMap;
    itemMap[itemID] = stackSize;

    return server->GetCharacterManager()
        ->AddRemoveItems(client, itemMap, true);
}

bool ChatManager::GMCommand_Kick(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;
    libcomp::String kickedPlayer;

    if(!GetStringArg(kickedPlayer, argsCopy) || argsCopy.size() > 1)
    {
         return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@kick requires one argument, <username>");
    }

    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();
    auto target = objects::Character::LoadCharacterByName(worldDB, kickedPlayer);
    auto targetAccount = target ? target->GetAccount().Get() : nullptr;
    auto targetClient = targetAccount ? server->GetManagerConnection()->GetClientConnection(
        targetAccount->GetUsername()) : nullptr;

    if(targetClient != nullptr) 
    {
        targetClient->Close();

        return true;
    }

    return false;
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
                "Invalid character name supplied for the current zone: %1").Arg(name));
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

    uint16_t mapID;
    if(!GetIntegerArg<uint16_t>(mapID, argsCopy))
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->AddMap(client,
        mapID);

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

bool ChatManager::GMCommand_Post(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto targetAccount = client->GetClientState()->GetAccountUID();
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();
    auto definitionManager = server->GetDefinitionManager();
    
    uint32_t itemID;

    if(!GetIntegerArg<uint32_t>(itemID, argsCopy) ||
        !definitionManager->GetItemData(itemID))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid item ID specified: %1\n").Arg(itemID));
    }

    libcomp::String name;

    if(GetStringArg(name, argsCopy))
    {
        auto target = objects::Character::LoadCharacterByName(worldDB, name);
        targetAccount = target ? target->GetAccount().GetUUID() : NULLUUID;
    }
    
    if(targetAccount.IsNull())
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid post target character specified: %1\n").Arg(name));
    }

    auto worldData = objects::AccountWorldData::LoadAccountWorldDataByAccount(
        worldDB, targetAccount);

    if(!worldData)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "No world data exists for account: %1\n").Arg(targetAccount.ToString()));
    }

    if(worldData->PostCount() >= MAX_POST_ITEM_COUNT)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "There is no more room in the Post!");
    }

    auto postItem = libcomp::PersistentObject::New<objects::PostItem>(true);
    postItem->SetType(itemID);
    postItem->SetTimestamp((uint32_t)std::time(0));
    postItem->SetAccount(targetAccount);

    worldData->AppendPost(postItem);

    postItem->Insert(worldDB);
    worldData->Update(worldDB);

    return true;
}

bool ChatManager::GMCommand_Quest(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    
    uint32_t questID;
    bool validQuestID = GetIntegerArg<uint32_t>(questID, argsCopy);
    auto questData = validQuestID
        ? definitionManager->GetQuestData(questID) : nullptr;
    if(!questData)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid quest ID specified: %1").Arg(questID));
    }

    int8_t phase;
    if(!GetIntegerArg<int8_t>(phase, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "No phase specified for @quest command");
    }
    else if(phase < -2 || (int8_t)questData->GetPhaseCount() < phase)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid phase '%1' supplied for quest: %2").Arg(phase).Arg(questID));
    }

    server->GetEventManager()->UpdateQuest(client, (int16_t)questID, phase, true);

    return true;
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

bool ChatManager::GMCommand_Spawn(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    (void)args;

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    auto zone = zoneManager->GetZoneInstance(client);

    zoneManager->UpdateSpawnGroups(zone, true);
    return true;
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
    auto entity = isDemon
        ? std::dynamic_pointer_cast<ActiveEntityState>(state->GetDemonState())
        : std::dynamic_pointer_cast<ActiveEntityState>(state->GetCharacterState());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_RUN_SPEED);
    p.WriteS32Little(entity->GetEntityID());
    p.WriteFloat(static_cast<float>(entity->GetMovementSpeed() * scaling));

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

    if(!GetIntegerArg(mode, argsCopy) || argsCopy.size() < 1)   
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Syntax invalid, try @tickermessage <mode> <message>"));
    }

    libcomp::String message = libcomp::String::Join(argsCopy, " ");

    if (mode == 1) 
    {
        server->SendSystemMessage(client, message, 0, true);
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
    uint32_t dynamicMapID = 0;
    float xCoord = 0.f;
    float yCoord = 0.f;
    float rotation = 0.f;
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
    else if(4 >= args.size())
    {
        bool getDynamicMapID = args.size() == 2 || args.size() == 4;

        // Parse the zone ID and dynamic map ID.
        bool parseOk = GetIntegerArg<uint32_t>(zoneID, argsCopy);
        if(parseOk && getDynamicMapID)
        {
            parseOk = GetIntegerArg<uint32_t>(dynamicMapID, argsCopy);
        }

        std::shared_ptr<objects::ServerZone> zoneData;

        // If the zone ID argument is right, look for the zone.
        if(parseOk)
        {
            zoneData = server->GetServerDataManager()->GetZoneData(zoneID,
                dynamicMapID);
        }

        // If the ID did not parse or the zone does not exist, stop here.
        if(!parseOk || !zoneData)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF, "ERROR: "
                "INVALID ZONE ID.  Please enter a proper zoneID and "
                "try again.");
        }

        if(argsCopy.size() < 2)
        {
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

        zoneManager->EnterZone(client, zoneID, dynamicMapID,
            xCoord, yCoord, rotation);
        
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
