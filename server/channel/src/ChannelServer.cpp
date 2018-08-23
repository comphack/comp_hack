/**
 * @file server/channel/src/ChannelServer.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Channel server class.
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

#include "ChannelServer.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerSystem.h>
#include <MessageTick.h>
#include <PacketCodes.h>
#include <ServerDataManager.h>

// object Includes
#include <Account.h>
#include <ChannelConfig.h>

// channel Includes
#include "AccountManager.h"
#include "ActionManager.h"
#include "AIManager.h"
#include "ChannelClientConnection.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "ChatManager.h"
#include "EventManager.h"
#include "FusionManager.h"
#include "ManagerClientPacket.h"
#include "ManagerConnection.h"
#include "Packets.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

ChannelServer::ChannelServer(const char *szProgram,
    std::shared_ptr<objects::ServerConfig> config,
    std::shared_ptr<libcomp::ServerCommandLineParser> commandLine) :
    libcomp::BaseServer(szProgram, config, commandLine), mAccountManager(0),
    mActionManager(0), mAIManager(0), mCharacterManager(0), mChatManager(0),
    mEventManager(0), mSkillManager(0), mZoneManager(0), mDefinitionManager(0),
    mServerDataManager(0), mRecalcTimeDependents(false), mMaxEntityID(0),
    mMaxObjectID(0), mTickRunning(true)
{
}

bool ChannelServer::Initialize()
{
    auto self = shared_from_this();

    if(!BaseServer::Initialize())
    {
        return false;
    }

    // Load newcharacter.xml for use when initializing new characters
    std::string newCharacterPath = GetConfigPath() + "newcharacter.xml";
    if(!LoadDataFromFile(newCharacterPath, mDefaultCharacterObjectMap, true,
        std::set<std::string>{ "Character", "CharacterProgress", "Demon",
            "EntityStats", "Expertise", "Hotbar", "Item" }))
    {
        LOG_INFO("No default character file loaded. New characters"
            " will start with nothing but chosen equipment and base"
            " expertise skills.\n");
    }

    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(mConfig);

    mDefinitionManager = new libcomp::DefinitionManager();
    if(!mDefinitionManager->LoadAllData(GetDataStore()))
    {
        return false;
    }

    mServerDataManager = new libcomp::ServerDataManager();
    if(!mServerDataManager->LoadData(GetDataStore(), mDefinitionManager))
    {
        return false;
    }

    mManagerConnection = std::make_shared<ManagerConnection>(self);

    auto internalPacketManager = std::make_shared<libcomp::ManagerPacket>(self);
    internalPacketManager->AddParser<Parsers::SetWorldInfo>(
        to_underlying(InternalPacketCode_t::PACKET_SET_WORLD_INFO));
    internalPacketManager->AddParser<Parsers::SetOtherChannelInfo>(
        to_underlying(InternalPacketCode_t::PACKET_SET_CHANNEL_INFO));
    internalPacketManager->AddParser<Parsers::AccountLogin>(
        to_underlying(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN));
    internalPacketManager->AddParser<Parsers::AccountLogout>(
        to_underlying(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT));
    internalPacketManager->AddParser<Parsers::Relay>(
        to_underlying(InternalPacketCode_t::PACKET_RELAY));
    internalPacketManager->AddParser<Parsers::DataSync>(
        to_underlying(InternalPacketCode_t::PACKET_DATA_SYNC));
    internalPacketManager->AddParser<Parsers::CharacterLogin>(
        to_underlying(InternalPacketCode_t::PACKET_CHARACTER_LOGIN));
    internalPacketManager->AddParser<Parsers::FriendsUpdate>(
        to_underlying(InternalPacketCode_t::PACKET_FRIENDS_UPDATE));
    internalPacketManager->AddParser<Parsers::PartyUpdate>(
        to_underlying(InternalPacketCode_t::PACKET_PARTY_UPDATE));
    internalPacketManager->AddParser<Parsers::ClanUpdate>(
        to_underlying(InternalPacketCode_t::PACKET_CLAN_UPDATE));
    internalPacketManager->AddParser<Parsers::WebGame>(
        to_underlying(InternalPacketCode_t::PACKET_WEB_GAME));

    //Add the managers to the main worker.
    mMainWorker.AddManager(internalPacketManager);
    mMainWorker.AddManager(mManagerConnection);

    //Add managers to the queue worker.
    auto systemManager = std::make_shared<channel::ManagerSystem>(self);
    mQueueWorker.AddManager(systemManager);

    // Map packet parsers to supported packets
    auto clientPacketManager = std::make_shared<ManagerClientPacket>(self);
    clientPacketManager->AddParser<Parsers::Login>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOGIN));
    clientPacketManager->AddParser<Parsers::Auth>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_AUTH));
    clientPacketManager->AddParser<Parsers::SendData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEND_DATA));
    clientPacketManager->AddParser<Parsers::Logout>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOGOUT));
    clientPacketManager->AddParser<Parsers::PopulateZone>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_POPULATE_ZONE));
    clientPacketManager->AddParser<Parsers::Move>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MOVE));
    clientPacketManager->AddParser<Parsers::Pivot>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PIVOT));
    clientPacketManager->AddParser<Parsers::Chat>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CHAT));
    clientPacketManager->AddParser<Parsers::Tell>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TELL));
    clientPacketManager->AddParser<Parsers::SkillActivate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SKILL_ACTIVATE));
    clientPacketManager->AddParser<Parsers::SkillExecute>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SKILL_EXECUTE));
    clientPacketManager->AddParser<Parsers::SkillCancel>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SKILL_CANCEL));
    clientPacketManager->AddParser<Parsers::SkillTarget>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SKILL_TARGET));
    clientPacketManager->AddParser<Parsers::ExpertiseDown>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EXPERTISE_DOWN));
    clientPacketManager->AddParser<Parsers::AllocateSkillPoint>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ALLOCATE_SKILL_POINT));
    clientPacketManager->AddParser<Parsers::ToggleExpertise>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TOGGLE_EXPERTISE));
    clientPacketManager->AddParser<Parsers::LearnSkill>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LEARN_SKILL));
    clientPacketManager->AddParser<Parsers::DemonSkillUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_SKILL_UPDATE));
    clientPacketManager->AddParser<Parsers::KeepAlive>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_KEEP_ALIVE));
    clientPacketManager->AddParser<Parsers::FixObjectPosition>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FIX_OBJECT_POSITION));
    clientPacketManager->AddParser<Parsers::State>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_STATE));
    clientPacketManager->AddParser<Parsers::DemonData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_DATA));
    clientPacketManager->AddParser<Parsers::DemonBox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_BOX));
    clientPacketManager->AddParser<Parsers::DemonBoxData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_BOX_DATA));
    clientPacketManager->AddParser<Parsers::ChannelList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CHANNEL_LIST));
    clientPacketManager->AddParser<Parsers::ReviveCharacter>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_REVIVE_CHARACTER));
    clientPacketManager->AddParser<Parsers::StopMovement>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_STOP_MOVEMENT));
    clientPacketManager->AddParser<Parsers::SpotTriggered>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SPOT_TRIGGERED));
    clientPacketManager->AddParser<Parsers::WorldTime>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_WORLD_TIME));
    clientPacketManager->AddParser<Parsers::ItemBox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_BOX));
    clientPacketManager->AddParser<Parsers::ItemMove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_MOVE));
    clientPacketManager->AddParser<Parsers::ItemDrop>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_DROP));
    clientPacketManager->AddParser<Parsers::ItemStack>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_STACK));
    clientPacketManager->AddParser<Parsers::EquipmentList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EQUIPMENT_LIST));
    clientPacketManager->AddParser<Parsers::TradeRequest>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRADE_REQUEST));
    clientPacketManager->AddParser<Parsers::TradeAccept>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRADE_ACCEPT));
    clientPacketManager->AddParser<Parsers::TradeAddItem>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRADE_ADD_ITEM));
    clientPacketManager->AddParser<Parsers::TradeLock>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRADE_LOCK));
    clientPacketManager->AddParser<Parsers::TradeFinish>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRADE_FINISH));
    clientPacketManager->AddParser<Parsers::TradeCancel>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRADE_CANCEL));
    clientPacketManager->AddParser<Parsers::LootItem>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOOT_ITEM));
    clientPacketManager->AddParser<Parsers::CashBalance>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CASH_BALANCE));
    clientPacketManager->AddParser<Parsers::ShopData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SHOP_DATA));
    clientPacketManager->AddParser<Parsers::ShopBuy>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SHOP_BUY));
    clientPacketManager->AddParser<Parsers::ShopSell>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SHOP_SELL));
    clientPacketManager->AddParser<Parsers::DemonBoxMove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_BOX_MOVE));
    clientPacketManager->AddParser<Parsers::DemonDismiss>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_DISMISS));
    clientPacketManager->AddParser<Parsers::PostList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_POST_LIST));
    clientPacketManager->AddParser<Parsers::PostItem>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_POST_ITEM));
    clientPacketManager->AddParser<Parsers::PostGift>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_POST_GIFT));
    clientPacketManager->AddParser<Parsers::HotbarData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_HOTBAR_DATA));
    clientPacketManager->AddParser<Parsers::HotbarSave>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_HOTBAR_SAVE));
    clientPacketManager->AddParser<Parsers::EventResponse>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EVENT_RESPONSE));
    clientPacketManager->AddParser<Parsers::ValuableList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_VALUABLE_LIST));
    clientPacketManager->AddParser<Parsers::ObjectInteraction>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_OBJECT_INTERACTION));
    clientPacketManager->AddParser<Parsers::FriendInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FRIEND_INFO));
    clientPacketManager->AddParser<Parsers::FriendRequest>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FRIEND_REQUEST));
    clientPacketManager->AddParser<Parsers::FriendAddRemove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FRIEND_ADD));
    clientPacketManager->AddParser<Parsers::FriendAddRemove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FRIEND_REMOVE));
    clientPacketManager->AddParser<Parsers::FriendData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FRIEND_DATA));
    clientPacketManager->AddParser<Parsers::PartyInvite>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_INVITE));
    clientPacketManager->AddParser<Parsers::PartyJoin>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_JOIN));
    clientPacketManager->AddParser<Parsers::PartyCancel>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_CANCEL));
    clientPacketManager->AddParser<Parsers::PartyLeave>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_LEAVE));
    clientPacketManager->AddParser<Parsers::PartyDisband>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_DISBAND));
    clientPacketManager->AddParser<Parsers::PartyLeaderUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_LEADER_UPDATE));
    clientPacketManager->AddParser<Parsers::PartyDropRule>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_DROP_RULE));
    clientPacketManager->AddParser<Parsers::PartyKick>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_KICK));
    clientPacketManager->AddParser<Parsers::DemonFusion>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_FUSION));
    clientPacketManager->AddParser<Parsers::LootDemonEggData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOOT_DEMON_EGG_DATA));
    clientPacketManager->AddParser<Parsers::Sync>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SYNC));
    clientPacketManager->AddParser<Parsers::ShopRepair>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SHOP_REPAIR));
    clientPacketManager->AddParser<Parsers::Rotate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ROTATE));
    clientPacketManager->AddParser<Parsers::LootBossBox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOOT_BOSS_BOX));
    clientPacketManager->AddParser<Parsers::UnionFlag>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_UNION_FLAG));
    clientPacketManager->AddParser<Parsers::ItemDepoList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_DEPO_LIST));
    clientPacketManager->AddParser<Parsers::DepoRent>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEPO_RENT));
    clientPacketManager->AddParser<Parsers::LootTreasureBox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOOT_TREASURE_BOX));
    clientPacketManager->AddParser<Parsers::QuestActiveList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_QUEST_ACTIVE_LIST));
    clientPacketManager->AddParser<Parsers::QuestCompletedList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_QUEST_COMPLETED_LIST));
    clientPacketManager->AddParser<Parsers::BazaarMarketOpen>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_MARKET_OPEN));
    clientPacketManager->AddParser<Parsers::BazaarMarketClose>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_MARKET_CLOSE));
    clientPacketManager->AddParser<Parsers::BazaarMarketInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_MARKET_INFO));
    clientPacketManager->AddParser<Parsers::BazaarItemAdd>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_ITEM_ADD));
    clientPacketManager->AddParser<Parsers::BazaarItemDrop>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_ITEM_DROP));
    clientPacketManager->AddParser<Parsers::BazaarItemUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_ITEM_UPDATE));
    clientPacketManager->AddParser<Parsers::BazaarItemBuy>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_ITEM_BUY));
    clientPacketManager->AddParser<Parsers::BazaarMarketSales>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_MARKET_SALES));
    clientPacketManager->AddParser<Parsers::SearchEntrySelf>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_ENTRY_SELF));
    clientPacketManager->AddParser<Parsers::SearchList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_LIST));
    clientPacketManager->AddParser<Parsers::SearchEntryData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_ENTRY_DATA));
    clientPacketManager->AddParser<Parsers::SearchEntryRegister>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_ENTRY_REGISTER));
    clientPacketManager->AddParser<Parsers::SearchEntryUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_ENTRY_UPDATE));
    clientPacketManager->AddParser<Parsers::SearchEntryRemove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_ENTRY_REMOVE));
    clientPacketManager->AddParser<Parsers::SearchAppReply>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_APPLICATION_REPLY));
    clientPacketManager->AddParser<Parsers::ClanDisband>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_DISBAND));
    clientPacketManager->AddParser<Parsers::ClanInvite>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_INVITE));
    clientPacketManager->AddParser<Parsers::ClanJoin>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_JOIN));
    clientPacketManager->AddParser<Parsers::ClanCancel>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_CANCEL));
    clientPacketManager->AddParser<Parsers::ClanKick>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_KICK));
    clientPacketManager->AddParser<Parsers::ClanMasterUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_MASTER_UPDATE));
    clientPacketManager->AddParser<Parsers::ClanSubMasterUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_SUB_MASTER_UPDATE));
    clientPacketManager->AddParser<Parsers::ClanLeave>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_LEAVE));
    clientPacketManager->AddParser<Parsers::ClanChat>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_CHAT));
    clientPacketManager->AddParser<Parsers::ClanInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_INFO));
    clientPacketManager->AddParser<Parsers::ClanList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_LIST));
    clientPacketManager->AddParser<Parsers::ClanData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_DATA));
    clientPacketManager->AddParser<Parsers::ClanForm>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_FORM));
    clientPacketManager->AddParser<Parsers::ItemPrice>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_PRICE));
    clientPacketManager->AddParser<Parsers::BazaarState>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_STATE));
    clientPacketManager->AddParser<Parsers::BazaarClerkSet>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_CLERK_SET));
    clientPacketManager->AddParser<Parsers::BazaarPrice>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_PRICE));
    clientPacketManager->AddParser<Parsers::BazaarMarketInfoSelf>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_MARKET_INFO_SELF));
    clientPacketManager->AddParser<Parsers::Warp>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_WARP));
    clientPacketManager->AddParser<Parsers::SkillExecuteInstant>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SKILL_EXECUTE_INSTANT));
    clientPacketManager->AddParser<Parsers::SyncCharacter>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SYNC_CHARACTER));
    clientPacketManager->AddParser<Parsers::DemonAISet>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_AI_SET));
    clientPacketManager->AddParser<Parsers::BazaarInteract>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_INTERACT));
    clientPacketManager->AddParser<Parsers::SkillForget>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SKILL_FORGET));
    clientPacketManager->AddParser<Parsers::BazaarMarketEnd>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_MARKET_END));
    clientPacketManager->AddParser<Parsers::BazaarMarketComment>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BAZAAR_MARKET_COMMENT));
    clientPacketManager->AddParser<Parsers::MapFlag>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MAP_FLAG));
    clientPacketManager->AddParser<Parsers::Analyze>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ANALYZE_DEMON));
    clientPacketManager->AddParser<Parsers::DemonCompendium>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_COMPENDIUM));
    clientPacketManager->AddParser<Parsers::ItemRepairMax>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_REPAIR_MAX));
    clientPacketManager->AddParser<Parsers::AppearanceAlter>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_APPEARANCE_ALTER));
    clientPacketManager->AddParser<Parsers::EntrustRequest>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENTRUST_REQUEST));
    clientPacketManager->AddParser<Parsers::EntrustAccept>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENTRUST_ACCEPT));
    clientPacketManager->AddParser<Parsers::EntrustRewardUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENTRUST_REWARD_UPDATE));
    clientPacketManager->AddParser<Parsers::EntrustRewardFinish>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENTRUST_REWARD_FINISH));
    clientPacketManager->AddParser<Parsers::EntrustRewardAccept>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENTRUST_REWARD_ACCEPT));
    clientPacketManager->AddParser<Parsers::EntrustFinish>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENTRUST_FINISH));
    clientPacketManager->AddParser<Parsers::DemonCrystallizeItem>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_CRYSTALLIZE_ITEM_UPDATE));
    clientPacketManager->AddParser<Parsers::DemonCrystallize>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_CRYSTALLIZE));
    clientPacketManager->AddParser<Parsers::EnchantItem>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENCHANT_ITEM_UPDATE));
    clientPacketManager->AddParser<Parsers::Enchant>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ENCHANT));
    clientPacketManager->AddParser<Parsers::DungeonRecords>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DUNGEON_RECORDS));
    clientPacketManager->AddParser<Parsers::Analyze>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ANALYZE_DUNGEON_RECORDS));
    clientPacketManager->AddParser<Parsers::TriFusionJoin>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRIFUSION_JOIN));
    clientPacketManager->AddParser<Parsers::TriFusionDemonUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRIFUSION_DEMON_UPDATE));
    clientPacketManager->AddParser<Parsers::TriFusionRewardUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRIFUSION_REWARD_UPDATE));
    clientPacketManager->AddParser<Parsers::TriFusionRewardAccept>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRIFUSION_REWARD_ACCEPT));
    clientPacketManager->AddParser<Parsers::TriFusionAccept>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRIFUSION_ACCEPT));
    clientPacketManager->AddParser<Parsers::TriFusionLeave>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRIFUSION_LEAVE));
    clientPacketManager->AddParser<Parsers::ClanEmblemUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_EMBLEM_UPDATE));
    clientPacketManager->AddParser<Parsers::DemonFamiliarity>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_FAMILIARITY));
    clientPacketManager->AddParser<Parsers::PlasmaStart>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PLASMA_START));
    clientPacketManager->AddParser<Parsers::PlasmaResult>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PLASMA_RESULT));
    clientPacketManager->AddParser<Parsers::PlasmaEnd>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PLASMA_END));
    clientPacketManager->AddParser<Parsers::PlasmaItemData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PLASMA_ITEM_DATA));
    clientPacketManager->AddParser<Parsers::PlasmaItem>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PLASMA_ITEM));
    clientPacketManager->AddParser<Parsers::TimeLimitSync>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TIME_LIMIT_SYNC));
    clientPacketManager->AddParser<Parsers::ItemDisassemble>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_DISASSEMBLE));
    clientPacketManager->AddParser<Parsers::SynthesizeRecipe>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SYNTHESIZE_RECIPE));
    clientPacketManager->AddParser<Parsers::Synthesize>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SYNTHESIZE));
    clientPacketManager->AddParser<Parsers::EquipmentMod>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EQUIPMENT_MODIFY));
    clientPacketManager->AddParser<Parsers::MaterialBox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MATERIAL_BOX));
    clientPacketManager->AddParser<Parsers::Analyze>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ANALYZE));
    clientPacketManager->AddParser<Parsers::MaterialExtract>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MATERIAL_EXTRACT));
    clientPacketManager->AddParser<Parsers::MaterialInsert>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MATERIAL_INSERT));
    clientPacketManager->AddParser<Parsers::ItemExchange>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_EXCHANGE));
    clientPacketManager->AddParser<Parsers::CompShopOpen>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMP_SHOP_OPEN));
    clientPacketManager->AddParser<Parsers::CompShopList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMP_SHOP_LIST));
    clientPacketManager->AddParser<Parsers::FusionGauge>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FUSION_GAUGE));
    clientPacketManager->AddParser<Parsers::TitleList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TITLE_LIST));
    clientPacketManager->AddParser<Parsers::TitleActiveUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TITLE_ACTIVE_UPDATE));
    clientPacketManager->AddParser<Parsers::TitleBuild>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TITLE_BUILD));
    clientPacketManager->AddParser<Parsers::DemonQuestData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_DATA));
    clientPacketManager->AddParser<Parsers::DemonQuestAccept>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_ACCEPT));
    clientPacketManager->AddParser<Parsers::DemonQuestEnd>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_END));
    clientPacketManager->AddParser<Parsers::DemonQuestCancel>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_CANCEL));
    clientPacketManager->AddParser<Parsers::DemonQuestList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_LIST));
    clientPacketManager->AddParser<Parsers::DemonQuestActive>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_ACTIVE));
    clientPacketManager->AddParser<Parsers::DemonLock>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_LOCK));
    clientPacketManager->AddParser<Parsers::DemonReunion>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_REUNION));
    clientPacketManager->AddParser<Parsers::DemonQuestReject>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_REJECT));
    clientPacketManager->AddParser<Parsers::PvPCharacterInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PVP_CHARACTER_INFO));
    clientPacketManager->AddParser<Parsers::AutoRecoveryUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_AUTO_RECOVERY_UPDATE));
    clientPacketManager->AddParser<Parsers::ItemMix>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_MIX));
    clientPacketManager->AddParser<Parsers::BikeBoostOn>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BIKE_BOOST_ON));
    clientPacketManager->AddParser<Parsers::BikeBoostOff>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BIKE_BOOST_OFF));
    clientPacketManager->AddParser<Parsers::BikeDismount>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BIKE_DISMOUNT));
    clientPacketManager->AddParser<Parsers::TeamInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TEAM_INFO));
    clientPacketManager->AddParser<Parsers::EquipmentSpiritFuse>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EQUIPMENT_SPIRIT_FUSE));
    clientPacketManager->AddParser<Parsers::DemonQuestPending>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_QUEST_PENDING));
    clientPacketManager->AddParser<Parsers::ItemDepoRemote>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_DEPO_REMOTE));
    clientPacketManager->AddParser<Parsers::DemonDepoRemote>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_DEPO_REMOTE));
    clientPacketManager->AddParser<Parsers::CommonSwitchUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMMON_SWITCH_UPDATE));
    clientPacketManager->AddParser<Parsers::CommonSwitchInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMMON_SWITCH_INFO));
    clientPacketManager->AddParser<Parsers::DemonForce>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_FORCE));
    clientPacketManager->AddParser<Parsers::DemonForceStack>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_FORCE_STACK));
    clientPacketManager->AddParser<Parsers::CasinoCoinTotal>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CASINO_COIN_TOTAL));
    clientPacketManager->AddParser<Parsers::TriFusionSolo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TRIFUSION_SOLO));
    clientPacketManager->AddParser<Parsers::EquipmentSpiritDefuse>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EQUIPMENT_SPIRIT_DEFUSE));
    clientPacketManager->AddParser<Parsers::DemonForceEnd>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_FORCE_END));
    clientPacketManager->AddParser<Parsers::SearchEntryInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_ENTRY_INFO));
    clientPacketManager->AddParser<Parsers::ITimeData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITIME_DATA));
    clientPacketManager->AddParser<Parsers::ITimeTalk>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITIME_TALK));
    clientPacketManager->AddParser<Parsers::CultureData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CULTURE_DATA));
    clientPacketManager->AddParser<Parsers::CultureMachineAccess>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CULTURE_MACHINE_ACCESS));
    clientPacketManager->AddParser<Parsers::CultureStart>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CULTURE_START));
    clientPacketManager->AddParser<Parsers::CultureItem>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CULTURE_ITEM));
    clientPacketManager->AddParser<Parsers::CultureEnd>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CULTURE_END));
    clientPacketManager->AddParser<Parsers::EquipmentModEdit>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EQUIPMENT_MOD_EDIT));
    clientPacketManager->AddParser<Parsers::PAttributeDeadline>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PATTRIBUTE_DEADLINE));
    clientPacketManager->AddParser<Parsers::MitamaReunion>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MITAMA_REUNION));
    clientPacketManager->AddParser<Parsers::MitamaReset>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MITAMA_RESET));
    clientPacketManager->AddParser<Parsers::DemonDepoList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_DEPO_LIST));
    clientPacketManager->AddParser<Parsers::DemonEquip>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_EQUIP));
    clientPacketManager->AddParser<Parsers::Barter>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BARTER));
    clientPacketManager->AddParser<Parsers::QuestTitle>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_QUEST_TITLE));
    clientPacketManager->AddParser<Parsers::ReportPlayer>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_REPORT_PLAYER));
    clientPacketManager->AddParser<Parsers::Blacklist>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BLACKLIST));
    clientPacketManager->AddParser<Parsers::BlacklistUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BLACKLIST_UPDATE));
    clientPacketManager->AddParser<Parsers::DigitalizePoints>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DIGITALIZE_POINTS));
    clientPacketManager->AddParser<Parsers::DigitalizeAssist>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DIGITALIZE_ASSIST));
    clientPacketManager->AddParser<Parsers::DigitalizeAssistLearn>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DIGITALIZE_ASSIST_LEARN));
    clientPacketManager->AddParser<Parsers::DigitalizeAssistRemove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DIGITALIZE_ASSIST_REMOVE));
    clientPacketManager->AddParser<Parsers::VABox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_VA_BOX));
    clientPacketManager->AddParser<Parsers::VABoxAdd>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_VA_BOX_ADD));
    clientPacketManager->AddParser<Parsers::VABoxRemove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_VA_BOX_REMOVE));
    clientPacketManager->AddParser<Parsers::VAChange>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_VA_CHANGE));
    clientPacketManager->AddParser<Parsers::VABoxMove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_VA_BOX_MOVE));
    clientPacketManager->AddParser<Parsers::ReunionPoints>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_REUNION_POINTS));
    clientPacketManager->AddParser<Parsers::ReunionExtract>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_REUNION_EXTRACT));
    clientPacketManager->AddParser<Parsers::ReunionInject>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_REUNION_INJECT));

    // Map the Unsupported packet parser to unsupported packets or packets that
    // the server does not need to react to
    clientPacketManager->AddParser<Parsers::Unsupported>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTY_MEMBER_UPDATE));
    clientPacketManager->AddParser<Parsers::Unsupported>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_UNSUPPORTED_0232));
    clientPacketManager->AddParser<Parsers::Unsupported>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_RECEIVED_PLAYER_DATA));
    clientPacketManager->AddParser<Parsers::Unsupported>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_RECEIVED_LISTS));

    // Add the managers to the generic workers.
    for(auto worker : mWorkers)
    {
        worker->AddManager(clientPacketManager);
        worker->AddManager(mManagerConnection);
    }

    auto channelPtr = std::dynamic_pointer_cast<ChannelServer>(self);
    mAccountManager = new AccountManager(channelPtr);
    mActionManager = new ActionManager(channelPtr);
    mAIManager = new AIManager(channelPtr);
    mCharacterManager = new CharacterManager(channelPtr);
    mChatManager = new ChatManager(channelPtr);
    mEventManager = new EventManager(channelPtr);
    mFusionManager = new FusionManager(channelPtr);
    mSkillManager = new SkillManager(channelPtr);
    mSyncManager = new ChannelSyncManager(channelPtr);

	mTokuseiManager = new TokuseiManager(channelPtr);
    if(!mTokuseiManager->Initialize())
    {
        return false;
    }

    mZoneManager = new ZoneManager(channelPtr);
    mZoneManager->LoadGeometry();

    // Pull first clock time then recalc
    auto clock = GetWorldClockTime();
    mTokuseiManager->RecalcTimedTokusei(clock);

    // Schedule the world clock to tick once every second
    auto sch = std::chrono::milliseconds(1000);
    mTimerManager.SchedulePeriodicEvent(sch, []
        (ChannelServer* pServer)
        {
            pServer->HandleClockEvents();
        }, this);

    // Schedule the demon quest reset for next midnight
    mTimerManager.ScheduleEventIn((int)GetTimeUntilMidnight(), []
        (ChannelServer* pServer)
        {
            pServer->HandleDemonQuestReset();
        }, this);

    // Now connect to the world server.
    auto worldConnection = std::make_shared<
        libcomp::InternalConnection>(mService);
    worldConnection->SetName("world");
    worldConnection->SetMessageQueue(mMainWorker.GetMessageQueue());

    mManagerConnection->SetWorldConnection(worldConnection);

    worldConnection->Connect(conf->GetWorldIP(), conf->GetWorldPort(), false);

    bool connected = libcomp::TcpConnection::STATUS_CONNECTED ==
        worldConnection->GetStatus();

    if(!connected)
    {
        LOG_CRITICAL("Failed to connect to the world server!\n");
        return false;
    }

    return true;
}

void ChannelServer::Shutdown()
{
    mTickRunning = false;

    BaseServer::Shutdown();
}

void ChannelServer::Cleanup()
{
    if(mTickThread.joinable())
    {
        mTickThread.join();
    }

    mDefaultCharacterObjectMap.clear();
}

ChannelServer::~ChannelServer()
{
    mTickRunning = false;

    if(mTickThread.joinable())
    {
        mTickThread.join();
    }

    delete mAccountManager;
    delete mActionManager;
    delete mAIManager;
    delete mCharacterManager;
    delete mChatManager;
    delete mEventManager;
    delete mFusionManager;
    delete mSkillManager;
    delete mSyncManager;
	delete mTokuseiManager;
    delete mZoneManager;
    delete mDefinitionManager;
    delete mServerDataManager;
}

ServerTime ChannelServer::GetServerTime()
{
    return sGetServerTime();
}

int32_t ChannelServer::GetExpirationInSeconds(uint32_t fixedTime, uint32_t relativeTo)
{
    if(fixedTime == 0)
    {
        return 0;
    }

    if(relativeTo == 0)
    {
        relativeTo = (uint32_t)time(0);
    }

    return (int32_t)(fixedTime > relativeTo ? fixedTime - relativeTo : 0);
}

const WorldClock ChannelServer::GetWorldClockTime()
{
    // World time is relative to seconds so no need for precision past time_t.
    time_t systemTime = std::time(0);

    // If the system time has not been updated, no need to run the
    // calculation again
    if((uint32_t)systemTime == mWorldClock.SystemTime)
    {
        return mWorldClock;
    }

    std::lock_guard<std::mutex> lock(mTimeLock);

    // Check again after lock so we don't double calculate
    if((uint32_t)systemTime == mWorldClock.SystemTime)
    {
        return mWorldClock;
    }

    bool eventPassed = mWorldClock.SystemTime < mNextEventTime &&
        mNextEventTime <= (uint32_t)systemTime;

    tm* t = gmtime(&systemTime);

    // Set the real time values
    WorldClock newClock;
    newClock.WeekDay = (int8_t)(t->tm_wday + 1);
    newClock.Month = (int8_t)(t->tm_mon + 1);
    newClock.Day = (int8_t)t->tm_mday;
    newClock.SystemHour = (int8_t)t->tm_hour;
    newClock.SystemMin = (int8_t)t->tm_min;
    newClock.SystemSec = (int8_t)t->tm_sec;

    newClock.SystemTime = (uint32_t)systemTime;
    newClock.GameOffset = mWorldClock.GameOffset;

    // Now calculate the game relative times

    // Every 4 days, 15 full moon cycles will elapse and the same game time
    // will occur on the same time offset.
    newClock.CycleOffset = (uint32_t)((newClock.SystemTime +
        newClock.GameOffset - BASE_WORLD_TIME) % 345600);

    // 24 minutes = 1 game phase (16 total)
    newClock.MoonPhase = (int8_t)((newClock.CycleOffset / 1440) % 16);

    // 2 minutes = 1 game hour
    newClock.Hour = (int8_t)((newClock.CycleOffset / 120) % 24);

    // 2 seconds = 1 game minute
    newClock.Min = (int8_t)((newClock.CycleOffset / 2) % 60);

    // Replace the old clock values
    mWorldClock = newClock;

    if(eventPassed || mNextEventTime == 0)
    {
        mRecalcTimeDependents = true;
        RecalcNextWorldEventTime();
    }

    return newClock;
}

void ChannelServer::SetTimeOffset(uint32_t offset)
{
    std::lock_guard<std::mutex> lock(mTimeLock);
    mWorldClock.GameOffset = offset;

    // Force a recalc
    mWorldClock.SystemTime = 0;
    mNextEventTime = 0;
    mLastEventTrigger = WorldClockTime();
}

const std::shared_ptr<objects::RegisteredChannel> ChannelServer::GetRegisteredChannel()
{
    return mRegisteredChannel;
}

const std::list<std::shared_ptr<objects::RegisteredChannel>>
    ChannelServer::GetAllRegisteredChannels()
{
    return mAllRegisteredChannels;
}

std::shared_ptr<objects::RegisteredWorld> ChannelServer::GetRegisteredWorld()
{
    return mRegisteredWorld;
}

void ChannelServer::RegisterWorld(const std::shared_ptr<
    objects::RegisteredWorld>& registeredWorld)
{
    mRegisteredWorld = registeredWorld;
}

void ChannelServer::LoadAllRegisteredChannels()
{
    mAllRegisteredChannels = libcomp::PersistentObject::LoadAll<
        objects::RegisteredChannel>(mWorldDatabase);

    // Key channels sorted by ID in ascending order
    auto currentChannel = mRegisteredChannel;
    mAllRegisteredChannels.sort([currentChannel](
        const std::shared_ptr<objects::RegisteredChannel>& a,
        const std::shared_ptr<objects::RegisteredChannel>& b)
    {
        return a->GetID() < b->GetID();
    });
}

std::shared_ptr<libcomp::Database> ChannelServer::GetWorldDatabase() const
{
    return mWorldDatabase;
}

void ChannelServer::SetWorldDatabase(const std::shared_ptr<libcomp::Database>& database)
{
    mWorldDatabase = database;
}

std::shared_ptr<libcomp::Database> ChannelServer::GetLobbyDatabase() const
{
    return mLobbyDatabase;
}

void ChannelServer::SetLobbyDatabase(const std::shared_ptr<libcomp::Database>& database)
{
    mLobbyDatabase = database;
}

bool ChannelServer::RegisterServer(uint8_t channelID)
{
    if(nullptr == mWorldDatabase)
    {
        return false;
    }

    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(mConfig);

    auto registeredChannel = objects::RegisteredChannel::LoadRegisteredChannelByID(
        mWorldDatabase, channelID);

    if(nullptr == registeredChannel)
    {
        auto name = conf->GetName().IsEmpty() ? libcomp::String("Channel %1").Arg(channelID)
            : conf->GetName();
        registeredChannel = std::shared_ptr<objects::RegisteredChannel>(new objects::RegisteredChannel);
        registeredChannel->SetID(channelID);
        registeredChannel->SetName(name);
        registeredChannel->SetPort(conf->GetPort());

        if(!conf->GetExternalIP().IsEmpty())
        {
            registeredChannel->SetIP(conf->GetExternalIP());
        }
        else
        {
            //Let the world set the IP it gets connected to from
            registeredChannel->SetIP("");
        }

        if(!registeredChannel->Register(registeredChannel) || !registeredChannel->Insert(mWorldDatabase))
        {
            return false;
        }
    }
    else
    {
        //Some other server already connected as this ID, let it fail
        return false;
    }

    mRegisteredChannel = registeredChannel;
    mAllRegisteredChannels.push_back(mRegisteredChannel);

    return true;
}

std::shared_ptr<ManagerConnection> ChannelServer::GetManagerConnection() const
{
    return mManagerConnection;
}

AccountManager* ChannelServer::GetAccountManager() const
{
    return mAccountManager;
}

ActionManager* ChannelServer::GetActionManager() const
{
    return mActionManager;
}

AIManager* ChannelServer::GetAIManager() const
{
    return mAIManager;
}

CharacterManager* ChannelServer::GetCharacterManager() const
{
    return mCharacterManager;
}

ChatManager* ChannelServer::GetChatManager() const
{
    return mChatManager;
}

EventManager* ChannelServer::GetEventManager() const
{
    return mEventManager;
}

FusionManager* ChannelServer::GetFusionManager() const
{
    return mFusionManager;
}

SkillManager* ChannelServer::GetSkillManager() const
{
    return mSkillManager;
}

ZoneManager* ChannelServer::GetZoneManager() const
{
    return mZoneManager;
}

libcomp::DefinitionManager* ChannelServer::GetDefinitionManager() const
{
    return mDefinitionManager;
}

libcomp::ServerDataManager* ChannelServer::GetServerDataManager() const
{
    return mServerDataManager;
}

ChannelSyncManager* ChannelServer::GetChannelSyncManager() const
{
    return mSyncManager;
}

TokuseiManager* ChannelServer::GetTokuseiManager() const
{
    return mTokuseiManager;
}

std::shared_ptr<objects::WorldSharedConfig>
    ChannelServer::GetWorldSharedConfig() const
{
    return std::dynamic_pointer_cast<objects::ChannelConfig>(
        GetConfig())->GetWorldSharedConfig();
}

int32_t ChannelServer::GetNextEntityID()
{
    std::lock_guard<std::mutex> lock(mLock);
    return ++mMaxEntityID;
}

int64_t ChannelServer::GetNextObjectID()
{
    std::lock_guard<std::mutex> lock(mLock);
    return ++mMaxObjectID;
}

void ChannelServer::Tick()
{
    ServerTime tickTime = GetServerTime();

    // Update the active zone states
    mZoneManager->UpdateActiveZoneStates();

    // Process queued world database changes
    auto worldFailures = mWorldDatabase->ProcessTransactionQueue();

    // Process queued lobby database changes
    auto lobbyFailures = mLobbyDatabase->ProcessTransactionQueue();

    if(worldFailures.size() > 0 || lobbyFailures.size() > 0)
    {
        // Disconnect any clients associated to failed account updates
        for(auto failures : { worldFailures, lobbyFailures })
        {
            for(auto failedUUID : failures)
            {
                auto account = std::dynamic_pointer_cast<objects::Account>(
                    libcomp::PersistentObject::GetObjectByUUID(failedUUID));

                if(nullptr != account)
                {
                    auto username = account->GetUsername();
                    auto client = mManagerConnection->GetClientConnection(
                        username);
                    if(nullptr != client)
                    {
                        LOG_ERROR(libcomp::String("Queued updates for client"
                            " failed to save for account: %1\n").Arg(username));
                        client->Close();
                    }
                }
            }
        }
    }

    std::map<ServerTime, std::list<libcomp::Message::Execute*>> schedule;
    {
        std::lock_guard<std::mutex> lock(mLock);

        // Retrieve all work scheduled for the current time or before
        for(auto it = mScheduledWork.begin(); it != mScheduledWork.end(); it++)
        {
            if(it->first <= tickTime)
            {
                schedule[it->first] = it->second;
            }
            else
            {
                break;
            }
        }

        // Clear all scheduled work retrieved from the member map
        for(auto it = schedule.begin(); it != schedule.end(); it++)
        {
            mScheduledWork.erase(it->first);
        }
    }

    // Queue any work that has been scheduled
    if(schedule.size() > 0)
    {
        auto queue = mQueueWorker.GetMessageQueue();
        for(auto it = schedule.begin(); it != schedule.end(); it++)
        {
            for(auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
            {
                queue->Enqueue(*it2);
            }
        }
    }
}

void ChannelServer::StartGameTick()
{
    mTickThread = std::thread([this](std::shared_ptr<
        libcomp::MessageQueue<libcomp::Message::Message*>> queue,
        volatile bool *pTickRunning)
    {
#if !defined(_WIN32)
        pthread_setname_np(pthread_self(), "tick");
#endif // !defined(_WIN32)

        const static int TICK_DELTA = 100;
        auto tickDelta = std::chrono::milliseconds(TICK_DELTA);

        while(*pTickRunning)
        {
            std::this_thread::sleep_for(tickDelta);
            queue->Enqueue(new libcomp::Message::Tick);
        }
    }, mQueueWorker.GetMessageQueue(), &mTickRunning);
}

bool ChannelServer::SendSystemMessage(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    libcomp::String message, int8_t type, bool sendToAll)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SYSTEM_MSG);
    p.WriteS8(type);
    p.WriteS8(0); // Appears to be some kind of sub-mode that is not used
    p.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
        message, true);

    if(!sendToAll)
    {
        client->SendPacket(p);
    }
    else
    {
        mManagerConnection->BroadcastPacketToClients(p);
    }
    return true;
}

int32_t ChannelServer::GetPAttributeDeadline()
{
    time_t systemTime = std::time(0);
    tm* t = gmtime(&systemTime);

    int systemDay = t->tm_wday;
    int systemHour = t->tm_hour;
    int systemMinutes = t->tm_min;
    int systemSeconds = t->tm_sec;

    // Get the system time for midnight of the next Monday
    int32_t deadlineDelta = ((7 - systemDay) * 86400) + ((23 - systemHour) * 3600) +
        ((59 - systemMinutes) * 60) + systemSeconds;
    int32_t deadline = (int32_t)systemTime + deadlineDelta;

    return deadline;
}

uint32_t ChannelServer::GetTimeUntilMidnight()
{
    time_t systemTime = std::time(0);
    tm* t = gmtime(&systemTime);

    int systemHour = t->tm_hour;
    int systemMinutes = t->tm_min;
    int systemSeconds = t->tm_sec;

    return (uint32_t)(((23 - systemHour) * 3600) +
        ((59 - systemMinutes) * 60) + systemSeconds);
}

ChannelServer::PersistentObjectMap
    ChannelServer::GetDefaultCharacterObjectMap() const
{
    return mDefaultCharacterObjectMap;
}

bool ChannelServer::RegisterClockEvent(WorldClockTime time, uint8_t type,
    bool remove)
{
    if(!time.IsSet())
    {
        // Ignore empty
        return false;
    }
    else if((time.Hour >= 0) != (time.Min >= 0) ||
        (time.SystemHour >= 0) != (time.SystemMin >= 0))
    {
        // Both hour and minute of a system or world time must be
        // set together
        return false;
    }
    else if(time.Hour >= 0 && time.SystemHour >= 0)
    {
        // World and system time cannot both be set
        return false;
    }

    bool recalcNext = false;

    std::lock_guard<std::mutex> lock(mTimeLock);
    if(remove)
    {
        mWorldClockEvents[time].erase(type);
        if(mWorldClockEvents[time].size() == 0)
        {
            mWorldClockEvents.erase(time);
            recalcNext = true;
        }
    }
    else
    {
        recalcNext = mWorldClockEvents.find(time) == mWorldClockEvents.end();
        mWorldClockEvents[time].insert(type);
    }

    if(recalcNext)
    {
        RecalcNextWorldEventTime();
    }

    return true;
}

void ChannelServer::HandleClockEvents()
{
    auto clock = GetWorldClockTime();
    auto lastTrigger = mLastEventTrigger;

    bool recalc = false;
    {
        std::lock_guard<std::mutex> lock(mTimeLock);
        if(mRecalcTimeDependents)
        {
            recalc = true;
            mRecalcTimeDependents = false;
            mLastEventTrigger = clock;
        }
    }

    if(recalc)
    {
        LOG_DEBUG(libcomp::String("Handling clock events at: %1\n")
            .Arg(clock.ToString()));

        mTokuseiManager->RecalcTimedTokusei(clock);
        mZoneManager->HandleTimedActions(clock, lastTrigger);
    }
}

void ChannelServer::HandleDemonQuestReset()
{
    // Get all currently logged in characters and reset their demon quests
    for(auto& client : mManagerConnection->GetAllConnections())
    {
        auto state = client->GetClientState();
        if(mEventManager->ResetDemonQuests(client))
        {
            LOG_DEBUG(libcomp::String("Demon quests reset for account: %1\n")
                .Arg(state->GetAccountUID().ToString()));
        }
        else
        {
            LOG_ERROR(libcomp::String("Failed to reset demon quests for"
                " account: %1\n").Arg(state->GetAccountUID().ToString()));
        }
    }

    // Reset timer to run again
    mTimerManager.ScheduleEventIn((int)GetTimeUntilMidnight(), []
        (ChannelServer* pServer)
        {
            pServer->HandleDemonQuestReset();
        }, this);
}

std::shared_ptr<libcomp::TcpConnection> ChannelServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    static int connectionID = 0;

    auto connection = std::make_shared<channel::ChannelClientConnection>(
        socket, CopyDiffieHellman(GetDiffieHellman()));
    connection->SetServerConfig(mConfig);
    connection->SetName(libcomp::String("client:%1").Arg(connectionID++));

    if(AssignMessageQueue(connection))
    {
        // Make sure this is called after connecting.
        connection->ConnectionSuccess();

        // Kill the connection if the client doesn't send packets
        // shortly after connecting
        connection->RefreshTimeout(GetServerTime(), 30);
    }
    else
    {
        connection->Close();
        return nullptr;
    }


    return connection;
}

GET_SERVER_TIME ChannelServer::sGetServerTime =
    std::chrono::high_resolution_clock::is_steady
        ? &ChannelServer::GetServerTimeHighResolution
        : &ChannelServer::GetServerTimeSteady;

ServerTime ChannelServer::GetServerTimeSteady()
{
    auto now = std::chrono::steady_clock::now();
    return (ServerTime)std::chrono::time_point_cast<std::chrono::microseconds>(now)
        .time_since_epoch().count();
}

ServerTime ChannelServer::GetServerTimeHighResolution()
{
    auto now = std::chrono::high_resolution_clock::now();
    return (ServerTime)std::chrono::time_point_cast<std::chrono::microseconds>(now)
        .time_since_epoch().count();
}

void ChannelServer::RecalcNextWorldEventTime()
{
    if(mWorldClock.IsSet() && mWorldClockEvents.size() > 0)
    {
        uint32_t timeToMidnight = (uint32_t)(
            ((23 - mWorldClock.SystemHour) * 3600) +
            ((59 - mWorldClock.SystemMin) * 60) + (60 - mWorldClock.SystemSec));

        // Midnight is always an option as day based times are not compared
        // at that level
        std::set<uint32_t> nextTimes = { timeToMidnight };

        uint8_t secOffset = (uint8_t)(mWorldClock.SystemSec % 2);

        int32_t timeSum = mWorldClock.Hour * 120 + mWorldClock.Min * 2 +
            secOffset;
        int32_t sysTimeSum = mWorldClock.SystemHour * 3600 +
            mWorldClock.SystemMin * 60 + mWorldClock.SystemSec;

        for(auto& pair : mWorldClockEvents)
        {
            const WorldClockTime& t = pair.first;

            // If the current time is not in the current phase,
            // calculate next time to phase no matter what
            bool inPhase = t.MoonPhase == -1 ||
                t.MoonPhase == mWorldClock.MoonPhase;

            uint32_t min = 0;
            if(inPhase && t.SystemHour != -1)
            {
                // Time to system time
                int32_t sysTimeSum2 = t.SystemHour * 3600 +
                    t.SystemMin * 60;
                min = (uint32_t)(sysTimeSum > sysTimeSum2
                    ? (86400 - sysTimeSum + sysTimeSum2)
                    : (sysTimeSum2 - sysTimeSum));
            }
            else if(inPhase && t.Hour != -1)
            {
                // Time to game time
                int32_t timeSum2 = t.Hour * 120 + t.Min * 2;
                min = (uint32_t)(timeSum > timeSum2
                    ? (1440 - timeSum + timeSum2)
                    : (timeSum2 - timeSum));
            }
            else
            {
                // Time to phase (full cycle if in phase)
                uint8_t phaseDelta = (uint8_t)(
                    mWorldClock.MoonPhase > t.MoonPhase
                    ? (16 - mWorldClock.MoonPhase + t.MoonPhase)
                    : t.MoonPhase - mWorldClock.MoonPhase);

                // Scale to seconds and reduce by time in current phase
                min = (uint32_t)((phaseDelta * 1440) - (timeSum % 1440));
            }

            if(min != 0)
            {
                nextTimes.insert(min);
            }
        }

        // Offset by the first one in the set
        mNextEventTime = (uint32_t)(
            mWorldClock.SystemTime + *nextTimes.begin());
    }
    else
    {
        mNextEventTime = 0;
    }
}
