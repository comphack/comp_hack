/**
 * @file server/channel/src/ClientState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a client connection.
 *
 * This file is part of the Channel Server (channel).
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

#include "ClientState.h"

// Standard C++11 Includes
#include <ctime>

// libcomp Includes
#include <Packet.h>
#include <PacketCodes.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <BazaarData.h>
#include <CharacterLogin.h>
#include <ClientCostAdjustment.h>
#include <EventInstance.h>
#include <EventOpenMenu.h>
#include <EventState.h>

// channel Includes
#include "ChannelServer.h"
#include "Zone.h"

using namespace channel;

std::unordered_map<bool, std::unordered_map<int32_t, ClientState*>>
    ClientState::sEntityClients;
std::mutex ClientState::sLock;

ClientState::ClientState()
    : objects::ClientStateObject(),
      mCharacterState(std::shared_ptr<CharacterState>(new CharacterState)),
      mDemonState(std::shared_ptr<DemonState>(new DemonState)),
      mStartTime(ChannelServer::GetServerTime()),
      mNextLocalObjectID(1) {}

ClientState::~ClientState() {
  auto cEntityID = mCharacterState->GetEntityID();
  auto dEntityID = mDemonState->GetEntityID();
  auto worldCID = GetWorldCID();
  if (cEntityID != 0 || dEntityID != 0) {
    std::lock_guard<std::mutex> lock(sLock);
    sEntityClients[false].erase(cEntityID);
    sEntityClients[false].erase(dEntityID);
    sEntityClients[true].erase(worldCID);
  }
}

libcomp::Convert::Encoding_t ClientState::GetClientStringEncoding() {
  /// @todo: Return UTF-8 for US Client
  return libcomp::Convert::Encoding_t::ENCODING_DEFAULT;
}

std::shared_ptr<CharacterState> ClientState::GetCharacterState() {
  return mCharacterState;
}

std::shared_ptr<DemonState> ClientState::GetDemonState() { return mDemonState; }

std::shared_ptr<ActiveEntityState> ClientState::GetEntityState(int32_t entityID,
                                                               bool readyOnly) {
  std::list<std::shared_ptr<ActiveEntityState>> states = {mCharacterState,
                                                          mDemonState};
  for (auto state : states) {
    if (state->GetEntityID() == entityID &&
        (!readyOnly || state->Ready(true))) {
      return state;
    }
  }

  return nullptr;
}

std::shared_ptr<BazaarState> ClientState::GetBazaarState() {
  auto zone = mCharacterState->GetZone();

  auto worldData = GetAccountWorldData().Get();
  auto bazaarData = worldData->GetBazaarData().Get();
  uint32_t marketID = bazaarData ? bazaarData->GetMarketID() : 0;

  if (zone && marketID != 0) {
    for (auto bState : zone->GetBazaars()) {
      if (bState->GetCurrentMarket(marketID) == bazaarData) {
        return bState;
      }
    }
  }

  return nullptr;
}

bool ClientState::IsMovementLocked(int32_t entityID) {
  auto eState = GetEntityState(entityID, false);
  if (!eState) {
    return false;
  }

  // Lock set directly on the state stops movement of either entity
  if (GetLockMovement()) {
    return true;
  }

  // Player characters can also be locked by their current event status
  if (eState->GetEntityType() != EntityType_t::CHARACTER) {
    return false;
  }

  // Active event and active trade stop the player character too
  int8_t status = GetStatusIcon();
  return status == 4 || status == 8;
}

bool ClientState::HasActiveEvent() const {
  auto eState = GetEventState();
  auto current = eState ? eState->GetCurrent() : nullptr;

  return current != nullptr;
}

int32_t ClientState::GetEventSourceEntityID() const {
  auto eState = GetEventState();
  auto current = eState ? eState->GetCurrent() : nullptr;

  return current ? current->GetSourceEntityID() : 0;
}

int32_t ClientState::GetCurrentMenuShopID(int32_t type) const {
  auto eState = GetEventState();
  auto current = eState ? eState->GetCurrent() : nullptr;
  if (current) {
    auto menu =
        std::dynamic_pointer_cast<objects::EventOpenMenu>(current->GetEvent());
    if (menu && (!type || menu->GetMenuType() == type)) {
      return menu->GetShopID();
    }
  }

  return 0;
}

bool ClientState::Register() {
  auto cEntityID = mCharacterState->GetEntityID();
  auto dEntityID = mDemonState->GetEntityID();
  auto worldCID = GetWorldCID();
  if (cEntityID == 0 || dEntityID == 0 || worldCID == 0) {
    return false;
  }

  std::lock_guard<std::mutex> lock(sLock);
  if (sEntityClients[false].find(cEntityID) != sEntityClients[false].end() ||
      sEntityClients[false].find(dEntityID) != sEntityClients[false].end()) {
    return false;
  }

  sEntityClients[false][cEntityID] = this;
  sEntityClients[false][dEntityID] = this;
  sEntityClients[true][worldCID] = this;

  return true;
}

int64_t ClientState::GetObjectID(const libobjgen::UUID& uuid) {
  std::lock_guard<std::mutex> lock(mLock);
  auto uuidStr = uuid.ToString();

  auto iter = mObjectIDs.find(uuidStr);
  if (iter != mObjectIDs.end()) {
    return iter->second;
  }

  return -1;
}

const libobjgen::UUID ClientState::GetObjectUUID(int64_t objectID) {
  std::lock_guard<std::mutex> lock(mLock);
  auto iter = mObjectUUIDs.find(objectID);
  if (iter != mObjectUUIDs.end()) {
    return iter->second;
  }

  return NULLUUID;
}

int32_t ClientState::GetLocalObjectID(const libobjgen::UUID& uuid) {
  std::lock_guard<std::mutex> lock(mLock);
  auto uuidStr = uuid.ToString();

  auto iter = mLocalObjectIDs.find(uuidStr);
  if (iter != mLocalObjectIDs.end()) {
    return iter->second;
  }

  int32_t localID = mNextLocalObjectID++;
  mLocalObjectIDs[uuidStr] = localID;
  mLocalObjectUUIDs[localID] = uuid;

  return localID;
}

const libobjgen::UUID ClientState::GetLocalObjectUUID(int32_t objectID) {
  std::lock_guard<std::mutex> lock(mLock);
  auto iter = mLocalObjectUUIDs.find(objectID);
  if (iter != mLocalObjectUUIDs.end()) {
    return iter->second;
  }

  return NULLUUID;
}

bool ClientState::SetObjectID(const libobjgen::UUID& uuid, int64_t objectID,
                              bool allowReset) {
  auto uuidStr = uuid.ToString();

  std::lock_guard<std::mutex> lock(mLock);

  auto iter = mObjectIDs.find(uuidStr);
  if (iter != mObjectIDs.end()) {
    if (allowReset) {
      mObjectUUIDs.erase(iter->second);
      mObjectIDs.erase(iter);
    } else {
      return false;
    }
  }

  mObjectIDs[uuidStr] = objectID;
  mObjectUUIDs[objectID] = uuid;
  return true;
}

const libobjgen::UUID ClientState::GetAccountUID() const {
  return GetAccountLogin()->GetAccount().GetUUID();
}

int32_t ClientState::GetUserLevel() const {
  auto account = GetAccountLogin()->GetAccount().Get();
  return account ? account->GetUserLevel() : 0;
}

int32_t ClientState::GetWorldCID() const {
  return GetAccountLogin()->GetCharacterLogin()->GetWorldCID();
}

std::shared_ptr<Zone> ClientState::GetZone() const {
  return mCharacterState->Ready(true) ? mCharacterState->GetZone() : nullptr;
}

uint32_t ClientState::GetPartyID() const {
  return GetAccountLogin()->GetCharacterLogin()->GetPartyID();
}

int32_t ClientState::GetClanID() const {
  return GetAccountLogin()->GetCharacterLogin()->GetClanID();
}

int32_t ClientState::GetTeamID() const {
  return GetAccountLogin()->GetCharacterLogin()->GetTeamID();
}

std::shared_ptr<objects::PartyCharacter> ClientState::GetPartyCharacter(
    bool includeDemon) const {
  auto character = mCharacterState->GetEntity();
  auto cStats = character->GetCoreStats();

  auto member = std::make_shared<objects::PartyCharacter>();
  member->SetWorldCID(GetWorldCID());
  member->SetName(character->GetName());
  member->SetLevel((uint8_t)cStats->GetLevel());
  member->SetHP((uint16_t)cStats->GetHP());
  member->SetMaxHP((uint16_t)mCharacterState->GetMaxHP());
  member->SetMP((uint16_t)cStats->GetMP());
  member->SetMaxMP((uint16_t)mCharacterState->GetMaxMP());
  if (includeDemon) {
    member->SetDemon(GetPartyDemon());
  } else {
    member->SetDemon(nullptr);
  }

  return member;
}

std::shared_ptr<objects::PartyMember> ClientState::GetPartyDemon() const {
  auto demon = mDemonState->GetEntity();

  auto dMember = std::make_shared<objects::PartyMember>();
  if (demon) {
    auto dStats = demon->GetCoreStats();
    dMember->SetDemonType(demon->GetType());
    dMember->SetHP((uint16_t)dStats->GetHP());
    dMember->SetMaxHP((uint16_t)mDemonState->GetMaxHP());
    dMember->SetMP((uint16_t)dStats->GetMP());
    dMember->SetMaxMP((uint16_t)mDemonState->GetMaxMP());
  }
  return dMember;
}

void ClientState::GetPartyCharacterPacket(libcomp::Packet& p) const {
  p.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
  p.WriteS32Little(GetWorldCID());
  p.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO);
  GetPartyCharacter(false)->SavePacket(p, true);
}

void ClientState::GetPartyDemonPacket(libcomp::Packet& p) const {
  p.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
  p.WriteS32Little(GetWorldCID());
  p.WriteU8((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO);
  GetPartyDemon()->SavePacket(p, true);
}

ClientTime ClientState::ToClientTime(ServerTime time) const {
  if (time <= mStartTime) {
    return 0.0f;
  }

  return static_cast<ClientTime>(time - mStartTime) / 1000000.0f;
}

ServerTime ClientState::ToServerTime(ClientTime time) const {
  return static_cast<ServerTime>(time * 1000000.0f) + mStartTime;
}

bool ClientState::RewindStartTime(ServerTime time) {
  if (mStartTime > time) {
    mStartTime = time;
    return true;
  } else {
    return false;
  }
}

ClientState* ClientState::GetEntityClientState(int32_t id, bool worldID) {
  std::lock_guard<std::mutex> lock(sLock);
  std::unordered_map<int32_t, ClientState*>& m = sEntityClients[worldID];
  auto it = m.find(id);
  return it != m.end() ? it->second : nullptr;
}

std::list<std::shared_ptr<objects::ClientCostAdjustment>>
ClientState::SetCostAdjustments(
    int32_t entityID,
    std::list<std::shared_ptr<objects::ClientCostAdjustment>> adjustments) {
  std::list<std::shared_ptr<objects::ClientCostAdjustment>> updates;

  std::lock_guard<std::mutex> lock(mLock);

  // Store old and set new
  auto current = mCostAdjustments[entityID];
  if (adjustments.size() == 0) {
    // No new adjustments
    mCostAdjustments.erase(entityID);

    if (current.size() == 0) {
      // No updates
      return updates;
    }
  } else {
    // New adjustments exist
    mCostAdjustments[entityID] = adjustments;
  }

  // Compare and set updates
  for (auto adjust : adjustments) {
    std::shared_ptr<objects::ClientCostAdjustment> match;
    for (auto it = current.begin(); it != current.end(); it++) {
      if ((*it)->GetCategory() == adjust->GetCategory() &&
          (*it)->GetType() == adjust->GetType()) {
        match = *it;
        current.erase(it);
        break;
      }
    }

    if (match) {
      if (match->GetHPCost() != adjust->GetHPCost() ||
          match->GetMPCost() != adjust->GetMPCost()) {
        // Updated
        updates.push_back(adjust);
      }
    } else if (adjust->GetHPCost() != 100 || adjust->GetMPCost() != 100) {
      // Newly set
      updates.push_back(adjust);
    }
  }

  // Create removals from leftover changes
  for (auto c : current) {
    if (c->GetHPCost() != 100 || c->GetMPCost() != 100) {
      auto defaultCost = std::make_shared<objects::ClientCostAdjustment>();
      defaultCost->SetCategory(c->GetCategory());
      defaultCost->SetType(c->GetType());
      updates.push_back(defaultCost);
    }
  }

  return updates;
}

std::list<std::shared_ptr<objects::ClientCostAdjustment>>
ClientState::GetCostAdjustments(int32_t entityID) {
  std::lock_guard<std::mutex> lock(mLock);
  auto it = mCostAdjustments.find(entityID);
  return it != mCostAdjustments.end()
             ? it->second
             : std::list<std::shared_ptr<objects::ClientCostAdjustment>>();
}

std::shared_ptr<Prefecture> ClientState::GetPrefecture() const {
  return mPrefecture.lock();
}

void ClientState::SetPrefecture(const std::weak_ptr<Prefecture>& prefecture) {
  mPrefecture = prefecture;
}