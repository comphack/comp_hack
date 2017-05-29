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

#include "ChannelServer.h"

// libcomp Includes
#include <Log.h>
#include <ManagerSystem.h>
#include <MessageTick.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "ManagerClientPacket.h"
#include "Packets.h"

// Object Includes
#include "Account.h"
#include "ChannelConfig.h"

using namespace channel;

ChannelServer::ChannelServer(const char *szProgram, std::shared_ptr<
    objects::ServerConfig> config, const libcomp::String& configPath) :
    libcomp::BaseServer(szProgram, config, configPath), mAccountManager(0),
    mActionManager(0), mCharacterManager(0), mChatManager(0), mEventManager(0),
    mSkillManager(0), mZoneManager(0), mDefinitionManager(0), mServerDataManager(0),
    mMaxEntityID(0), mMaxObjectID(0)
{
}

bool ChannelServer::Initialize()
{
    auto self = shared_from_this();

    if(!BaseServer::Initialize())
    {
        return false;
    }

    auto conf = std::dynamic_pointer_cast<objects::ChannelConfig>(mConfig);

    mDefinitionManager = new libcomp::DefinitionManager();
    if(!mDefinitionManager->LoadAllData(GetDataStore()))
    {
        return false;
    }

    mServerDataManager = new libcomp::ServerDataManager();
    if(!mServerDataManager->LoadData(GetDataStore()))
    {
        return false;
    }

    // Connect to the world server.
    auto worldConnection = std::make_shared<
        libcomp::InternalConnection>(mService);
    worldConnection->SetMessageQueue(mMainWorker.GetMessageQueue());

    mManagerConnection = std::make_shared<ManagerConnection>(self);
    mManagerConnection->SetWorldConnection(worldConnection);

    worldConnection->Connect(conf->GetWorldIP(), conf->GetWorldPort(), false);

    bool connected = libcomp::TcpConnection::STATUS_CONNECTED ==
        worldConnection->GetStatus();

    if(!connected)
    {
        LOG_CRITICAL("Failed to connect to the world server!\n");
        return false;
    }

    auto internalPacketManager = std::make_shared<libcomp::ManagerPacket>(self);
    internalPacketManager->AddParser<Parsers::SetWorldInfo>(
        to_underlying(InternalPacketCode_t::PACKET_SET_WORLD_INFO));
    internalPacketManager->AddParser<Parsers::SetOtherChannelInfo>(
        to_underlying(InternalPacketCode_t::PACKET_SET_CHANNEL_INFO));
    internalPacketManager->AddParser<Parsers::AccountLogin>(
        to_underlying(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN));

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
    clientPacketManager->AddParser<Parsers::Move>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MOVE));
    clientPacketManager->AddParser<Parsers::PopulateZone>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_POPULATE_ZONE));
    clientPacketManager->AddParser<Parsers::Chat>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CHAT));
    clientPacketManager->AddParser<Parsers::ActivateSkill>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ACTIVATE_SKILL));
    clientPacketManager->AddParser<Parsers::ExecuteSkill>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_EXECUTE_SKILL));
    clientPacketManager->AddParser<Parsers::CancelSkill>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CANCEL_SKILL));
    clientPacketManager->AddParser<Parsers::AllocateSkillPoint>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ALLOCATE_SKILL_POINT));
    clientPacketManager->AddParser<Parsers::ToggleExpertise>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TOGGLE_EXPERTISE));
    clientPacketManager->AddParser<Parsers::LearnSkill>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LEARN_SKILL));
    clientPacketManager->AddParser<Parsers::UpdateDemonSkill>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_UPDATE_DEMON_SKILL));
    clientPacketManager->AddParser<Parsers::KeepAlive>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_KEEP_ALIVE));
    clientPacketManager->AddParser<Parsers::FixObjectPosition>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FIX_OBJECT_POSITION));
    clientPacketManager->AddParser<Parsers::State>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_STATE));
    clientPacketManager->AddParser<Parsers::PartnerDemonData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTNER_DEMON_DATA));
    clientPacketManager->AddParser<Parsers::DemonBox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_BOX));
    clientPacketManager->AddParser<Parsers::DemonBoxData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_BOX_DATA));
    clientPacketManager->AddParser<Parsers::ChannelList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CHANNEL_LIST));
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
    clientPacketManager->AddParser<Parsers::DemonBoxMove>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_BOX_MOVE));
    clientPacketManager->AddParser<Parsers::DismissDemon>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DISMISS_DEMON));
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
    clientPacketManager->AddParser<Parsers::Sync>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SYNC));
    clientPacketManager->AddParser<Parsers::Rotate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ROTATE));
    clientPacketManager->AddParser<Parsers::UnionFlag>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_UNION_FLAG));
    clientPacketManager->AddParser<Parsers::ItemDepoList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_DEPO_LIST));
    clientPacketManager->AddParser<Parsers::DepoRent>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEPO_RENT));
    clientPacketManager->AddParser<Parsers::QuestActiveList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_QUEST_ACTIVE_LIST));
    clientPacketManager->AddParser<Parsers::QuestCompletedList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_QUEST_COMPLETED_LIST));
    clientPacketManager->AddParser<Parsers::ClanInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CLAN_INFO));
    clientPacketManager->AddParser<Parsers::MapFlag>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MAP_FLAG));
    clientPacketManager->AddParser<Parsers::DemonCompendium>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_COMPENDIUM));
    clientPacketManager->AddParser<Parsers::DungeonRecords>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DUNGEON_RECORDS));
    clientPacketManager->AddParser<Parsers::MaterialBox>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_MATERIAL_BOX));
    clientPacketManager->AddParser<Parsers::FusionGauge>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FUSION_GAUGE));
    clientPacketManager->AddParser<Parsers::TitleList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TITLE_LIST));
    clientPacketManager->AddParser<Parsers::PartnerDemonQuestList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTNER_DEMON_QUEST_LIST));
    clientPacketManager->AddParser<Parsers::LockDemon>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOCK_DEMON));
    clientPacketManager->AddParser<Parsers::PvPCharacterInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PVP_CHARACTER_INFO));
    clientPacketManager->AddParser<Parsers::TeamInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TEAM_INFO));
    clientPacketManager->AddParser<Parsers::PartnerDemonQuestTemp>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTNER_DEMON_QUEST_TEMP));
    clientPacketManager->AddParser<Parsers::ItemDepoRemote>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ITEM_DEPO_REMOTE));
    clientPacketManager->AddParser<Parsers::DemonDepoRemote>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_DEPO_REMOTE));
    clientPacketManager->AddParser<Parsers::CommonSwitchInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMMON_SWITCH_INFO));
    clientPacketManager->AddParser<Parsers::CasinoCoinTotal>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CASINO_COIN_TOTAL));
    clientPacketManager->AddParser<Parsers::SearchEntryInfo>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SEARCH_ENTRY_INFO));
    clientPacketManager->AddParser<Parsers::HouraiData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_HOURAI_DATA));
    clientPacketManager->AddParser<Parsers::CultureData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_CULTURE_DATA));
    clientPacketManager->AddParser<Parsers::DemonDepoList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DEMON_DEPO_LIST));
    clientPacketManager->AddParser<Parsers::Blacklist>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_BLACKLIST));
    clientPacketManager->AddParser<Parsers::DigitalizePoints>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DIGITALIZE_POINTS));
    clientPacketManager->AddParser<Parsers::DigitalizeAssist>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DIGITALIZE_ASSIST));

    // Map the Unsupported packet parser to unsupported packets or packets that
    // the server does not need to react to
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
    mCharacterManager = new CharacterManager(channelPtr);
    mChatManager = new ChatManager(channelPtr);
    mEventManager = new EventManager(channelPtr);
    mSkillManager = new SkillManager(channelPtr);
    mZoneManager = new ZoneManager(channelPtr);

    return true;
}

ChannelServer::~ChannelServer()
{
    if(mTickThread.joinable())
    {
        mTickThread.join();
    }

    delete[] mAccountManager;
    delete[] mCharacterManager;
    delete[] mChatManager;
    delete[] mEventManager;
    delete[] mSkillManager;
    delete[] mZoneManager;
    delete[] mDefinitionManager;
    delete[] mServerDataManager;
}

ServerTime ChannelServer::GetServerTime()
{
    auto now = std::chrono::system_clock::now();
    return (ServerTime)std::chrono::time_point_cast<std::chrono::microseconds>(now)
        .time_since_epoch().count();
}

uint32_t ChannelServer::GetServerTimeInSeconds()
{
    auto now = std::chrono::system_clock::now();
    return (uint32_t)std::chrono::time_point_cast<std::chrono::seconds>(now)
        .time_since_epoch().count();
}

int32_t ChannelServer::GetExpirationInSeconds(uint32_t fixedTime, uint32_t relativeTo)
{
    if(relativeTo == 0)
    {
        relativeTo = GetServerTimeInSeconds();
    }

    return (int32_t)(fixedTime > relativeTo ? fixedTime - relativeTo : 0);
}

void ChannelServer::GetWorldClockTime(int8_t& phase, int8_t& hour, int8_t& min)
{
    // Use the first calculable new moon start time.
    // (Adjusted from NEW MOON 00:00 on Sep 9, 2012 12:47:40 GMT)
    static uint64_t baseTime = 46060;

    // World time is relative to seconds so no need for precision past time_t.
    // Every 4 days, 15 full moon cycles will elapse and the same game time will
    // occur on the same time offset.
    uint64_t nowOffset = (uint64_t)(((uint64_t)std::time(nullptr) - baseTime) % 345600);

    // 24 minutes = 1 game phase (16 total)
    phase = (int8_t)((nowOffset / 1440) % 16);

    // 2 minutes = 1 game hour
    hour = (int8_t)((nowOffset / 120) % 24);

    // 2 seconds = 1 game minute
    min = (int8_t)((nowOffset / 2) % 60);
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
    /// @todo: update zone states

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

    QueueNextTick();
}

std::shared_ptr<libcomp::TcpConnection> ChannelServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::make_shared<channel::ChannelClientConnection>(
        socket, CopyDiffieHellman(GetDiffieHellman()));
    connection->SetServerConfig(mConfig);

    if(AssignMessageQueue(connection))
    {
        // Make sure this is called after connecting.
        connection->ConnectionSuccess();
    }
    else
    {
        connection->Close();
        return nullptr;
    }


    return connection;
}

void ChannelServer::QueueNextTick()
{
    if(mTickThread.joinable())
    {
        mTickThread.join();
    }
    
    mTickThread = std::thread([this](std::shared_ptr<
        libcomp::MessageQueue<libcomp::Message::Message*>> queue)
    {
        const static int tickDelta = 100;
        std::this_thread::sleep_for(std::chrono::milliseconds(tickDelta));
        queue->Enqueue(new libcomp::Message::Tick);
    }, mQueueWorker.GetMessageQueue());
}
