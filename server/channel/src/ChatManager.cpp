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

#include "ChatManager.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <Git.h>
#include <Log.h>
#include <PacketCodes.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <ActivatedAbility.h>
#include <ChannelConfig.h>
#include <Character.h>
#include <CharacterProgress.h>
#include <Event.h>
#include <EventInstance.h>
#include <EventState.h>
#include <Expertise.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiDevilData.h>
#include <MiDevilLVUpRateData.h>
#include <MiNPCBasicData.h>
#include <MiQuestData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpotData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <PostItem.h>
#include <ReportedPlayer.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>

// Standard C Includes
#include <cstdlib>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "ClientState.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

ChatManager::ChatManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mGMands["addcp"] = &ChatManager::GMCommand_AddCP;
    mGMands["announce"] = &ChatManager::GMCommand_Announce;
    mGMands["ban"] = &ChatManager::GMCommand_Ban;
    mGMands["coin"] = &ChatManager::GMCommand_Coin;
    mGMands["contract"] = &ChatManager::GMCommand_Contract;
    mGMands["crash"] = &ChatManager::GMCommand_Crash;
    mGMands["dxp"] = &ChatManager::GMCommand_DigitalizePoints;
    mGMands["effect"] = &ChatManager::GMCommand_Effect;
    mGMands["enchant"] = &ChatManager::GMCommand_Enchant;
    mGMands["enemy"] = &ChatManager::GMCommand_Enemy;
    mGMands["event"] = &ChatManager::GMCommand_Event;
    mGMands["expertise"] = &ChatManager::GMCommand_ExpertiseSet;
    mGMands["expertisemax"] = &ChatManager::GMCommand_ExpertiseExtend;
    mGMands["familiarity"] = &ChatManager::GMCommand_Familiarity;
    mGMands["flag"] = &ChatManager::GMCommand_Flag;
    mGMands["fgauge"] = &ChatManager::GMCommand_FusionGauge;
    mGMands["goto"] = &ChatManager::GMCommand_Goto;
    mGMands["help"] = &ChatManager::GMCommand_Help;
    mGMands["homepoint"] = &ChatManager::GMCommand_Homepoint;
    mGMands["instance"] = &ChatManager::GMCommand_Instance;
    mGMands["item"] = &ChatManager::GMCommand_Item;
    mGMands["kick"] = &ChatManager::GMCommand_Kick;
    mGMands["kill"] = &ChatManager::GMCommand_Kill;
    mGMands["levelup"] = &ChatManager::GMCommand_LevelUp;
    mGMands["license"] = &ChatManager::GMCommand_License;
    mGMands["lnc"] = &ChatManager::GMCommand_LNC;
    mGMands["map"] = &ChatManager::GMCommand_Map;
    mGMands["plugin"] = &ChatManager::GMCommand_Plugin;
    mGMands["pos"] = &ChatManager::GMCommand_Position;
    mGMands["post"] = &ChatManager::GMCommand_Post;
    mGMands["quest"] = &ChatManager::GMCommand_Quest;
    mGMands["reported"] = &ChatManager::GMCommand_Reported;
    mGMands["resolve"] = &ChatManager::GMCommand_Resolve;
    mGMands["reunion"] = &ChatManager::GMCommand_Reunion;
    mGMands["scrap"] = &ChatManager::GMCommand_Scrap;
    mGMands["skill"] = &ChatManager::GMCommand_Skill;
    mGMands["skillpoint"] = &ChatManager::GMCommand_SkillPoint;
    mGMands["slotadd"] = &ChatManager::GMCommand_SlotAdd;
    mGMands["sp"] = &ChatManager::GMCommand_SoulPoints;
    mGMands["spawn"] = &ChatManager::GMCommand_Spawn;
    mGMands["speed"] = &ChatManager::GMCommand_Speed;
    mGMands["spirit"] = &ChatManager::GMCommand_Spirit;
    mGMands["tickermessage"] = &ChatManager::GMCommand_TickerMessage;
    mGMands["title"] = &ChatManager::GMCommand_Title;
    mGMands["tokusei"] = &ChatManager::GMCommand_Tokusei;
    mGMands["valuable"] = &ChatManager::GMCommand_Valuable;
    mGMands["version"] = &ChatManager::GMCommand_Version;
    mGMands["worldtime"] = &ChatManager::GMCommand_WorldTime;
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
            LOG_INFO(libcomp::String("[Party]:  %1: %2\n").Arg(sentFrom)
                .Arg(message));
            if(state->GetPartyID())
            {
                relayID = state->GetPartyID();
                relayMode = PacketRelayMode_t::RELAY_PARTY;
            }
            else
            {
                LOG_ERROR(libcomp::String("Party chat attempted by"
                    " character not in a party: %1\n").Arg(sentFrom));
                return false;
            }
            break;
        case ChatType_t::CHAT_CLAN:
            visibility = ChatVis_t::CHAT_VIS_CLAN;
            LOG_INFO(libcomp::String("[Clan]:  %1: %2\n").Arg(sentFrom)
                .Arg(message));
            if(state->GetClanID())
            {
                relayID = (uint32_t)state->GetClanID();
                relayMode = PacketRelayMode_t::RELAY_CLAN;
            }
            else
            {
                LOG_ERROR(libcomp::String("Clan chat attempted by"
                    " character not in a clan: %1\n").Arg(sentFrom));
                return false;
            }
            break;
        case ChatType_t::CHAT_SHOUT:
            visibility = ChatVis_t::CHAT_VIS_ZONE;
            LOG_INFO(libcomp::String("[Shout]:  %1: %2\n").Arg(sentFrom)
                .Arg(message));
            break;
        case ChatType_t::CHAT_SAY:
            visibility = ChatVis_t::CHAT_VIS_RANGE;
            LOG_INFO(libcomp::String("[Say]:  %1: %2\n").Arg(sentFrom)
                .Arg(message));
            break;
        case ChatType_t::CHAT_SELF:
            visibility = ChatVis_t::CHAT_VIS_SELF;
            LOG_INFO(libcomp::String("[Self]:  %1: %2\n").Arg(sentFrom)
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

bool ChatManager::HandleGMand(const std::shared_ptr<
    ChannelClientConnection>& client, const libcomp::String& message)
{
    auto state = client->GetClientState();

    std::smatch match;
    std::string input = message.C();
    std::regex toFind("@([^\\s]+)(.*)");
    if(std::regex_match(input, match, toFind))
    {
        if(state->GetUserLevel() == 0 && "@version" != message &&
            "@license" != message)
        {
            // Don't process the message but don't fail
            LOG_DEBUG(libcomp::String("Non-GM account attempted to execute a GM"
                " command: %1\n").Arg(state->GetAccountUID().ToString()));
            return true;
        }

        libcomp::String sentFrom = state->GetCharacterState()->GetEntity()
            ->GetName();
        LOG_INFO(libcomp::String("[GM] %1: %2\n").Arg(sentFrom).Arg(message));

        libcomp::String command(match[1]);
        libcomp::String args(match.max_size() > 2 ? match[2].str() : "");

        command = command.ToLower();

        std::list<libcomp::String> argsList;
        if(!args.IsEmpty())
        {
            argsList = args.Split(" ");
        }
        argsList.remove_if([](const libcomp::String& value) { return value.IsEmpty(); });

        mServer.lock()->QueueWork([](ChatManager* pChatManager,
            const std::shared_ptr<ChannelClientConnection>& cmdClient,
            const libcomp::String& cmd,
            const std::list<libcomp::String>& cmdArgs)
        {
            if(!pChatManager->ExecuteGMCommand(cmdClient, cmd, cmdArgs))
            {
                LOG_WARNING(libcomp::String("GM command could not be"
                    " processed: %1\n").Arg(cmd));
            }
        }, this, client, command, argsList);

        return true;
    }

    return false;
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

bool ChatManager::GMCommand_AddCP(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 950))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    auto targetClient = client;
    auto targetAccount = client->GetClientState()->GetAccountLogin()
        ->GetAccount().Get();
    auto server = mServer.lock();

    int64_t amount = 0;
    if(!GetIntegerArg<int64_t>(amount, argsCopy) || amount <= 0)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@addcp requires a positive amount be specified"));
    }

    libcomp::String name;
    if(GetStringArg(name, argsCopy))
    {
        std::shared_ptr<objects::Character> targetCharacter;
        if(!GetTargetCharacterAccount(name, false, targetCharacter,
            targetAccount, targetClient))
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
                "Invalid character name supplied for add CP command: %1").Arg(name));
        }
    }

    auto accountManager = server->GetAccountManager();
    if(accountManager->IncreaseCP(targetAccount, amount))
    {
        if(targetClient)
        {
            if(targetClient != client)
            {
                // Notify the other player
                auto character = client->GetClientState()->GetCharacterState()
                    ->GetEntity();
                SendChatMessage(targetClient, ChatType_t::CHAT_SELF,
                    libcomp::String("You have received %1 CP from %2!\n")
                    .Arg(amount).Arg(character ? character->GetName() : "a GM"));
            }
            else
            {
                SendChatMessage(targetClient, ChatType_t::CHAT_SELF,
                    libcomp::String("Your available CP has increased by %1!\n")
                    .Arg(amount));
            }

            accountManager->SendCPBalance(targetClient);
        }
    }
    else
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("Failed to add CP to target character"
                " account: %1\n").Arg(name));
    }

    return true;
}

bool ChatManager::GMCommand_Announce(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 100))
    {
        return true;
    }

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
    if(!HaveUserLevel(client, 400))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;
    libcomp::String bannedPlayer;

    if(!GetStringArg(bannedPlayer, argsCopy) || argsCopy.size() > 1)
    {
         return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "@ban requires one argument, <username>"));
    }

    std::shared_ptr<objects::Character> target;
    std::shared_ptr<objects::Account> targetAccount;
    std::shared_ptr<channel::ChannelClientConnection> targetClient;
    GetTargetCharacterAccount(bannedPlayer, false, target, targetAccount,
        targetClient);

    if(targetAccount &&
        client->GetClientState()->GetUserLevel() < targetAccount->GetUserLevel())
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Your user level is lower than the user you tried to ban.");
    }

    if(targetClient != nullptr)
    {
        targetAccount->SetEnabled(false);
        targetAccount->Update(mServer.lock()->GetLobbyDatabase());
        targetClient->Close();

        return true;
    }

    return false;
}

bool ChatManager::GMCommand_Coin(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    auto state = client->GetClientState();
    auto eState = state->GetEventState();
    if(eState && eState->GetGameSession())
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Coins cannot be added while web-game session is active");
    }

    std::list<libcomp::String> argsCopy = args;

    int64_t amount;

    if(!GetIntegerArg(amount, argsCopy))
    {
        return false;
    }

    if(!mServer.lock()->GetCharacterManager()->UpdateCoinTotal(client,
        amount, false))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Coin total could not be updated");
    }

    return true;
}

bool ChatManager::GMCommand_Contract(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

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

    auto demon = characterManager->ContractDemon(client, devilData, 0,
        MAX_FAMILIARITY);
    return demon != nullptr || SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Demon could not be contracted");
}

bool ChatManager::GMCommand_Crash(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 950))
    {
        return true;
    }

    (void)client;
    (void)args;

    abort();
}

bool ChatManager::GMCommand_DigitalizePoints(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint8_t raceID;
    int32_t points;
    if(!GetIntegerArg(raceID, argsCopy) || !GetIntegerArg(points, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@dxp requires a demon race ID and digitalize XP");
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    if(!definitionManager->GetGuardianLevelData(raceID))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Invalid digitalize demon race ID");
    }
    
    if(points < 0)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Invalid digitalize XP supplied");
    }

    std::unordered_map<uint8_t, int32_t> pointMap;
    pointMap[raceID] = points;

    server->GetCharacterManager()->UpdateDigitalizePoints(client, pointMap,
        false, false);

    return true;
}

bool ChatManager::GMCommand_Effect(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

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

    StatusEffectChanges effects;
    effects[effectID] = StatusEffectChange(effectID, stack, !isAdd);
    eState->AddStatusEffects(effects, definitionManager);

    server->GetCharacterManager()->RecalculateTokuseiAndStats(eState, client);

    return true;
}

bool ChatManager::GMCommand_Enchant(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 650))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    // All but BULLET types are (technically) valid
    const std::set<int8_t> validTypes = {
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_HEAD,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FACE,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NECK,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_ARMS,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FEET,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_COMP,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EARRING,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EXTRA,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BACK,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TALISMAN,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON,
    };

    int8_t equipType = GetEquipTypeArg(client, argsCopy, "enchant", validTypes);
    if(equipType == -1)
    {
        return true;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto item = character
        ? character->GetEquippedItems((size_t)equipType).Get() : nullptr;

    if(!item)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "The specified item type is not equipped");
    }

    int16_t tarot = 0;
    int16_t soul = 0;

    if(!GetIntegerArg(tarot, argsCopy) ||
        !GetIntegerArg(soul, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Missing tarot and soul effects");
    }

    // Default to current effect if not specified
    if(tarot == -1)
    {
        tarot = item->GetTarot();
    }

    if(soul == -1)
    {
        soul = item->GetSoul();
    }

    // No restrictions enforced here except validity checks
    if(!definitionManager->GetEnchantData(tarot) ||
        !definitionManager->GetEnchantData(soul))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Invalid tarot or soul effect specified");
    }

    item->SetTarot(tarot);
    item->SetSoul(soul);

    auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
        libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
    if(itemBox)
    {
        server->GetCharacterManager()->SendItemBoxData(client, itemBox,
            { (uint16_t)item->GetBoxSlot() });
    }

    cState->RecalcEquipState(definitionManager);

    server->GetCharacterManager()->RecalculateTokuseiAndStats(cState, client);

    server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());

    return true;
}

bool ChatManager::GMCommand_Enemy(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 400))
    {
        return true;
    }

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
    auto zone = zoneManager->GetCurrentZone(client);

    if(nullptr == def)
    {
        return false;
    }

    return zoneManager->SpawnEnemy(zone, demonID, x, y, rot, aiType);
}

bool ChatManager::GMCommand_Event(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 950))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    libcomp::String eventID;

    if(argsCopy.size() > 0 && !GetStringArg(eventID, argsCopy))
    {
        return false;
    }

    if(eventID.IsEmpty())
    {
        auto state = client->GetClientState();
        auto eState = state->GetEventState();
        auto current = eState->GetCurrent();
        auto event = current ? current->GetEvent() : nullptr;

        if(event)
        {
            eventID = event->GetID();
        }

        if(eventID.IsEmpty())
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("No event is currently active"));
        }
        else
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("The active event is: %1").Arg(eventID));
        }
    }
    else
    {
        auto server = mServer.lock();
        auto serverDataManager = server->GetServerDataManager();
        auto event = serverDataManager->GetEventData(eventID);

        if(event)
        {
            auto state = client->GetClientState();
            auto cState = state->GetCharacterState();

            if(server->GetEventManager()->HandleEvent(client, eventID,
                cState->GetEntityID()))
            {
                return SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Event started: %1").Arg(eventID));
            }
            else
            {
                return SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Event could not be started: %1")
                    .Arg(eventID));
            }
        }
        else
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Event does not exist: %1").Arg(eventID));
        }
    }
}

bool ChatManager::GMCommand_ExpertiseExtend(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint8_t val = 1;
    if(argsCopy.size() != 0 && !GetIntegerArg<uint8_t>(val, argsCopy))
    {
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(character)
    {
        int8_t newVal = (int8_t)((character->GetExpertiseExtension() + val) > 127
            ? 127 : (character->GetExpertiseExtension() + val));

        if(newVal != character->GetExpertiseExtension())
        {
            auto server = mServer.lock();
            auto characterManager = server->GetCharacterManager();

            character->SetExpertiseExtension(newVal);

            characterManager->SendExpertiseExtension(client);
            server->GetWorldDatabase()->QueueUpdate(character,
                state->GetAccountUID());
        }

        return true;
    }

    return false;
}

bool ChatManager::GMCommand_ExpertiseSet(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();

    uint8_t expertiseID;
    if(!GetIntegerArg(expertiseID, argsCopy))
    {
        return false;
    }

    uint8_t rank;
    if(!GetIntegerArg(rank, argsCopy))
    {
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto exp = character->GetExpertises((size_t)expertiseID).Get();

    int32_t adjust = (rank * 10000);
    if(exp)
    {
        adjust = adjust - exp->GetPoints();
    }

    std::list<std::pair<uint8_t, int32_t>> pointMap;
    pointMap.push_back(std::make_pair(expertiseID, adjust));

    server->GetCharacterManager()->UpdateExpertisePoints(
        client, pointMap, true);

    return true;
}

bool ChatManager::GMCommand_Familiarity(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

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

bool ChatManager::GMCommand_Flag(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 950))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    libcomp::String source;

    if(argsCopy.size() > 0 && !GetStringArg(source, argsCopy))
    {
        return false;
    }

    source = source.ToLower();

    bool isInstance = "inst" == source;

    if("zone" != source && !isInstance)
    {
        return false;
    }

    auto cState = client->GetClientState()->GetCharacterState();

    if(!cState)
    {
        return false;
    }

    auto zone = cState->GetZone();

    if(!zone)
    {
        return false;
    }

    auto instance = zone->GetInstance();

    if(isInstance && !instance)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("Zone is not an instance!"));
    }

    if(argsCopy.empty())
    {
        // List flags.
        auto flags = isInstance ? instance->GetFlagStates() :
            zone->GetFlagStates();

        SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("CID, Key, Value"));

        for(auto it : flags)
        {
            auto cid = it.first;

            for(auto it2 : it.second)
            {
                auto key = it2.first;
                auto val = it2.second;

                SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("%1, %2, %3").Arg(cid).Arg(key).Arg(val));
            }
        }
    }
    else
    {
        int32_t cid, key;

        if(argsCopy.empty() || !GetIntegerArg<int32_t>(cid, argsCopy))
        {
            return false;
        }

        if(argsCopy.empty() || !GetIntegerArg<int32_t>(key, argsCopy))
        {
            return false;
        }

        bool get = argsCopy.empty();

        // Get or set flag.
        if(get)
        {
            int32_t value = isInstance ?
                instance->GetFlagStateValue(key, 0, cid) :
                zone->GetFlagStateValue(key, 0, cid);

            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Value: %1").Arg(value));
        }
        else // set
        {
            int32_t value;

            if(argsCopy.empty() || !GetIntegerArg<int32_t>(value, argsCopy))
            {
                return false;
            }

            if(isInstance)
            {
                instance->SetFlagState(key, value, cid);
            }
            else
            {
                zone->SetFlagState(key, value, cid);
            }

            return true;
        }
    }

    return true;
}

bool ChatManager::GMCommand_FusionGauge(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint32_t points;

    if(!GetIntegerArg(points, argsCopy))
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->UpdateFusionGauge(
        client, (int32_t)points, false);

    return true;
}

bool ChatManager::GMCommand_Goto(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 400))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    bool toSelf = false;
    libcomp::String name;
    if(argsCopy.size() > 0 && !GetStringArg(name, argsCopy))
    {
        return false;
    }

    if(name == "self")
    {
        toSelf = true;
        if(!GetStringArg(name, argsCopy))
        {
            return false;
        }
    }

    std::shared_ptr<ChannelClientConnection> oClient;
    if(name.IsEmpty())
    {
        oClient = client;
    }
    else
    {
        std::shared_ptr<objects::Character> oTarget;
        std::shared_ptr<objects::Account> oTargetAccount;
        GetTargetCharacterAccount(name, true, oTarget, oTargetAccount,
            oClient);
    }

    if(oClient)
    {
        auto zoneManager = mServer.lock()->GetZoneManager();

        auto cState = client->GetClientState()->GetCharacterState();
        auto oTargetState = oClient->GetClientState();
        auto oTargetCState = oTargetState->GetCharacterState();
        if(oTargetCState && cState->GetZone() == oTargetCState->GetZone())
        {
            auto fromCState = toSelf ? oTargetCState : cState;
            auto fromDState = toSelf ? oTargetState->GetDemonState()
                : client->GetClientState()->GetDemonState();
            auto toCState = toSelf
                ? cState : oTargetCState;

            float destX = toCState->GetCurrentX();
            float destY = toCState->GetCurrentY();

            zoneManager->Warp(client, fromCState, destX, destY,
                toCState->GetDestinationRotation());
            if(fromDState->GetEntity())
            {
                zoneManager->Warp(client, fromDState, destX, destY,
                    toCState->GetDestinationRotation());
            }

            return true;
        }
        else
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Player '%1' does not exist in the"
                " same zone").Arg(name));
        }
    }
    else
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("Character '%1' is not currently logged"
                " into this channel").Arg(name));
    }
}

bool ChatManager::GMCommand_Help(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    static std::map<std::string, std::vector<std::string>> usage = {
        { "addcp", {
            "@addcp AMOUNT [NAME]",
            "Gives AMOUNT of CP to the character NAME or to yourself",
            "if no NAME is specified.",
        } },
        { "announce", {
            "@announce COLOR MESSAGE...",
            "Announce a ticker MESSAGE with the specified COLOR.",
        } },
        { "ban", {
            "@ban NAME",
            "Bans the account which owns the character NAME.",
        } },
        { "coin", {
            "@coin AMOUNT",
            "Set the current character's casino coin AMOUNT."
        } },
        { "contract", {
            "@contract ID|NAME",
            "Adds demon given by it's ID or NAME to your COMP.",
        } },
        { "crash", {
            "@crash",
            "Causes the server to crash for testing the database.",
        } },
        { "dxp", {
            "@dxp RACEID POINTS",
            "Gain digitalize XP for the specified demon race ID."
        } },
        { "effect", {
            "@effect ID [+]STACK [DEMON]",
            "Add or sets the stack count for status effect with",
            "the given ID for the player or demon if DEMON is set",
            "to 'demon'."
        } },
        { "enchant", {
            "@enchant EQUIP TAROT SOUL",
            "Set the enchnantment TAROT and SOUL effects for the",
            "specified EQUIP type. Use 0 to remove.",
            "EQUIP types: HEAD (0), FACE (1), NECK (2), TOP (3),",
            "ARMS (4), BOTTOM (5), FEET (6), COMP (7), RING (8),",
            "EARRING (9), EXTRA (10), BACK (11), TALISMAN (12)",
            "or WEAPON (13)",
        } },
        { "enemy", {
            "@enemy ID|NAME [AI [X Y [ROT]]]",
            "Spawns the enemy with the given ID or NAME at the",
            "position of the character or at the position X, Y",
            "with the rotation ROT. If AI is specified that AI",
            "type is used instead of the default."
        } },
        { "event", {
            "@event [ID]",
            "Starts an event specified by ID or returns the current",
            "event if not specified.",
        } },
        { "expertise", {
            "@expertise ID RANK",
            "Sets the expertise by ID to a specified RANK."
        } },
        { "expertisemax", {
            "@expertisemax STACKS",
            "Adds STACKS * 1000 points to the expertise cap.",
            "Maximum expertise cap is 154,000."
        } },
        { "familiarity", {
            "@familiarity VALUE",
            "Updates the current partner's familiarity to the given",
            "VALUE which can be in the range [0-10000]."
        } },
        { "flag", {
            "@flag TYPE [CID KEY [VALUE]]",
            "List, get or set zone flags. TYPE must be 'zone' or 'inst'.",
            "CID may be 0 for no specific character. If VALUE is not",
            "given the key is printed out instead of set.",
        } },
        { "fgauge", {
            "@fgauge VALUE",
            "Updates the current character's fusion gauge to the given",
            "VALUE which can be in the range of [0-10000] times the",
            "number of fusion gauge stocks available."
        } },
        { "goto", {
            "@goto [SELF] NAME",
            "If SELF is set to 'self' the player is moved to the",
            "character with the given name. If not set, the character",
            "with the given name is moved to the player."
        } },
        { "help", {
            "@help [GMAND]",
            "Lists the GMands supported by the server or prints the",
            "description of the GMAND specified."
        } },
        { "homepoint", {
            "@homepoint",
            "Sets your current position as your homepoint.",
        } },
        { "instance", {
            "@instance ID [VARIANTID]",
            "Creates a dungeon instance given the specified ",
            "instance ID and optional variant ID. This IDs must be in",
            "the XML data.",
        } },
        { "item", {
            "@item ID|NAME [QTY]",
            "Adds the item given by the ID or NAME in the specified",
            "quantity to the player's inventory. The NAME may be",
            "'macca' or 'mag' instead."
        } },
        { "kick", {
            "@kick NAME",
            "Kicks the character with the given NAME from the server.",
        } },
        { "kill", {
            "@kill [NAME]",
            "Kills the character with the given NAME or your player",
            "if no NAME is specified."
        } },
        { "levelup", {
            "@levelup LEVEL [DEMON]",
            "Levels up the player to the specified LEVEL or the",
            "player's current partner if DEMON is set to 'demon'."
        } },
        { "license", {
            "@license",
            "Prints license information for the running server.",
        } },
        { "lnc", {
            "@lnc VALUE",
            "Sets the player's LNC to the given VALUE. VALUE should",
            "be in the range [-10000, 10000]."
        } },
        { "map", {
            "@map ID",
            "Adds map for the player with the given ID.",
        } },
        { "plugin", {
            "@plugin ID",
            "Adds plugin for the player with the given ID.",
        } },
        { "pos", {
            "@pos [X Y]",
            "Prints out the X, Y position of the player or moves",
            "the player to the given X, Y position."
        } },
        { "post", {
            "@post ID [NAME]",
            "Adds the post item given by the ID to the character",
            "specified by NAME's post. If NAME is not specified",
            "the player's post is used."
        } },
        { "quest", {
            "@quest ID PHASE",
            "Sets the phase of the quest given by the ID to the phase",
            "PHASE. A phase of -1 is complete and -2 is a reset."
        } },
        { "reported",{
            "@reported [COUNT|PLAYERNAME]",
            "Get a set of unresolved reported player records of a",
            "specified COUNT or targeted at a specific PLAYERNAME."
        } },
        { "resolve",{
            "@resolve ENTRYUID",
            "Resolve a reported player record by UID. Records can",
            "be retrieved via @reported."
        } },
        { "reunion",{
            "@reunion TYPE [RANK]",
            "Perform reunion on your currently summoned demon setting",
            "the normal TYPE [1-12] and RANK or explicit growth TYPE",
            "by ID if no RANK specified."
        } },
        { "scrap", {
            "@scrap SLOT [NAME]",
            "Removes the item in slot SLOT from the character's",
            "inventory whose NAME is specified. The SLOT is a value",
            "in the range [1, 50] and if the NAME is not specified",
            "the player's inventory is used."
        } },
        { "skill", {
            "@skill ID DEMON",
            "Grants the skill with the specified ID to the player or",
            "the player's partner if DEMON is set to 'demon'."
        } },
        { "skillpoint", {
            "@skillpoint PTS",
            "Adds the specified number of skill points PTS to the",
            "available skill points for allocation."
        } },
        { "slotadd", {
            "@slotadd EQUIP",
            "Adds a slot to the specified EQUIP type.",
            "EQUIP types: TOP (3), BOTTOM (5) or WEAPON (13)"
        } },
        { "sp", {
            "@sp PTS",
            "Updates the player's partner to have PTS SP.",
        } },
        { "spawn", {
            "@spawn",
            "Spawn the max number of enemies in each spawn group",
            "in the current zone.",
        } },
        { "speed", {
            "@speed MULTIPLIER [DEMON]",
            "Multiplies the speed of the player by MULTIPLIER or",
            "the demon if DEMON is set to 'demon'."
        } },
        { "spirit", {
            "@spirit EQUIP BASIC SPECIAL [B1 B2 B3]",
            "Set the spirit fusion BASIC and SPECIAL effects for the",
            "specified EQUIP type. Effect values are equal to the item",
            "ID they are gained from. B1-B3 fusion bonuses from 0-50",
            "can also be specified but will otherwise be ignored.",
            "EQUIP types: HEAD (0), FACE (1), NECK (2), TOP (3),",
            "ARMS (4), BOTTOM (5), FEET (6), COMP (7), RING (8),",
            "EARRING (9), EXTRA (10), BACK (11), TALISMAN (12)",
            "or WEAPON (13)",
        } },
        { "tickermessage", {
            "@tickermessage MESSAGE...",
            "Sends the ticker message MESSAGE to all players.",
        } },
        { "title", {
            "@title ID",
            "Grants the player a new character title by ID."
        } },
        { "tokusei", {
            "@tokusei CLEAR|ID [STACK] [DEMON]",
            "Adds STACK of a tokusei given by the specified ID",
            "to the player or the player's demon if DEMON is set",
            "to 'demon'. STACK must be specified if CLEAR is not",
            "set to 'clear'. DEMON may not be specified if CLEAR",
            "is set."
        } },
        { "valuable", {
            "@valuable ID [REMOVE]",
            "Grants the player the valuable with the given ID. If",
            "REMOVE is set to 'remove' the valuable is removed."
        } },
        { "version", {
            "@version",
            "Prints version information for the running server.",
        } },
        { "worldtime",{
            "@worldtime OFFSET",
            "Adjusts the current channel's world clock time by",
            "a specified OFFSET (in seconds). 2 for 1 minute,"
            "120 for 1 hour and 1440 for 1 phase."
        } },
        { "xp", {
            "@xp PTS [DEMON]",
            "Grants the player PTS XP or the demon if DEMON is",
            "set to 'demon'."
        } },
        { "zone", {
            "@zone ID",
            "Moves the player to the zone specified by ID.",
        } },
    };

    if(!HaveUserLevel(client, 1))
    {
        return true;
    }

    libcomp::String command;

    if(1 < args.size())
    {
        command = "help";
    }
    else if(!args.empty())
    {
        command = args.front().ToLower();
    }

    if(command.IsEmpty())
    {
        for(auto cmd : usage)
        {
            SendChatMessage(client, ChatType_t::CHAT_SELF,
                cmd.second.front());
        }
    }
    else
    {
        auto cmd = usage.find(command.ToUtf8());

        if(usage.end() != cmd)
        {
            for(auto line : cmd->second)
            {
                SendChatMessage(client, ChatType_t::CHAT_SELF, line);
            }
        }
        else
        {
            SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
                "The command '%1' does not exist.").Arg(command));
        }
    }

    return true;
}

bool ChatManager::GMCommand_Homepoint(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 1))
    {
        return true;
    }

    (void)args;

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto zoneDef = cState->GetZone()->GetDefinition();
    auto spots = mServer.lock()->GetDefinitionManager()
        ->GetSpotData(zoneDef->GetDynamicMapID());
    for(auto spotPair : spots)
    {
        // Take the first zone-in point found
        if(spotPair.second->GetType() == 3)
        {
            character->SetHomepointZone(zoneDef->GetID());
            character->SetHomepointSpotID(spotPair.first);

            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_HOMEPOINT_UPDATE);
            p.WriteS32Little((int32_t)zoneDef->GetID());
            p.WriteFloat(spotPair.second->GetCenterX());
            p.WriteFloat(spotPair.second->GetCenterY());

            client->SendPacket(p);

            mServer.lock()->GetWorldDatabase()->QueueUpdate(character,
                state->GetAccountUID());

            return true;
        }
    }

    return SendChatMessage(client, ChatType_t::CHAT_SELF,
        "No valid spot ID found for the current zone");
}

bool ChatManager::GMCommand_Instance(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 200))
    {
        return true;
    }

    auto state = client->GetClientState();
    auto currentZone = state->GetZone();
    if(currentZone && currentZone->GetInstance())
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "You must leave the current instance to enter another");
    }

    std::list<libcomp::String> argsCopy = args;

    uint32_t instanceID;

    if(!GetIntegerArg<uint32_t>(instanceID, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@instance requires a zone instance ID");
    }

    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();
    auto zoneManager = server->GetZoneManager();

    auto instDef = serverDataManager->GetZoneInstanceData(instanceID);
    if(!instDef)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("Invalid instance ID supplied: %1")
            .Arg(instanceID));
    }

    uint32_t variantID = 0;
    if(argsCopy.size() > 0)
    {
        if(!GetIntegerArg<uint32_t>(variantID, argsCopy))
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                "Variant ID must be an integer");
        }
        else
        {
            auto variantDef = serverDataManager->GetZoneInstanceVariantData(
                variantID);
            if(!variantDef)
            {
                return SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Invalid variant ID supplied: %1")
                    .Arg(variantID));
            }
        }
    }

    auto instance = zoneManager->CreateInstance(client, instanceID, variantID);
    bool success = instance != nullptr;
    if(success)
    {
        uint32_t firstZoneID = *instDef->ZoneIDsBegin();
        uint32_t firstDynamicMapID = *instDef->DynamicMapIDsBegin();

        auto zoneDef = server->GetServerDataManager()->GetZoneData(
            firstZoneID, firstDynamicMapID);
        success = zoneManager->EnterZone(client, firstZoneID,
            firstDynamicMapID, zoneDef->GetStartingX(),
            zoneDef->GetStartingY(), zoneDef->GetStartingRotation());
    }

    if(!success)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("Failed to add to instance: %1").Arg(instanceID));
    }

    return true;
}

bool ChatManager::GMCommand_Item(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

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
    if(!HaveUserLevel(client, 400))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;
    libcomp::String kickedPlayer;

    if(!GetStringArg(kickedPlayer, argsCopy) || argsCopy.size() > 1)
    {
         return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@kick requires one argument, <username>");
    }

    std::shared_ptr<objects::Character> target;
    std::shared_ptr<objects::Account> targetAccount;
    std::shared_ptr<channel::ChannelClientConnection> targetClient;
    GetTargetCharacterAccount(kickedPlayer, false, target, targetAccount,
        targetClient);

    if(targetAccount &&
        client->GetClientState()->GetUserLevel() < targetAccount->GetUserLevel())
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Your user level is lower than the user you tried to kick.");
    }

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
    if(!HaveUserLevel(client, 500))
    {
        return true;
    }

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
        reply.WriteS32Little(MAX_PLAYER_HP_MP); // Damage 1
        reply.WriteU8(0);           // Damage 1 type (generic)
        reply.WriteS32Little(0);    // Damage 2
        reply.WriteU8(2);           // Damage 2 type (none)
        reply.WriteU16Little(1);    // Lethal flag
        reply.WriteBlank(48);

        zoneManager->BroadcastPacket(client, reply);

        // Cancel any pending skill
        auto activated = targetState->GetActivatedAbility();
        if(activated)
        {
            server->GetSkillManager()->CancelSkill(targetState,
                activated->GetActivationID());
        }

        std::set<std::shared_ptr<ActiveEntityState>> entities;
        entities.insert(targetState);
        characterManager->UpdateWorldDisplayState(entities);

        server->GetTokuseiManager()->Recalculate(cState,
            std::set<TokuseiConditionType> { TokuseiConditionType::CURRENT_HP });

        auto entityClient = server->GetManagerConnection()->GetEntityClient(
            targetState->GetEntityID());
        server->GetZoneManager()->TriggerZoneActions(targetState->GetZone(),
            { targetState }, ZoneTrigger_t::ON_DEATH, entityClient);
    }

    return true;
}

bool ChatManager::GMCommand_LevelUp(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

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
        auto dState = state->GetDemonState();

        if(!dState->GetEntity())
        {
            return SendChatMessage(client,
                ChatType_t::CHAT_SELF, "No demon summoned");
        }

        entityID = dState->GetEntityID();
        currentLevel = dState->GetLevel();
    }
    else
    {
        auto cState = state->GetCharacterState();
        entityID = cState->GetEntityID();
        currentLevel = cState->GetLevel();
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

bool ChatManager::GMCommand_License(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    (void)args;

    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "COMP_hack Server v%1.%2.%3 (%4)").Arg(VERSION_MAJOR).Arg(
        VERSION_MINOR).Arg(VERSION_PATCH).Arg(VERSION_CODENAME));
    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "Copyright (C) 2010-%1 COMP_hack Team").Arg(VERSION_YEAR));

    SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "This program is free software: you can redistribute it and/or modify");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "it under the terms of the GNU Affero General Public License as");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "published by the Free Software Foundation, either version 3 of the");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "License, or (at your option) any later version.");
    SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "This program is distributed in the hope that it will be useful,");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "but WITHOUT ANY WARRANTY; without even the implied warranty of");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "GNU Affero General Public License for more details.");
    SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "You should have received a copy of the GNU Affero General Public License");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "along with this program.  If not, see <https://www.gnu.org/licenses/>.");
    SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "IMPORTANT: If you see this you have a right to the source code!");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "This includes all modifications ANYONE has made to it! This does");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "not include any content changes (just the code). Please let us");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "know if someone isn't respecting the license for this software.");
    SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "You can get the mainline source from here:");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "https://github.com/comphack/comp_hack");
    SendChatMessage(client, ChatType_t::CHAT_SELF, " ");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "Hopefully if someone changed the code for the server you are connected");
    SendChatMessage(client, ChatType_t::CHAT_SELF, "to they added their URL here as well.");

    return true;
}

bool ChatManager::GMCommand_LNC(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 1))
    {
        return true;
    }

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
    if(!HaveUserLevel(client, 1))
    {
        return true;
    }

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

bool ChatManager::GMCommand_Plugin(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint16_t pluginID;
    if(!GetIntegerArg<uint16_t>(pluginID, argsCopy))
    {
        return false;
    }

    if(!mServer.lock()->GetCharacterManager()->AddPlugin(client,
        pluginID))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Invalid plugin ID supplied for @plugin command");
    }

    return true;
}

bool ChatManager::GMCommand_Position(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 200))
    {
        return true;
    }

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
    if(!HaveUserLevel(client, 750))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    auto targetAccount = client->GetClientState()->GetAccountUID();
    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();
    auto definitionManager = server->GetDefinitionManager();

    uint32_t productID;

    if(!GetIntegerArg<uint32_t>(productID, argsCopy) ||
        !definitionManager->GetShopProductData(productID))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid shop product ID specified: %1\n").Arg(productID));
    }

    libcomp::String name;

    if(GetStringArg(name, argsCopy))
    {
        auto target = objects::Character::LoadCharacterByName(worldDB, name);
        targetAccount = target ? target->GetAccount() : NULLUUID;
    }

    if(targetAccount.IsNull())
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Invalid post target character specified: %1\n").Arg(name));
    }

    auto postItems = objects::PostItem::LoadPostItemListByAccount(
        lobbyDB, targetAccount);
    if(postItems.size() >= MAX_POST_ITEM_COUNT)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "There is no more room in the Post!");
    }

    auto postItem = libcomp::PersistentObject::New<objects::PostItem>(true);
    postItem->SetType(productID);
    postItem->SetTimestamp((uint32_t)std::time(0));
    postItem->SetAccount(targetAccount);

    postItem->Insert(lobbyDB);

    return true;
}

bool ChatManager::GMCommand_Reported(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 400))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto lobbyDB = server->GetLobbyDatabase();
    auto worldDB = server->GetWorldDatabase();

    uint8_t count = 5;
    libcomp::String playerName;
    if(!GetIntegerArg(count, argsCopy))
    {
        GetStringArg(playerName, argsCopy);
    }

    // Cap at 5 records
    if(count > 5)
    {
        count = 5;
    }

    std::list<std::shared_ptr<objects::ReportedPlayer>> reported;
    if(!playerName.IsEmpty())
    {
        for(auto entry : objects::ReportedPlayer::
            LoadReportedPlayerListByPlayerName(worldDB, playerName))
        {
            if(!entry->GetResolved())
            {
                reported.push_back(entry);
            }
        }
    }
    else
    {
        reported = objects::ReportedPlayer::
            LoadReportedPlayerListByResolved(worldDB, false);
    }

    SendChatMessage(client, ChatType_t::CHAT_SELF,
        libcomp::String("%1 record%2 found%3")
        .Arg(reported.size()).Arg(reported.size() != 1 ? "s" : "")
        .Arg(reported.size() > (size_t)count ? " (limited to 5)" : ""));

    for(auto entry : reported)
    {
        SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Entry: %1").Arg(entry->GetUUID().ToString()));
        if(playerName.IsEmpty())
        {
            SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
                "Player: %1").Arg(entry->GetPlayerName()));
        }

        auto account = entry->GetReporter().Get(lobbyDB);

        SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Reported by: %1").Arg(account ? account->GetUsername()
                : "[Unknown]"));

        libcomp::String subject;
        switch(entry->GetSubject())
        {
        case objects::ReportedPlayer::Subject_t::CHAT_HARASSMENT:
            subject = "Chat harassment";
            break;
        case objects::ReportedPlayer::Subject_t::ABUSIVE_ACTION:
            subject = "Abusive action";
            break;
        case objects::ReportedPlayer::Subject_t::PROGRESS_OBSTRUCTION:
            subject = "Progress obstruction";
            break;
        case objects::ReportedPlayer::Subject_t::CHEATING:
            subject = "Cheating";
            break;
        case objects::ReportedPlayer::Subject_t::ILLEGAL_TOOL_USE:
            subject = "Illegal tool use";
            break;
        case objects::ReportedPlayer::Subject_t::REAL_MONEY_TRADE:
            subject = "Real money trade";
            break;
        case objects::ReportedPlayer::Subject_t::MISC_HARASSMENT:
            subject = "Misc harassment";
            break;
        case objects::ReportedPlayer::Subject_t::UNSPECIFIED:
        default:
            subject = "Unspecified";
            break;
        }

        SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Reason: %1").Arg(subject));

        if(!entry->GetLocation().IsEmpty())
        {
            SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
                "Location: %1").Arg(entry->GetLocation()));
        }

        SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Comment: %1").Arg(entry->GetComment()));
    }

    return true;
}

bool ChatManager::GMCommand_Resolve(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 400))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();

    bool failed = false;

    libcomp::String uidStr;
    libobjgen::UUID uuid;
    if(!GetStringArg(uidStr, argsCopy))
    {
        failed = true;
    }
    else
    {
        uuid = libobjgen::UUID(uidStr.C());
        failed = uuid.IsNull();
    }

    if(!failed)
    {
        auto reported = libcomp::PersistentObject::LoadObjectByUUID<
            objects::ReportedPlayer>(worldDB, uuid);
        if(reported)
        {
            if(!reported->GetResolved())
            {
                auto state = client->GetClientState();

                reported->SetResolved(true);
                reported->SetResolveTime((uint32_t)std::time(0));
                reported->SetResolver(state->GetAccountUID());

                failed = !reported->Update(worldDB);
            }
        }
        else
        {
            failed = true;
        }
    }

    if(failed)
    {
        SendChatMessage(client, ChatType_t::CHAT_SELF, "Resolve failed");
    }

    return true;
}

bool ChatManager::GMCommand_Reunion(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint8_t type;
    int8_t rank = -1;

    if(!GetIntegerArg(type, argsCopy) ||
        (argsCopy.size() > 0 && !GetIntegerArg(rank, argsCopy)))
    {
        return SendChatMessage(client,
            ChatType_t::CHAT_SELF, "Invalid parameters");
    }

    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();
    if(!demon)
    {
        return SendChatMessage(client,
            ChatType_t::CHAT_SELF, "Demon must be summoned");
    }

    int64_t demonID = state->GetObjectID(demon->GetUUID());

    auto server = mServer.lock();
    if(rank >= 0)
    {
        if(type < 1 || type > 12)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                "Invalid type specified with rank. Should be"
                " between 1 and 12");
        }

        // Determine actual growth type
        bool found = false;
        for(auto& pair : server->GetDefinitionManager()
            ->GetAllDevilLVUpRateData())
        {
            auto lvlUpRate = pair.second;
            if((uint8_t)lvlUpRate->GetGroupID() == type &&
                (lvlUpRate->GetSubID() == rank ||
                (rank > 9 && lvlUpRate->GetSubID() == 9)))
            {
                type = (uint8_t)pair.first;
                found = true;
                break;
            }
        }

        if(!found)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                "Invalid type/rank combination specified");
        }
    }

    if(!mServer.lock()->GetCharacterManager()->ReunionDemon(client, demonID,
        type, 0, false, true, rank))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Reunion failed");
    }

    return true;
}

bool ChatManager::GMCommand_Quest(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 200))
    {
        return true;
    }

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

bool ChatManager::GMCommand_Scrap(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 700))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint8_t slotNum = 0;
    if(GetIntegerArg<uint8_t>(slotNum, argsCopy))
    {
        if(slotNum == 0 || slotNum > 50)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                "@scrap slot numbers must be between 1 and 50");
        }
    }
    else
    {
        slotNum = 50;
    }

    auto targetClient = client;

    libcomp::String name;
    if(GetStringArg(name, argsCopy))
    {
        targetClient = nullptr;

        auto zoneManager = mServer.lock()->GetZoneManager();
        for(auto zConnection : zoneManager->GetZoneConnections(client, true))
        {
            auto zChar = zConnection->GetClientState()
                ->GetCharacterState()->GetEntity();

            if(zChar && zChar->GetName() == name)
            {
                targetClient = zConnection;
                break;
            }
        }

        if(!targetClient)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
                "Invalid character name supplied for the current zone: %1").Arg(name));
        }
    }

    if(targetClient)
    {
        auto cState = targetClient->GetClientState()->GetCharacterState();
        auto character = cState->GetEntity();
        auto inventory = character ? character->GetItemBoxes(0).Get() : nullptr;
        auto item = inventory ? inventory->GetItems((size_t)(slotNum - 1)).Get() : nullptr;
        if(item)
        {
            std::list<std::shared_ptr<objects::Item>> insertItems;
            std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;
            stackAdjustItems[item] = 0;

            if(mServer.lock()->GetCharacterManager()->UpdateItems(targetClient,
                false, insertItems, stackAdjustItems))
            {
                return SendChatMessage(client, ChatType_t::CHAT_SELF,
                    libcomp::String("Item %1 in slot %2 scrapped")
                    .Arg(item->GetType()).Arg(slotNum));
            }
        }
        else
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("No item in slot %1").Arg(slotNum));
        }
    }

    return SendChatMessage(client, ChatType_t::CHAT_SELF, "Could not scrap item");
}

bool ChatManager::GMCommand_Skill(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

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

bool ChatManager::GMCommand_SkillPoint(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    int32_t pointCount;
    if(!GetIntegerArg<int32_t>(pointCount, argsCopy) || pointCount < 0)
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->UpdateSkillPoints(
        client, pointCount);

    return true;
}

bool ChatManager::GMCommand_SlotAdd(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 650))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    const std::set<int8_t> validTypes = {
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON
    };

    int8_t equipType = GetEquipTypeArg(client, argsCopy, "slotadd", validTypes);
    if(equipType == -1)
    {
        return true;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto item = character
        ? character->GetEquippedItems((size_t)equipType).Get() : nullptr;

    if(!item)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "The specified item type is not equipped");
    }

    size_t openSlot = 0;
    for(uint32_t modSlot : item->GetModSlots())
    {
        if(modSlot == 0)
        {
            break;
        }

        openSlot++;
    }

    if(openSlot < item->ModSlotsCount())
    {
        item->SetModSlots(openSlot, MOD_SLOT_NULL_EFFECT);

        auto server = mServer.lock();

        auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
            libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
        if(itemBox)
        {
            server->GetCharacterManager()->SendItemBoxData(client, itemBox,
                { (uint16_t)item->GetBoxSlot() });
        }

        server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());
    }

    return true;
}

bool ChatManager::GMCommand_SoulPoints(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    int32_t pointCount;
    if(!GetIntegerArg<int32_t>(pointCount, argsCopy) || pointCount < 0)
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->UpdateSoulPoints(
        client, pointCount, true);

    return true;
}

bool ChatManager::GMCommand_Spawn(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 950))
    {
        return true;
    }

    (void)args;

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();
    auto zone = zoneManager->GetCurrentZone(client);

    zoneManager->UpdateSpawnGroups(zone, true);
    return true;
}

bool ChatManager::GMCommand_Speed(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 200))
    {
        return true;
    }

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

    if(entity)
    {
        int16_t defaultSpeed = entity->GetCombatRunSpeed();
        int16_t speed = (int16_t)ceil((float)defaultSpeed * scaling);
        int16_t maxSpeed = (int16_t)ceil((double)defaultSpeed * 1.5);

        if(speed > maxSpeed)
        {
            maxSpeed = speed;
        }

        entity->SetSpeedBoost((int16_t)(speed - defaultSpeed));

        server->GetCharacterManager()->SendMovementSpeed(client, entity, false);
    }

    return true;
}

bool ChatManager::GMCommand_Spirit(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 650))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    // All but BULLET types are valid
    const std::set<int8_t> validTypes = {
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_HEAD,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FACE,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NECK,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_ARMS,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BOTTOM,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_FEET,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_COMP,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_RING,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EARRING,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_EXTRA,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_BACK,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TALISMAN,
        (int8_t)objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON,
    };

    int8_t equipType = GetEquipTypeArg(client, argsCopy, "spirit", validTypes);
    if(equipType == -1)
    {
        return true;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto item = character
        ? character->GetEquippedItems((size_t)equipType).Get() : nullptr;

    if(!item)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "The specified item type is not equipped");
    }

    uint32_t basicEffect = 0;
    uint32_t specialEffect = 0;

    int8_t bonuses[] = { -1, -1, -1 };

    if(!GetIntegerArg(basicEffect, argsCopy) ||
        !GetIntegerArg(specialEffect, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Missing basic and special effects");
    }

    // Default to current effects or self if not specified
    bool basicDefault = false;
    if(basicEffect == 0)
    {
        basicEffect = item->GetBasicEffect()
            ? item->GetBasicEffect() : item->GetType();
        basicDefault = true;
    }

    bool specialDefault = false;
    if(specialEffect == 0)
    {
        specialEffect = item->GetSpecialEffect()
            ? item->GetSpecialEffect() : item->GetType();
        specialDefault = true;
    }

    for(size_t i = 0; i < 3; i++)
    {
        if(argsCopy.size() > 0 && (!GetIntegerArg(bonuses[i], argsCopy) ||
            bonuses[i] < -1 || bonuses[i] > 50))
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Invalid bonus %1 value supplied").Arg(i));
        }
    }

    auto basicDef = definitionManager->GetItemData(basicEffect);
    auto specialDef = definitionManager->GetItemData(specialEffect);
    if(!basicDef || !specialDef ||
        (int8_t)basicDef->GetBasic()->GetEquipType() != equipType ||
        (int8_t)specialDef->GetBasic()->GetEquipType() != equipType)
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Invalid basic or special effect");
    }

    // Do not apply other restrictions, go nuts

    // If clearing all, remove all effects
    if(basicDefault && specialDefault && !bonuses[0] &&
        !bonuses[1] && !bonuses[2])
    {
        item->SetBasicEffect(0);
        item->SetSpecialEffect(0);
    }
    else
    {
        item->SetBasicEffect(basicEffect);
        item->SetSpecialEffect(specialEffect);
    }

    for(size_t i = 0; i < 3; i++)
    {
        if(bonuses[i] >= 0)
        {
            item->SetFuseBonuses(i, bonuses[i]);
        }
    }

    auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
        libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
    if(itemBox)
    {
        server->GetCharacterManager()->SendItemBoxData(client, itemBox,
            { (uint16_t)item->GetBoxSlot() });
    }

    cState->RecalcEquipState(definitionManager);

    server->GetCharacterManager()->RecalculateTokuseiAndStats(cState, client);

    server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());

    return true;
}

bool ChatManager::GMCommand_TickerMessage(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 100))
    {
        return true;
    }

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

bool ChatManager::GMCommand_Title(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 100))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    int16_t titleID;
    if(!GetIntegerArg<int16_t>(titleID, argsCopy))
    {
        return false;
    }

    mServer.lock()->GetCharacterManager()->AddTitle(client,
        titleID);

    return true;
}

bool ChatManager::GMCommand_Tokusei(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    bool isClear = false;
    if(argsCopy.size() > 0 && argsCopy.front() == "clear")
    {
        isClear = true;
    }

    auto server = mServer.lock();

    int32_t tokuseiID;
    if(!isClear)
    {
        if(!GetIntegerArg<int32_t>(tokuseiID, argsCopy))
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                "@tokusei requires a tokusei ID or \"clear\"");
        }

        auto definitionManager = server->GetDefinitionManager();
        auto def = definitionManager->GetTokuseiData(tokuseiID);

        if(!def)
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Invalid tokusei ID supplied: %1")
                .Arg(tokuseiID));
        }
    }

    uint16_t count = 1;
    if(!isClear && !GetIntegerArg<uint16_t>(count, argsCopy))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "@tokusei requires an effect count if not clearing");
    }

    auto state = client->GetClientState();

    libcomp::String target;
    bool isDemon = GetStringArg(target, argsCopy) && target.ToLower() == "demon";
    auto eState = isDemon
        ? std::dynamic_pointer_cast<ActiveEntityState>(state->GetDemonState())
        : std::dynamic_pointer_cast<ActiveEntityState>(state->GetCharacterState());

    if(isClear)
    {
        eState->ClearAdditionalTokusei();
    }
    else
    {
        eState->SetAdditionalTokusei(tokuseiID, count);
    }

    server->GetTokuseiManager()->Recalculate(state->GetCharacterState(), true);

    return true;
}

bool ChatManager::GMCommand_Valuable(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 200))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint16_t valuableID;
    if(!GetIntegerArg<uint16_t>(valuableID, argsCopy))
    {
        return false;
    }

    bool remove = argsCopy.size() > 0 && argsCopy.front() == "remove";
    if(!mServer.lock()->GetCharacterManager()->AddRemoveValuable(client,
        valuableID, remove))
    {
        return SendChatMessage(client, ChatType_t::CHAT_SELF,
            "Invalid valuable ID supplied for @valuable command");
    }

    return true;
}

bool ChatManager::GMCommand_Version(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    (void)args;

    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "COMP_hack Server v%1.%2.%3 (%4)").Arg(VERSION_MAJOR).Arg(
        VERSION_MINOR).Arg(VERSION_PATCH).Arg(VERSION_CODENAME));

#if 1 == HAVE_GIT
    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "%1 on branch %2").Arg(szGitCommittish).Arg(szGitBranch));
    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "Commit by %1 on %2").Arg(szGitAuthor).Arg(szGitDate));
    SendChatMessage(client, ChatType_t::CHAT_SELF, szGitDescription);
    SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
        "URL: %1").Arg(szGitRemoteURL));
#endif

    return true;
}

bool ChatManager::GMCommand_WorldTime(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 950))
    {
        return true;
    }

    std::list<libcomp::String> argsCopy = args;

    uint32_t offset;
    if(!GetIntegerArg<uint32_t>(offset, argsCopy))
    {
        return false;
    }

    auto server = mServer.lock();

    server->SetTimeOffset(offset);

    auto clock = server->GetWorldClockTime();

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_WORLD_TIME);
    notify.WriteS8(clock.MoonPhase);
    notify.WriteS8(clock.Hour);
    notify.WriteS8(clock.Min);

    server->GetManagerConnection()->BroadcastPacketToClients(notify);

    return true;
}

bool ChatManager::GMCommand_Zone(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<libcomp::String>& args)
{
    if(!HaveUserLevel(client, 200))
    {
        return true;
    }

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

        if(!zoneManager->EnterZone(client, zoneID, dynamicMapID,
            xCoord, yCoord, rotation))
        {
            return SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Failed to enter zone: %1 (%2)")
                .Arg(zoneID).Arg(dynamicMapID));
        }

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
    if(!HaveUserLevel(client, 250))
    {
        return true;
    }

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

bool ChatManager::HaveUserLevel(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int32_t requiredLevel)
{
    int32_t currentLevel = client->GetClientState()->GetUserLevel();
    if(currentLevel < requiredLevel)
    {
        SendChatMessage(client, ChatType_t::CHAT_SELF, libcomp::String(
            "Requested GMand requires a user level of at least %1."
            " Your level is only %2.").Arg(requiredLevel).Arg(currentLevel));
        return false;
    }

    return true;
}

bool ChatManager::GetTargetCharacterAccount(libcomp::String& targetName,
    bool currentOnly, std::shared_ptr<objects::Character>& targetCharacter,
    std::shared_ptr<objects::Account>& targetAccount,
    std::shared_ptr<channel::ChannelClientConnection>& targetClient)
{
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();

    targetCharacter = objects::Character::LoadCharacterByName(worldDB, targetName);
    if(targetCharacter)
    {
        targetAccount = libcomp::PersistentObject::LoadObjectByUUID<
            objects::Account>(worldDB, targetCharacter->GetAccount());
    }
    else
    {
        targetAccount = nullptr;
    }

    targetClient = targetAccount ? server->GetManagerConnection()
        ->GetClientConnection(targetAccount->GetUsername()) : nullptr;

    if(currentOnly && targetClient)
    {
        auto state = targetClient->GetClientState();
        auto cState = state ? state->GetCharacterState() : nullptr;
        if(!cState || cState->GetEntity() != targetCharacter)
        {
            targetClient = nullptr;
        }
    }

    return targetCharacter && targetAccount;
}

int8_t ChatManager::GetEquipTypeArg(const std::shared_ptr<
    channel::ChannelClientConnection>& client, std::list<libcomp::String>& args,
    const libcomp::String& gmandName, const std::set<int8_t>& validTypes)
{
    // Listed in index order matching objects::MiItemBasicData::EquipType_t
    const libcomp::String txtVals[14] = {
        "head",
        "face",
        "neck",
        "top",
        "arms",
        "bottom",
        "feet",
        "comp",
        "ring",
        "earring",
        "extra",
        "back",
        "talisman",
        "weapon"
    };

    int8_t result = -1;
    if(!GetIntegerArg(result, args))
    {
        libcomp::String txt;
        if(!GetStringArg(txt, args))
        {
            SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Missing equip type argument for"
                    " command: @%1").Arg(gmandName));
            return -1;
        }

        txt = txt.ToLower();
        for(int8_t i = 0; i < 14; i++)
        {
            if(txtVals[(size_t)i] == txt)
            {
                result = i;
                break;
            }
        }

        if(result == -1)
        {
            SendChatMessage(client, ChatType_t::CHAT_SELF,
                libcomp::String("Invalid equip type argument '%1' supplied"
                    " for command: @%2").Arg(txt).Arg(gmandName));
            return -1;
        }
    }

    if(validTypes.find(result) == validTypes.end())
    {
        SendChatMessage(client, ChatType_t::CHAT_SELF,
            libcomp::String("Unsupported equip type argument supplied"
                " for command: @%1").Arg(gmandName));
        return -1;
    }

    return result;
}

bool ChatManager::GetStringArg(libcomp::String& outVal,
    std::list<libcomp::String>& args,
    libcomp::Convert::Encoding_t encoding) const
{
    if(args.size() == 0)
    {
        return false;
    }

    outVal = args.front().Replace("\\\"", "\"");
    args.pop_front();

    // Rebuild the full string if it starts with a double quote,
    // end if another unescaped double quote is read
    if(outVal.At(0) == '"')
    {
        while(outVal.At(outVal.Length() - 1) != '"' &&
            (outVal.Length() < 2 || outVal.At(outVal.Length() - 2) != '\\') &&
            !args.empty())
        {
            outVal += " " + args.front().Replace("\\\"", "\"");
            args.pop_front();
        }

        outVal = outVal.Mid(1, outVal.Length() - 2);
    }

    if(encoding != libcomp::Convert::Encoding_t::ENCODING_UTF8)
    {
        auto convertedBytes = libcomp::Convert::ToEncoding(
            encoding, outVal, false);
        outVal = libcomp::String (std::string(convertedBytes.begin(),
            convertedBytes.end()));
    }

    return true;
}
