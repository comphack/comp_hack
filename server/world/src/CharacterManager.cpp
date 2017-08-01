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
#include <ClanMember.h>
#include <EntityStats.h>
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
    mMaxClanID = 0;
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
    const std::list<std::shared_ptr<objects::CharacterLogin>>& cLogins, uint32_t cidOffset)
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

    if(cidOffset > (p.Size() - 2))
    {
        cidOffset = (p.Size() - 2);
    }

    auto server = mServer.lock();
    auto channels = server->GetChannels();
    for(auto pair : channelMap)
    {
        auto channel = server->GetChannelConnectionByID(pair.first);

        // If the channel is not valid, move on and clean it up later
        if(!channel) continue;

        libcomp::Packet p2(p);
        ConvertToTargetCIDPacket(p2, cidOffset, pair.second.size());
        for(int32_t fCID : pair.second)
        {
            p2.WriteS32Little(fCID);
        }

        channel->SendPacket(p2);
    }

    return true;
}

void CharacterManager::ConvertToTargetCIDPacket(libcomp::Packet& p, uint32_t cidOffset,
    size_t cidCount)
{
    cidOffset = (uint32_t)(cidOffset + 2);

    p.Seek(cidOffset);
    auto afterData = p.ReadArray(p.Left());
    p.Seek(cidOffset);

    p.WriteU16Little((uint16_t)cidCount);
    p.WriteBlank((uint32_t)(cidCount * 4));
    p.WriteArray(afterData);

    // Seek to the first CID position
    p.Seek(cidOffset + 2);
}

bool CharacterManager::SendToRelatedCharacters(libcomp::Packet& p,
    int32_t worldCID, uint32_t cidOffset, uint8_t relatedTypes,
    bool includeSelf, bool zoneRestrict)
{
    auto server = mServer.lock();
    auto cLogin = GetCharacterLogin(worldCID);
    if(!cLogin)
    {
        LOG_ERROR(libcomp::String("Invalid world CID encountered: %1\n")
            .Arg(worldCID));
        return false;
    }

    auto cLogins = GetRelatedCharacterLogins(cLogin, relatedTypes);
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

    return cLogins.size() == 0 || SendToCharacters(p, cLogins, cidOffset);
}

std::list<std::shared_ptr<objects::CharacterLogin>>
    CharacterManager::GetRelatedCharacterLogins(
    std::shared_ptr<objects::CharacterLogin> cLogin, uint8_t relatedTypes)
{
    auto server = mServer.lock();
    auto worldDB = server->GetWorldDatabase();

    std::list<int32_t> targetCIDs;
    std::list<libobjgen::UUID> targetUUIDs;
    if(relatedTypes & RELATED_FRIENDS)
    {
        auto fSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
            worldDB, cLogin->GetCharacter().GetUUID());
        for(auto f : fSettings->GetFriends())
        {
            targetUUIDs.push_back(f.GetUUID());
        }
    }

    if(relatedTypes & RELATED_CLAN)
    {
        auto clanInfo = GetClan(cLogin->GetClanID());
        if(clanInfo)
        {
            for(auto mPair : clanInfo->GetMemberMap())
            {
                targetCIDs.push_back(mPair.first);
            }
        }
    }

    if(relatedTypes & RELATED_PARTY)
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
            bool clanUpdate = 0 != (outFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC);
            bool friendUpdate = 0 != (outFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_FLAGS);
            bool partyUpdate = 0 != (outFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_FLAGS);
            uint8_t relatedTypes = (uint8_t)((clanUpdate ? RELATED_CLAN : 0) |
                (friendUpdate ? RELATED_FRIENDS : 0) | (partyUpdate ? RELATED_PARTY : 0));

            // If all that is being sent is zone visible stats, restrict to the same zone
            bool partyStatsOnly = zoneRestrict &&
                (0 == (outFlags & (uint8_t)(~((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO) |
                (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO)));
            SendToRelatedCharacters(reply, cLogin->GetWorldCID(), 1, relatedTypes, false, partyStatsOnly);
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
            updateFlags = updateFlags & ((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_FLAGS
                | (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC);
        }

        if(!cLogin->GetClanID())
        {
            // Drop the clan flags
            updateFlags = updateFlags & ((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_FRIEND_FLAGS
                | (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_FLAGS);
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

    if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC)
    {
        p.WriteS32Little(cLogin->GetClanID());
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
        response.WriteU16Little(1); // CID Count
        response.WriteS32Little(member->GetWorldCID());
        response.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
            targetName, true);
        response.WriteU16Little(responseCode);

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
                1, RELATED_PARTY, true);
        }
        else
        {
            // Send everyone to the new member
            ConvertToTargetCIDPacket(request, 1, 1);
            request.WriteS32Little(member->GetWorldCID());
            sourceConnection->SendPacket(request);

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
                1, false, false, true);
        }

        request.Clear();
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_PARTY_DROP_RULE);
        request.WriteU8(0); // Not a response
        request.WriteU8(party->GetDropRule());

        // Send to everyone if the party is new
        SendToRelatedCharacters(request, member->GetWorldCID(),
            1, newParty ? RELATED_PARTY : 0, true);
    }

    sourceConnection->FlushOutgoing();

    return responseCode == 200;
}

void CharacterManager::PartyLeave(std::shared_ptr<objects::CharacterLogin> cLogin,
    std::shared_ptr<libcomp::TcpConnection> requestConnection, bool tempLeave)
{
    uint32_t partyID = cLogin->GetPartyID();
    auto party = GetParty(partyID);
    auto partyLogins = GetRelatedCharacterLogins(cLogin, RELATED_PARTY);

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
        response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE);
        response.WriteU16Little(1); // CID Count
        response.WriteS32Little(cLogin->GetWorldCID());
        response.WriteU8(1); // Is a response
        response.WriteU16Little(responseCode);

        requestConnection->QueuePacket(response);
    }

    if(responseCode == 200)
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE);
        request.WriteU8(0); // Not a response
        request.WriteS32Little(cLogin->GetWorldCID());

        partyLogins.push_back(cLogin);
        SendToCharacters(request, partyLogins, 1);

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
        response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_DISBAND);
        response.WriteU16Little(1); // CID Count
        response.WriteS32Little(sourceCID);
        response.WriteU8(1); // Is a response
        response.WriteU16Little(responseCode);

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
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_DISBAND);
        request.WriteU8(0); // Not a response

        SendToCharacters(request, partyLogins, 1);
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
        response.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE);
        response.WriteU16Little(1); // CID Count
        response.WriteS32Little(sourceCID);
        response.WriteU8(1); // Is a response
        response.WriteU16Little(responseCode);

        requestConnection->QueuePacket(response);
    }

    if(responseCode == 200)
    {
        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE);
        request.WriteU8(0); // Not a response
        request.WriteS32Little(targetCID);

        std::list<std::shared_ptr<objects::CharacterLogin>> partyLogins;
        for(auto pair : party->GetMembers())
        {
            partyLogins.push_back(GetCharacterLogin(pair.first));
        }

        SendToCharacters(request, partyLogins, 1);
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

    auto partyLogins = GetRelatedCharacterLogins(cLogin, RELATED_PARTY);
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
    request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_KICK);
    request.WriteS32Little(targetCID);

    partyLogins.push_back(cLogin);
    SendToCharacters(request, partyLogins, 1);

    auto members = party->GetMembers();
    if(members.size() <= 1)
    {
        PartyDisband(party->GetID());
    }
}

std::shared_ptr<objects::ClanInfo> CharacterManager::GetClan(int32_t clanID)
{
    if(clanID == 0)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mLock);

    auto it = mClans.find(clanID);
    return it != mClans.end() ? it->second : nullptr;
}

std::shared_ptr<objects::ClanInfo> CharacterManager::GetClan(const libobjgen::UUID& uuid)
{
    // Attempt to load existing first
    int32_t clanID = 0;
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mClanMap.find(uuid.ToString());
        if(it != mClanMap.end())
        {
            clanID = it->second;
        }
    }

    auto clanInfo = GetClan(clanID);
    if(clanInfo)
    {
        return clanInfo;
    }

    // Both the clan and members should have been loaded already, do not load them
    // if they haven't been
    auto clan = std::dynamic_pointer_cast<objects::Clan>(
        libcomp::PersistentObject::GetObjectByUUID(uuid));
    if(clan)
    {
        clanInfo = std::make_shared<objects::ClanInfo>();
        {
            std::lock_guard<std::mutex> lock(mLock);
            clanID = ++mMaxClanID;
        }

        // Load the members and ensure all characters in the clan have
        // a world CID
        for(auto member : clan->GetMembers())
        {
            if(member.Get())
            {
                auto character = member->GetCharacter();

                auto cLogin = std::make_shared<objects::CharacterLogin>();
                cLogin->SetCharacter(character);
                cLogin->SetClanID(clanID);
                cLogin = RegisterCharacter(cLogin);
                clanInfo->SetMemberMap(cLogin->GetWorldCID(), member);
            }
        }

        std::lock_guard<std::mutex> lock(mLock);
        clanInfo->SetID(clanID);
        clanInfo->SetClan(clan);

        mClans[clanID] = clanInfo;
        mClanMap[clan->GetUUID().ToString()] = clanID;
    }

    return clanInfo;
}

bool CharacterManager::ClanJoin(std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t clanID)
{
    // No failure codes for this, either it works or nothing happens

    std::shared_ptr<objects::ClanInfo> clanInfo = GetClan(clanID);
    if(clanInfo && !cLogin->GetClanID())
    {
        std::lock_guard<std::mutex> lock(mLock);
        if(clanInfo->MemberMapCount() >= MAX_CLAN_COUNT)
        {
            // Not enough space
            return false;
        }

        if(clanInfo->MemberMapKeyExists(cLogin->GetWorldCID()))
        {
            // Already joined
            return true;
        }
    }
    else
    {
        // Not a valid clan or already in one
        return false;
    }

    // Request is valid
    auto server = mServer.lock();
    auto db = server->GetWorldDatabase();

    // Reload the character
    auto character = libcomp::PersistentObject::LoadObjectByUUID<
        objects::Character>(db, cLogin->GetCharacter().GetUUID(), true);

    auto clan = clanInfo->GetClan().Get();

    auto newMember = libcomp::PersistentObject::New<objects::ClanMember>(true);
    newMember->SetClan(clan);
    newMember->SetMemberType(objects::ClanMember::MemberType_t::NORMAL);
    newMember->SetCharacter(character);

    clan->AppendMembers(newMember);
    clanInfo->SetMemberMap(cLogin->GetWorldCID(), newMember);
    cLogin->SetClanID(clanID);

    character->SetClan(clan);

    auto dbChanges = libcomp::DatabaseChangeSet::Create();
    dbChanges->Insert(newMember);
    dbChanges->Update(clan);
    dbChanges->Update(character);

    if(!db->ProcessChangeSet(dbChanges))
    {
        character->SetClan(NULLUUID);
        return false;
    }

    // Follow up with the the source so they can update the locally set clan
    // and update other players in the zone with the new info
    std::list<int32_t> cids = { cLogin->GetWorldCID() };
    SendClanInfo(clanInfo->GetID(), 0x0F, cids);

    // Tell everyone in the clan, including the character who just joined
    // that the join has happened
    libcomp::Packet relay;
    auto cidOffset = server->GetRelayPacket(relay);
    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_JOIN);
    relay.WriteS32Little(clanInfo->GetID());
    relay.WriteS32Little(cLogin->GetWorldCID());
    relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
        cLogin->GetCharacter()->GetName(), true);
    relay.WriteS8((int8_t)cLogin->GetStatus());
    relay.WriteU32Little(cLogin->GetZoneID());
    relay.WriteS8(cLogin->GetChannelID());

    SendToRelatedCharacters(relay, cLogin->GetWorldCID(), cidOffset, RELATED_CLAN, true);

    SendClanMemberInfo(cLogin, 0x30);
    RecalculateClanLevel(clanID);
    SendClanMemberInfo(cLogin, (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_BASIC);

    return true;
}

void CharacterManager::ClanLeave(std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t clanID, std::shared_ptr<libcomp::TcpConnection> requestConnection)
{
    auto clanLogins = GetRelatedCharacterLogins(cLogin, RELATED_CLAN);
    clanLogins.push_back(cLogin);

    if(requestConnection)
    {
        libcomp::Packet relay;
        mServer.lock()->GetRelayPacket(relay, cLogin->GetWorldCID());
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_LEAVE);
        relay.WriteS8(0);   // Response code doesn't seem to matter

        requestConnection->SendPacket(relay);
    }

    auto clanInfo = GetClan(clanID);
    auto member = clanInfo->GetMemberMap(cLogin->GetWorldCID()).Get();
    if(RemoveFromClan(cLogin, clanID))
    {
        libcomp::Packet relay;
        auto cidOffset = mServer.lock()->GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_LEFT);
        relay.WriteS32Little(clanID);
        relay.WriteS32Little(cLogin->GetWorldCID());

        SendToCharacters(relay, clanLogins, cidOffset);
        RecalculateClanLevel(clanID);

        std::list<int32_t> cids = { cLogin->GetWorldCID() };
        SendClanInfo(0, 0x0F, cids);
        
        if(member->GetMemberType() == objects::ClanMember::MemberType_t::MASTER)
        {
            // Need to set the new master
            std::shared_ptr<objects::ClanMember> newMaster;
            for(auto m : clanInfo->GetClan()->GetMembers())
            {
                // First sub-master else first member
                if(m->GetMemberType() ==
                    objects::ClanMember::MemberType_t::SUB_MASTER)
                {
                    newMaster = m.Get();
                    break;
                }
                else if(newMaster == nullptr)
                {
                    newMaster = m.Get();
                }
            }

            if(newMaster)
            {
                auto newMasterLogin = GetCharacterLogin(newMaster->GetCharacter().GetUUID());

                auto server = mServer.lock();
                auto worldDB = server->GetWorldDatabase();
                newMaster->SetMemberType(objects::ClanMember::MemberType_t::MASTER);
                newMaster->Update(worldDB);

                relay.Clear();
                cidOffset = server->GetRelayPacket(relay);
                relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_MASTER_UPDATED);
                relay.WriteS32Little(clanID);
                relay.WriteS32Little(newMasterLogin->GetWorldCID());

                SendToRelatedCharacters(relay, newMasterLogin->GetWorldCID(), cidOffset,
                    RELATED_CLAN, true);
            }
        }
    }
}

void CharacterManager::ClanDisband(int32_t clanID, int32_t sourceCID,
    std::shared_ptr<libcomp::TcpConnection> requestConnection)
{
    auto clanInfo = GetClan(clanID);

    int8_t responseCode = 0;    // Success
    std::list<int32_t> clanCIDs;
    for(auto mPair : clanInfo->GetMemberMap())
    {
        clanCIDs.push_back(mPair.first);
    }

    std::list<std::shared_ptr<objects::CharacterLogin>> clanLogins;
    if(requestConnection)
    {
        // If the disband request came from a player (instead of being a side-effect
        // from a leave for example) check that they are the clan master
        auto source = GetCharacterLogin(sourceCID);
        auto souceMember = source ? clanInfo->GetMemberMap(sourceCID).Get() : nullptr;
        if(!souceMember || souceMember->GetMemberType() !=
            objects::ClanMember::MemberType_t::MASTER)
        {
            responseCode = 1; // Failure
        }
    }

    if(responseCode == 0)
    {
        for(auto memberID : clanCIDs)
        {
            auto login = GetCharacterLogin(memberID);
            if(login)
            {
                clanLogins.push_back(login);
                login->SetClanID(0);
            }
        }
    }

    auto server = mServer.lock();
    if(requestConnection)
    {
        libcomp::Packet relay;
        server->GetRelayPacket(relay, sourceCID);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_DISBAND);
        relay.WriteS32Little(clanID);
        relay.WriteS8(responseCode);

        requestConnection->QueuePacket(relay);
    }

    if(responseCode == 0)
    {
        {
            std::lock_guard<std::mutex> lock(mLock);
            mClans.erase(clanID);
            mClanMap.erase(clanInfo->GetClan().GetUUID().ToString());
        }

        // Reload and update all member characters, then delete all
        // clan records
        auto worldDB = server->GetWorldDatabase();
        auto dbChanges = libcomp::DatabaseChangeSet::Create();
        auto clan = clanInfo->GetClan().Get();
        for(auto member : clan->GetMembers())
        {
            auto character = libcomp::PersistentObject::LoadObjectByUUID<
                objects::Character>(worldDB, member->GetCharacter().GetUUID(), true);
            character->SetClan(NULLUUID);
            dbChanges->Update(character);
            dbChanges->Delete(member.Get());
        }
        dbChanges->Delete(clan);

        if(!worldDB->ProcessChangeSet(dbChanges))
        {
            // This could get very messy, kill the server
            mServer.lock()->Shutdown();
            return;
        }

        libcomp::Packet relay;
        auto cidOffset = server->GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_DISBANDED);
        relay.WriteS32Little(clanID);

        SendToCharacters(relay, clanLogins, cidOffset);

        SendClanInfo(0, 0x0F, clanCIDs);
    }

    if(requestConnection)
    {
        requestConnection->FlushOutgoing();
    }
}

void CharacterManager::ClanKick(std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t clanID, int32_t targetCID, std::shared_ptr<libcomp::TcpConnection> requestConnection)
{
    auto server = mServer.lock();

    if(requestConnection)
    {
        libcomp::Packet relay;
        server->GetRelayPacket(relay, cLogin->GetWorldCID());
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_KICK);
        relay.WriteS32Little(clanID);
        relay.WriteS8(0);   // Response code doesn't seem to matter

        requestConnection->QueuePacket(relay);
    }

    auto targetLogin = GetCharacterLogin(targetCID);
    auto clanLogins = GetRelatedCharacterLogins(targetLogin, RELATED_CLAN);
    clanLogins.push_back(targetLogin);
    if(targetLogin && RemoveFromClan(targetLogin, clanID))
    {
        libcomp::Packet relay;
        auto cidOffset = server->GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_KICKED);
        relay.WriteS32Little(clanID);
        relay.WriteS32Little(targetLogin->GetWorldCID());

        SendToCharacters(relay, clanLogins, cidOffset);

        std::list<int32_t> cids = { targetCID };
        SendClanInfo(0, 0x0F, cids);
    }

    if(requestConnection)
    {
        requestConnection->FlushOutgoing();
    }
}

void CharacterManager::RecalculateClanLevel(int32_t clanID, bool sendUpdate)
{
    auto clanInfo = GetClan(clanID);
    if(clanInfo)
    {
        auto server = mServer.lock();
        auto db = server->GetWorldDatabase();
        auto clan = clanInfo->GetClan();

        int8_t currentLevel = clan->GetLevel();

        uint64_t totalPoints = 0;
        for(auto member : clan->GetMembers())
        {
            totalPoints = (uint64_t)(totalPoints + (uint64_t)member->LoadCharacter(db)
                ->GetLoginPoints());
        }

        uint32_t newLevel = (uint32_t)((double)totalPoints * 0.0001);
        if(newLevel > 10)
        {
            newLevel = 10;
        }
        else if(newLevel == 0)
        {
            newLevel = 1;
        }

        if(currentLevel != (int8_t)newLevel)
        {
            clan->SetLevel((int8_t)newLevel);
            clan->Update(db);

            if(sendUpdate)
            {
                SendClanInfo(clanID, 0x04);
            }
        }
    }
}

void CharacterManager::SendClanDetails(std::shared_ptr<objects::CharacterLogin> cLogin,
    std::shared_ptr<libcomp::TcpConnection> requestConnection, std::list<int32_t> memberIDs)
{
    auto clanInfo = GetClan(cLogin->GetClanID());
    auto server = mServer.lock();

    libcomp::Packet relay;
    server->GetRelayPacket(relay, cLogin->GetWorldCID());
    if(memberIDs.size() > 0)
    {
        // Member level info
        if(!clanInfo)
        {
            // Nothing to send
            return;
        }

        auto clan = clanInfo->GetClan().Get();
        auto worldDB = server->GetWorldDatabase();

        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_LIST);
        relay.WriteS32Little(clanInfo->GetID());
        relay.WriteS8((int8_t)clanInfo->MemberMapCount());
        for(auto mPair : clanInfo->GetMemberMap())
        {
            relay.WriteS32Little(mPair.first);

            auto member = mPair.second;
            auto memberLogin = GetCharacterLogin(mPair.first);
            auto memberChar = memberLogin->LoadCharacter(worldDB);
            auto stats = memberChar
                ? memberChar->LoadCoreStats(worldDB) : nullptr;

            relay.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                memberChar ? memberChar->GetName() : "", true);
            relay.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                member->GetClanMessage(), true);
            relay.WriteU8((uint8_t)member->GetMemberType());
            relay.WriteU8(1);   // Always 1
            relay.WriteS8((int8_t)memberLogin->GetStatus());
            relay.WriteU8(memberLogin->GetWorldCID() == cLogin->GetWorldCID()
                ? 1 : 0);
            relay.WriteS8(memberLogin->GetChannelID());
            relay.WriteS32Little(memberLogin->GetZoneID()
                ? (int32_t)memberLogin->GetZoneID() : -1);
            relay.WriteS32Little((int32_t)memberChar->GetLastLogin());
            relay.WriteS8(stats ? stats->GetLevel() : 0);
            relay.WriteS32Little(memberChar->GetLoginPoints());
        }
    }
    else
    {
        // Clan level info
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_INFO);
        if(clanInfo)
        {
            auto clan = clanInfo->GetClan().Get();

            relay.WriteS32Little(clanInfo->GetID());
            relay.WriteString16Little(libcomp::Convert::ENCODING_CP932,
                clan->GetName(), true);
            relay.WriteS32Little((int32_t)clan->GetBaseZoneID());

            relay.WriteS8((int8_t)clanInfo->MemberMapCount());
            for(auto mPair : clanInfo->GetMemberMap())
            {
                relay.WriteS32Little(mPair.first);
            }

            relay.WriteS8(clan->GetLevel());
            relay.WriteU8(clan->GetEmblemBase());
            relay.WriteU8(clan->GetEmblemSymbol());

            relay.WriteU8(clan->GetEmblemColorR1());
            relay.WriteU8(clan->GetEmblemColorG1());
            relay.WriteU8(clan->GetEmblemColorB1());

            relay.WriteU8(clan->GetEmblemColorR2());
            relay.WriteU8(clan->GetEmblemColorG2());
            relay.WriteU8(clan->GetEmblemColorB2());

            /// @todo: determine how we should actually receive emblem patterns
            relay.WriteU16Little(32);
            relay.WriteS64Little(-1);
            relay.WriteS64Little(-1);
            relay.WriteS64Little(-1);
            relay.WriteS64Little(-1);
        }
        else
        {
            relay.WriteS32Little(-1);
            relay.WriteBlank(18);
        }
    }

    requestConnection->SendPacket(relay);
}

void CharacterManager::SendClanInfo(int32_t clanID, uint8_t updateFlags,
    const std::list<int32_t>& cids)
{
    auto clanInfo = GetClan(clanID);
    auto clan = clanInfo ? clanInfo->GetClan().Get() : nullptr;
    std::list<int32_t> cidList = cids;
    if(cids.size() == 0)
    {
        for(auto mPair : clanInfo->GetMemberMap())
        {
            cidList.push_back(mPair.first);
        }
    }

    std::list<std::shared_ptr<objects::CharacterLogin>> cLogins;
    for(auto cid : cidList)
    {
        auto cLogin = GetCharacterLogin(cid);
        if(cLogin)
        {
            cLogins.push_back(cLogin);
        }
    }

    if(cLogins.size() == 0)
    {
        return;
    }

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_CLAN_UPDATE);
    request.WriteU8((int8_t)InternalPacketAction_t::PACKET_ACTION_UPDATE);
    request.WriteU8(updateFlags);

    // Always send the clan UUID to reload
    libobjgen::UUID uid = clan ? clan->GetUUID() : NULLUUID;
    request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
        uid.ToString(), true);

    if(updateFlags & 0x01)
    {
        // Name
        request.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8,
            clan ? clan->GetName() : "", true);
    }

    if(updateFlags & 0x02)
    {
        // Emblem
        if(clan)
        {
            request.WriteU8(clan->GetEmblemBase());
            request.WriteU8(clan->GetEmblemSymbol());
            request.WriteU8(clan->GetEmblemColorR1());
            request.WriteU8(clan->GetEmblemColorG1());
            request.WriteU8(clan->GetEmblemColorB1());
            request.WriteU8(clan->GetEmblemColorR2());
            request.WriteU8(clan->GetEmblemColorG2());
            request.WriteU8(clan->GetEmblemColorB2());
        }
        else
        {
            request.WriteBlank(8);
        }
    }

    if(updateFlags & 0x04)
    {
        // Level
        request.WriteS8(clan ? clan->GetLevel() : 0);
    }

    if(updateFlags & 0x08)
    {
        // New ID
        request.WriteS32Little(clanID);
    }

    SendToCharacters(request, cLogins, 1);
}

void CharacterManager::SendClanMemberInfo(std::shared_ptr<objects::CharacterLogin> cLogin,
    uint8_t updateFlags)
{
    auto clanInfo = GetClan(cLogin->GetClanID());
    auto clan = clanInfo->GetClan().Get();
    auto member = clanInfo->GetMemberMap(cLogin->GetWorldCID()).Get();

    if(member)
    {
        auto server = mServer.lock();
        libcomp::Packet relay;
        auto cidOffset = server->GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_DATA);
        relay.WriteS32Little(clanInfo->GetID());
        relay.WriteS32Little(cLogin->GetWorldCID());
        relay.WriteS8((int8_t)updateFlags);
        
        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_STATUS)
        {
            relay.WriteS8((int8_t)cLogin->GetStatus());
        }
        
        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE)
        {
            relay.WriteS32Little(cLogin->GetZoneID() ? (int32_t)cLogin->GetZoneID() : -1);
        }
        
        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_CHANNEL)
        {
            relay.WriteS8(cLogin->GetChannelID() ? cLogin->GetChannelID() : -1);
        }

        if(updateFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_MESSAGE)
        {
            relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                member->GetClanMessage(), true);
        }

        if(updateFlags & 0x10)
        {
            // Points
            relay.WriteS32Little(cLogin->GetCharacter()->GetLoginPoints());
        }

        if(updateFlags & 0x20)
        {
            // Level
            auto worldDB = server->GetWorldDatabase();
            relay.WriteS8(cLogin->GetCharacter()->LoadCoreStats(worldDB)->GetLevel());
        }

        SendToRelatedCharacters(relay, cLogin->GetWorldCID(), cidOffset,
            RELATED_CLAN, true);
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

bool CharacterManager::RemoveFromClan(std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t clanID)
{
    auto clanInfo = GetClan(clanID);
    std::lock_guard<std::mutex> lock(mLock);
    if(clanInfo && cLogin->GetClanID() == clanID)
    {
        cLogin->SetClanID(0);
        auto clan = clanInfo->GetClan().Get();
        clanInfo->RemoveMemberMap(cLogin->GetWorldCID());

        size_t idx = 0;
        std::shared_ptr<objects::ClanMember> member;
        for(auto m : clan->GetMembers())
        {
            if(m->GetCharacter().GetUUID() == cLogin->GetCharacter().GetUUID())
            {
                member = m.Get();
                clan->RemoveMembers(idx);
                break;
            }
            idx++;
        }

        if(member)
        {
            auto server = mServer.lock();
            auto worldDB = server->GetWorldDatabase();
            auto character = cLogin->LoadCharacter(worldDB);
            character->SetClan(NULLUUID);

            auto dbChanges = libcomp::DatabaseChangeSet::Create();
            dbChanges->Update(clan);
            dbChanges->Update(character);
            dbChanges->Delete(member);

            return worldDB->ProcessChangeSet(dbChanges);
        }
    }

    return false;
}
