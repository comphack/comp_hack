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
#include <ManagerPacket.h>
#include <PacketCodes.h>

// channel Includes
#include "ChannelClientConnection.h"
#include "Packets.h"

// Object Includes
#include "ChannelConfig.h"

using namespace channel;

ChannelServer::ChannelServer(std::shared_ptr<objects::ServerConfig> config,
    const libcomp::String& configPath) : libcomp::BaseServer(config, configPath),
    mAccountManager(0), mCharacterManager(0), mChatManager(0), mDefinitionManager(0),
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
    if(!mDefinitionManager->LoadAllData(conf->GetBinaryDataDirectory()))
    {
        return false;
    }

    mServerDataManager = new libcomp::ServerDataManager();
    if(!mServerDataManager->LoadData(conf->GetServerDataDefinitionsFile()))
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
    internalPacketManager->AddParser<Parsers::AccountLogin>(
        to_underlying(InternalPacketCode_t::PACKET_ACCOUNT_LOGIN));

    //Add the managers to the main worker.
    mMainWorker.AddManager(internalPacketManager);
    mMainWorker.AddManager(mManagerConnection);

    auto clientPacketManager = std::make_shared<libcomp::ManagerPacket>(self);
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
    clientPacketManager->AddParser<Parsers::AllocateSkillPoint>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ALLOCATE_SKILL_POINT));
    clientPacketManager->AddParser<Parsers::ToggleExpertise>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_TOGGLE_EXPERTISE));
    clientPacketManager->AddParser<Parsers::LearnSkill>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LEARN_SKILL));
    clientPacketManager->AddParser<Parsers::KeepAlive>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_KEEP_ALIVE));
    clientPacketManager->AddParser<Parsers::FixObjectPosition>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_FIX_OBJECT_POSITION));
    clientPacketManager->AddParser<Parsers::State>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_STATE));
    clientPacketManager->AddParser<Parsers::PartnerDemonData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_PARTNER_DEMON_DATA));
    clientPacketManager->AddParser<Parsers::COMPList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMP_LIST));
    clientPacketManager->AddParser<Parsers::COMPDemonData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMP_DEMON_DATA));
    clientPacketManager->AddParser<Parsers::StopMovement>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_STOP_MOVEMENT));
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
    clientPacketManager->AddParser<Parsers::COMPSlotUpdate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_COMP_SLOT_UPDATE));
    clientPacketManager->AddParser<Parsers::DismissDemon>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_DISMISS_DEMON));
    clientPacketManager->AddParser<Parsers::HotbarData>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_HOTBAR_DATA));
    clientPacketManager->AddParser<Parsers::HotbarSave>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_HOTBAR_SAVE));
    clientPacketManager->AddParser<Parsers::ValuableList>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_VALUABLE_LIST));
    clientPacketManager->AddParser<Parsers::Sync>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_SYNC));
    clientPacketManager->AddParser<Parsers::Rotate>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_ROTATE));
    clientPacketManager->AddParser<Parsers::UnionFlag>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_UNION_FLAG));
    clientPacketManager->AddParser<Parsers::LockDemon>(
        to_underlying(ClientToChannelPacketCode_t::PACKET_LOCK_DEMON));

    // Add the managers to the generic workers.
    for(auto worker : mWorkers)
    {
        worker->AddManager(clientPacketManager);
        worker->AddManager(mManagerConnection);
    }

    auto channelPtr = std::dynamic_pointer_cast<ChannelServer>(self);
    mAccountManager = new AccountManager(channelPtr);
    mCharacterManager = new CharacterManager(channelPtr);
    mChatManager = new ChatManager(channelPtr);

    return true;
}

ChannelServer::~ChannelServer()
{
    delete[] mAccountManager;
    delete[] mCharacterManager;
    delete[] mChatManager;
    delete[] mDefinitionManager;
    delete[] mServerDataManager;
}

ServerTime ChannelServer::GetServerTime()
{
    return sGetServerTime();
}

const std::shared_ptr<objects::RegisteredChannel> ChannelServer::GetRegisteredChannel()
{
    return mRegisteredChannel;
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

CharacterManager* ChannelServer::GetCharacterManager() const
{
    return mCharacterManager;
}

ChatManager* ChannelServer::GetChatManager() const
{
    return mChatManager;
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

std::shared_ptr<libcomp::TcpConnection> ChannelServer::CreateConnection(
    asio::ip::tcp::socket& socket)
{
    auto connection = std::make_shared<channel::ChannelClientConnection>(
        socket, CopyDiffieHellman(GetDiffieHellman()));

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
