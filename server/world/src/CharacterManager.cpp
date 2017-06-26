/**
 * @file server/world/src/CharacterManager.h
 * @ingroup world
 *
 * @author HACKfrost
 *
 * @brief Manager to handle world level character actions.
 *
 * This file is part of the World Server (world).
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

#include "CharacterManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <Character.h>
#include <FriendSettings.h>

// world Includes
#include "WorldServer.h"

using namespace world;

CharacterManager::CharacterManager(const std::weak_ptr<WorldServer>& server)
    : mServer(server)
{
    // By default create the pending party
    mParties[0] = std::make_shared<objects::Party>();

    mMaxCID = 0;
    mMaxPartyID = 0;
}

std::shared_ptr<objects::CharacterLogin> CharacterManager::RegisterCharacter(
    std::shared_ptr<objects::CharacterLogin> cLogin)
{
    libcomp::String lookup = cLogin->GetCharacter().GetUUID().ToString();

    std::lock_guard<std::mutex> lock(mLock);

    auto pair = mCharacterMap.find(lookup);
    if(pair != mCharacterMap.end())
    {
        cLogin = pair->second;
    }
    else
    {
        int32_t cid = ++mMaxCID;
        cLogin->SetWorldCID(cid);
        mCharacterMap[lookup] = cLogin;
        mCharacterCIDMap[cid] = cLogin;
    }

    return cLogin;
}

std::shared_ptr<objects::CharacterLogin> CharacterManager::GetCharacterLogin(
    const libobjgen::UUID& uuid)
{
    libcomp::String lookup = uuid.ToString();
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mCharacterMap.find(lookup);
        if (it != mCharacterMap.end())
        {
            return it->second;
        }
    }

    // Register a new character login
    auto cLogin = std::make_shared<objects::CharacterLogin>();
    cLogin->SetCharacter(uuid);
    cLogin = RegisterCharacter(cLogin);

    return cLogin;
}

std::shared_ptr<objects::CharacterLogin> CharacterManager::GetCharacterLogin(
    int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mCharacterCIDMap.find(worldCID);
    return it != mCharacterCIDMap.end() ? it->second : nullptr;
}

std::shared_ptr<objects::CharacterLogin> CharacterManager::GetCharacterLogin(
    const libcomp::String& characterName)
{
    auto worldDB = mServer.lock()->GetWorldDatabase();
    auto character = objects::Character::LoadCharacterByName(worldDB, characterName);

    return character ? GetCharacterLogin(character->GetUUID()) : nullptr;
}

bool CharacterManager::SendToCharacters(libcomp::Packet& p,
    const std::list<std::shared_ptr<objects::CharacterLogin>>& cLogins, bool appendCIDs)
{
    std::unordered_map<int8_t, std::list<int32_t>> channelMap;
    for(auto c : cLogins)
    {
        int8_t channelID = c->GetChannelID();
        if(channelID >= 0)
        {
            channelMap[channelID].push_back(c->GetWorldCID());
        }
    }

    auto server = mServer.lock();
    auto channels = server->GetChannels();
    for(auto pair : channelMap)
    {
        auto channel = server->GetChannelConnectionByID(pair.first);

        // If the channel is not valid, move on and clean it up later
        if(!channel) continue;

        // Make a copy and keep writing or send
        libcomp::Packet p2(p);
        if(appendCIDs)
        {
            // Write the list of CIDs to inform of the update
            p2.WriteU16Little((uint16_t)pair.second.size());
            for(int32_t fCID : pair.second)
            {
                p2.WriteS32Little(fCID);
            }
        }

        channel->SendPacket(p2);
    }

    return true;
}

bool CharacterManager::SendToRelatedCharacters(libcomp::Packet& p,
    int32_t worldCID, bool appendCIDs, bool friends, bool party, bool includeSelf,
    bool zoneRestrict)
{
    auto server = mServer.lock();
    auto cLogin = GetCharacterLogin(worldCID);
    if(!cLogin)
    {
        LOG_ERROR(libcomp::String("Invalid world CID encountered: %1\n")
            .Arg(worldCID));
        return false;
    }

    auto cLogins = GetRelatedCharacterLogins(cLogin, friends, party);
    if(zoneRestrict)
    {
        cLogins.remove_if([cLogin](const std::shared_ptr<objects::CharacterLogin>& l)
            {
                return l->GetZoneID() != cLogin->GetZoneID() ||
                    l->GetChannelID() != cLogin->GetChannelID();
            });
    }

    if(includeSelf)
    {
        cLogins.push_back(cLogin);
    }

    cLogins.unique();

    return cLogins.size() == 0 || SendToCharacters(p, cLogins, appendCIDs);
}

std::list<std::shared_ptr<objects::CharacterLogin>>
    CharacterManager::GetRelatedCharacterLogins(
    std::shared_ptr<objects::CharacterLogin> cLogin, bool friends, bool party)
{
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();

    std::list<int32_t> targetCIDs;
    std::list<libobjgen::UUID> targetUUIDs;
    if(friends)
    {
        auto fSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, cLogin->GetCharacter().GetUUID());
        for(auto f : fSettings->GetFriends())
        {
            targetUUIDs.push_back(f.GetUUID());
        }
    }

    if(party)
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mParties.find(cLogin->GetPartyID());
        if(it != mParties.end())
        {
            for(auto memberPair : it->second->GetMembers())
            {
                targetCIDs.push_back(memberPair.first);
            }
        }
    }

    std::list<std::shared_ptr<objects::CharacterLogin>> cLogins;
    for(auto targetUUID : targetUUIDs)
    {
        if(targetUUID != cLogin->GetCharacter().GetUUID())
        {
            cLogins.push_back(GetCharacterLogin(targetUUID));
        }
    }

    for(auto cid : targetCIDs)
    {
        if(cid != cLogin->GetWorldCID())
        {
            cLogins.push_back(GetCharacterLogin(cid));
        }
    }

    return cLogins;
}

void CharacterManager::SendStatusToRelatedCharacters(
    const std::list<std::shared_ptr<objects::CharacterLogin>>& cLogins, uint8_t updateFlags, bool zoneRestrict)
{
    for(auto cLogin : cLogins)
    {
        uint8_t outFlags = updateFlags;

        libcomp::Packet reply;
        if(GetStatusPacket(reply, cLogin, outFlags))
        {
            bool friendUpdate = 0 != (outFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_FLAGS);
            bool partyUpdate = 0 != (outFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_FLAGS);

            // If all that is being sent is zone visible stats, restrict to the same zone
            bool partyStatsOnly = zoneRestrict &&
                (0 == (outFlags & (uint8_t)(~((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO) |
                (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO)));
            SendToRelatedCharacters(reply, cLogin->GetWorldCID(), true, friendUpdate, partyUpdate, false,
                partyStatsOnly);
        }
    }
}

bool CharacterManager::GetStatusPacket(libcomp::Packet& p,
    const std::shared_ptr<objects::CharacterLogin>& cLogin, uint8_t& updateFlags)
{
    std::shared_ptr<objects::PartyCharacter> member;
    if(0 != (updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_FLAGS))
    {
        member = GetPartyMember(cLogin);
        if(!member)
        {
            // Drop the party flags
            updateFlags = updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_FLAGS;
        }
    }

    if(!updateFlags)
    {
        return false;
    }

    p.WritePacketCode(InternalPacketCode_t::PACKET_CHARACTER_LOGIN);
    p.WriteU8(updateFlags);
    cLogin->SavePacket(p, false);
        
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO)
    {
        member->SavePacket(p, true);
    }
        
    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO)
    {
        member->GetDemon()->SavePacket(p, true);
    }

    return true;
}

std::shared_ptr<objects::Party> CharacterManager::GetParty(uint32_t partyID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mParties.find(partyID);
    if(it != mParties.end())
    {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<objects::PartyCharacter>
    CharacterManager::GetPartyMember(std::shared_ptr<objects::CharacterLogin> cLogin)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mParties.find(cLogin->GetPartyID());
    if(it != mParties.end() && it->second->MembersKeyExists(cLogin->GetWorldCID()))
    {
        return it->second->GetMembers(cLogin->GetWorldCID());
    }

    return nullptr;
}

bool CharacterManager::AddToParty(std::shared_ptr<objects::PartyCharacter> member,
    uint32_t partyID)
{
    auto login = GetCharacterLogin(member->GetWorldCID());

    std::lock_guard<std::mutex> lock(mLock);
    auto it = mParties.find(partyID);
    if(it != mParties.end() && it->second->MembersCount() < 5
        && (login->GetPartyID() == 0 || login->GetPartyID() == partyID))
    {
        mParties[0]->RemoveMembers(login->GetWorldCID());
        login->SetPartyID(partyID);
        it->second->SetMembers(login->GetWorldCID(), member);
        return true;
    }

    return false;
}

bool CharacterManager::PartyJoin(std::shared_ptr<objects::PartyCharacter> member,
    const libcomp::String targetName, uint32_t partyID,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection)
{
    bool newParty = false;
    uint16_t responseCode = 201;  // Not available
    std::shared_ptr<objects::CharacterLogin> targetLogin;
    if(!targetName.IsEmpty())
    {
        // Request response
        targetLogin = GetCharacterLogin(targetName);
        if(targetLogin && targetLogin->GetChannelID() >= 0)
        {
            auto targetMember = GetPartyMember(targetLogin);
            if(targetMember)
            {
                if(partyID == 0)
                {
                    partyID = CreateParty(targetMember);
                    newParty = true;
                }
                else if(GetCharacterLogin(
                    targetMember->GetWorldCID())->GetPartyID() != partyID)
                {
                    responseCode = 202; // In a different party
                }

                if(responseCode != 202 && AddToParty(member, partyID))
                {
                    responseCode = 200; // Success
                }
            }
        }

        libcomp::Packet response;
        response.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_RESPONSE_YES);
        response.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
            targetName, true);
        response.WriteU16Little(responseCode);
        response.WriteS32Little(member->GetWorldCID());

        sourceConnection->QueuePacket(response);
    }
    else if(partyID)
    {
        // Rejoining from login
        if(AddToParty(member, partyID))
        {
            responseCode = 200; // Success
        }
    }

    if(responseCode == 200)
    {
        auto cLogin = GetCharacterLogin(member->GetWorldCID());
        auto party = GetParty(partyID);
        auto partyMembers = party->GetMembers();

        // All members
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
        request.WriteU32Little(partyID);
        request.WriteU8((uint8_t)partyMembers.size());
        for(auto memberPair : partyMembers)
        {
            auto login = GetCharacterLogin(memberPair.first);
            memberPair.second->SavePacket(request, false);
            request.WriteU32Little(login->GetZoneID());
            request.WriteU8(party->GetLeaderCID() == memberPair.first ? 1 : 0);
        }

        if(newParty)
        {
            // Send everyone to everyone
            SendToRelatedCharacters(request, member->GetWorldCID(),
                true, false, true, true);
        }
        else
        {
            // Send everyone to the new member
            request.WriteS32Little(member->GetWorldCID());
            sourceConnection->QueuePacket(request);

            // Send new member to everyone else
            request.Clear();
            request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
            request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
            request.WriteU32Little(partyID);
            request.WriteU8(1);
            member->SavePacket(request, false);
            request.WriteU32Little(cLogin->GetZoneID());
            request.WriteU8(0);

            SendToRelatedCharacters(request, member->GetWorldCID(),
                true, false, true);
        }

        request.Clear();
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_DROP_RULE);
        request.WriteU8(0); // Not a response
        request.WriteU8(party->GetDropRule());

        // Send to everyone if the party is new
        SendToRelatedCharacters(request, member->GetWorldCID(),
            true, false, newParty, true);
    }

    sourceConnection->FlushOutgoing();

    return responseCode == 200;
}

void CharacterManager::PartyLeave(std::shared_ptr<objects::CharacterLogin> cLogin,
    std::shared_ptr<libcomp::TcpConnection> requestConnection, bool tempLeave)
{
    uint32_t partyID = cLogin->GetPartyID();
    auto party = GetParty(partyID);
    auto partyLogins = GetRelatedCharacterLogins(cLogin, false, true);

    uint16_t responseCode = 201;    // Failure
    if(RemoveFromParty(cLogin))
    {
        responseCode = 200; // Success
        if(!tempLeave)
        {
            cLogin->SetPartyID(0);
        }
    }

    if(requestConnection)
    {
        libcomp::Packet response;
        response.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_LEAVE);
        response.WriteU8(1); // Is a response
        response.WriteU16Little(responseCode);
        response.WriteS32Little(cLogin->GetWorldCID());

        requestConnection->QueuePacket(response);
    }

    if(responseCode == 200)
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_LEAVE);
        request.WriteU8(0); // Not a response
        request.WriteS32Little(cLogin->GetWorldCID());

        partyLogins.push_back(cLogin);
        SendToCharacters(request, partyLogins, true);

        auto members = party->GetMembers();
        if(members.size() <= 1)
        {
            // Cannot exist with one or zero members
            PartyDisband(partyID, cLogin->GetWorldCID());
        }
        else if(cLogin->GetWorldCID() == party->GetLeaderCID())
        {
            // If the leader left, take the next person who joined
            PartyLeaderUpdate(party->GetID(), cLogin->GetWorldCID(),
                nullptr, members.begin()->first);
        }
    }
    
    if(requestConnection)
    {
        requestConnection->FlushOutgoing();
    }
}

void CharacterManager::PartyDisband(uint32_t partyID, int32_t sourceCID,
    std::shared_ptr<libcomp::TcpConnection> requestConnection)
{
    auto party = GetParty(partyID);

    uint16_t responseCode = 200;    // Success
    std::list<std::shared_ptr<objects::CharacterLogin>> partyLogins;
    for(auto memberPair : party->GetMembers())
    {
        auto login = GetCharacterLogin(memberPair.first);
        if(login)
        {
            partyLogins.push_back(login);

            if(RemoveFromParty(login))
            {
                login->SetPartyID(0);
            }
            else
            {
                responseCode = 201; // Failure
                break;
            }
        }
    }

    if(requestConnection)
    {
        libcomp::Packet response;
        response.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_DISBAND);
        response.WriteU8(1); // Is a response
        response.WriteU16Little(responseCode);
        response.WriteS32Little(sourceCID);

        requestConnection->QueuePacket(response);
    }

    if(responseCode == 200)
    {
        {
            std::lock_guard<std::mutex> lock(mLock);
            mParties.erase(party->GetID());
        }

        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_DISBAND);
        request.WriteU8(0); // Not a response

        SendToCharacters(request, partyLogins, true);
    }

    if(requestConnection)
    {
        requestConnection->FlushOutgoing();
    }
}

void CharacterManager::PartyLeaderUpdate(uint32_t partyID, int32_t sourceCID,
    std::shared_ptr<libcomp::TcpConnection> requestConnection, int32_t targetCID)
{
    auto party = GetParty(partyID);

    uint16_t responseCode = 201;    // Failure
    if(party->MembersKeyExists(targetCID))
    {
        party->SetLeaderCID(targetCID);
        responseCode = 200; // Success
    }

    if(requestConnection)
    {
        libcomp::Packet response;
        response.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_LEADER_UPDATE);
        response.WriteU8(1); // Is a response
        response.WriteU16Little(responseCode);
        response.WriteS32Little(sourceCID);

        requestConnection->QueuePacket(response);
    }

    if(responseCode == 200)
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_LEADER_UPDATE);
        request.WriteU8(0); // Not a response
        request.WriteS32Little(targetCID);

        std::list<std::shared_ptr<objects::CharacterLogin>> partyLogins;
        for(auto pair : party->GetMembers())
        {
            partyLogins.push_back(GetCharacterLogin(pair.first));
        }

        SendToCharacters(request, partyLogins, true);
    }
    
    if(requestConnection)
    {
        requestConnection->FlushOutgoing();
    }
}

void CharacterManager::PartyKick(std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t targetCID)
{
    auto party = GetParty(cLogin->GetPartyID());
    if(!party)
    {
        return;
    }

    auto partyLogins = GetRelatedCharacterLogins(cLogin, false, true);
    if(party->MembersKeyExists(targetCID))
    {
        party->RemoveMembers(targetCID);
    }

    auto targetLogin = GetCharacterLogin(targetCID);
    if(targetLogin)
    {
        targetLogin->SetPartyID(0);
    }

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
    request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_KICK);
    request.WriteS32Little(targetCID);

    partyLogins.push_back(cLogin);
    SendToCharacters(request, partyLogins, true);

    auto members = party->GetMembers();
    if(members.size() <= 1)
    {
        PartyDisband(party->GetID());
    }
}

uint32_t CharacterManager::CreateParty(std::shared_ptr<objects::PartyCharacter> member)
{
    auto login = GetCharacterLogin(member->GetWorldCID());

    std::lock_guard<std::mutex> lock(mLock);
    uint32_t partyID = login->GetPartyID();
    if(!partyID)
    {
        mParties[0]->RemoveMembers(login->GetWorldCID());
        partyID = ++mMaxPartyID;
        login->SetPartyID(partyID);

        auto party = std::make_shared<objects::Party>();
        party->SetID(partyID);
        party->SetLeaderCID(login->GetWorldCID());
        party->SetMembers(login->GetWorldCID(), member);
        mParties[partyID] = party;
    }

    return partyID;
}

bool CharacterManager::RemoveFromParty(std::shared_ptr<objects::CharacterLogin> cLogin)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mParties.find(cLogin->GetPartyID());
    if(it != mParties.end() && it->second->MembersKeyExists(cLogin->GetWorldCID()))
    {
        it->second->RemoveMembers(cLogin->GetWorldCID());
        return true;
    }

    return false;
}
