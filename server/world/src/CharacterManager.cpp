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

#include "CharacterManager.h"

// libcomp Includes
#include <Constants.h>
#include <ErrorCodes.h>
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <Character.h>
#include <ClanMember.h>
#include <EntityStats.h>
#include <FriendSettings.h>
#include <MatchEntry.h>

// world Includes
#include "WorldServer.h"
#include "WorldSyncManager.h"

using namespace world;

CharacterManager::CharacterManager(const std::weak_ptr<WorldServer>& server)
    : mServer(server)
{
    // By default create the pending party
    mParties[0] = std::make_shared<objects::Party>();

    mMaxCID = 0;
    mMaxPartyID = 0;
    mMaxClanID = 0;
    mMaxTeamID = 0;
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

bool CharacterManager::UnregisterCharacter(std::shared_ptr<
    objects::CharacterLogin> cLogin)
{
    bool removed = false;
    if(cLogin)
    {
        std::lock_guard<std::mutex> lock(mLock);

        // Loop through each character instead of using the lookup
        // as the character may have already been removed
        for(auto& pair : mCharacterMap)
        {
            if(pair.second->GetWorldCID() == cLogin->GetWorldCID())
            {
                mCharacterMap.erase(pair.first);
                mCharacterCIDMap.erase(cLogin->GetWorldCID());
                removed = true;
                break;
            }
        }
    }

    return removed;
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

std::list<std::shared_ptr<objects::CharacterLogin>>
    CharacterManager::GetActiveCharacters()
{
    std::list<std::shared_ptr<objects::CharacterLogin>> active;

    std::lock_guard<std::mutex> lock(mLock);
    for(auto& pair : mCharacterCIDMap)
    {
        if(pair.second->GetStatus() !=
            objects::CharacterLogin::Status_t::OFFLINE)
        {
            active.push_back(pair.second);
        }
    }

    return active;
}

bool CharacterManager::RequestChannelDisconnect(int32_t worldCID)
{
    auto cLogin = GetCharacterLogin(worldCID);
    if(cLogin && cLogin->GetChannelID() >= 0)
    {
        auto channel = mServer.lock()->GetChannelConnectionByID(
            cLogin->GetChannelID());
        if(channel)
        {
            libcomp::Packet p;
            p.WritePacketCode(InternalPacketCode_t::PACKET_ACCOUNT_LOGOUT);
            p.WriteS32Little(worldCID);
            p.WriteU32Little((uint32_t)
                LogoutPacketAction_t::LOGOUT_DISCONNECT);

            channel->SendPacket(p);

            return true;
        }
    }

    return false;
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
        LogCharacterManagerError([&]()
        {
            return libcomp::String("Invalid world CID encountered: %1\n")
                .Arg(worldCID);
        });

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
        std::shared_ptr<objects::FriendSettings> fSettings;

        // If the character is currently loaded on the server, pull the friend
        // settings directly from it so we don't need to load every time
        auto character = cLogin->GetCharacter().Get();
        if(character && cLogin->GetStatus() !=
            objects::CharacterLogin::Status_t::OFFLINE)
        {
            fSettings = character->GetFriendSettings().Get(worldDB);
            if(!fSettings && !character->GetFriendSettings().IsNull())
            {
                LogCharacterManagerError([&]()
                {
                    return libcomp::String(
                        "Failed to get friend settings. Character UUID: %1\n")
                        .Arg(cLogin->GetCharacter().GetUUID().ToString());
                });
            }
        }
        else
        {
            fSettings = objects::FriendSettings::LoadFriendSettingsByCharacter(
                worldDB, cLogin->GetCharacter().GetUUID());
        }

        if(fSettings)
        {
            targetUUIDs = fSettings->GetFriends();
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
            for(auto worldCID : it->second->GetMemberIDs())
            {
                targetCIDs.push_back(worldCID);
            }
        }
    }

    if(relatedTypes & RELATED_TEAM)
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mTeams.find(cLogin->GetTeamID());
        if(it != mTeams.end())
        {
            for(auto worldCID : it->second->GetMemberIDs())
            {
                targetCIDs.push_back(worldCID);
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
            // If the zone is contained in the change, relay it to the player
            bool partyStatsOnly = zoneRestrict &&
                (0 == (outFlags & (uint8_t)(~((uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_INFO) |
                (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_PARTY_DEMON_INFO)));
            bool containsZone = 0 != (outFlags & (uint8_t)CharacterLoginStateFlag_t::CHARLOGIN_ZONE);
            SendToRelatedCharacters(reply, cLogin->GetWorldCID(), 1, relatedTypes, containsZone,
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
        member = GetPartyMember(cLogin->GetWorldCID());
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

std::shared_ptr<objects::PartyCharacter> CharacterManager::GetPartyMember(
    int32_t worldCID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mPartyCharacters.find(worldCID);
    if(it != mPartyCharacters.end())
    {
        return it->second;
    }

    return nullptr;
}

bool CharacterManager::AddToParty(std::shared_ptr<objects::PartyCharacter> member,
    uint32_t partyID)
{
    auto cid = member->GetWorldCID();
    auto login = GetCharacterLogin(cid);

    bool success = false;
    if(login)
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mParties.find(partyID);
        if(it != mParties.end() && it->second->MemberIDsCount() < MAX_PARTY_SIZE
            && (login->GetPartyID() == 0 || login->GetPartyID() == partyID))
        {
            mParties[0]->RemoveMemberIDs(cid);
            login->SetPartyID(partyID);
            it->second->InsertMemberIDs(cid);
            mPartyCharacters[cid] = member;

            success = true;
        }
    }

    if(success && partyID && login->GetTeamID())
    {
        // When joining a party, all teams must be left
        TeamLeave(login);
    }

    return success;
}

bool CharacterManager::PartyJoin(std::shared_ptr<objects::PartyCharacter> member,
    const libcomp::String targetName, uint32_t partyID,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection)
{
    bool newParty = false;
    uint16_t responseCode = (uint16_t)PartyErrorCodes_t::INVALID_OR_OFFLINE;
    std::shared_ptr<objects::CharacterLogin> targetLogin;
    if(!targetName.IsEmpty())
    {
        // Request response
        targetLogin = GetCharacterLogin(targetName);
        if(targetLogin && targetLogin->GetChannelID() >= 0)
        {
            auto targetMember = GetPartyMember(targetLogin->GetWorldCID());
            if(targetMember)
            {
                bool valid = true;
                if(partyID == 0)
                {
                    partyID = CreateParty(targetMember);
                    newParty = true;
                }
                else if(GetCharacterLogin(
                    targetMember->GetWorldCID())->GetPartyID() != partyID)
                {
                    responseCode = (uint16_t)PartyErrorCodes_t::IN_PARTY;
                    valid = false;
                }

                if(valid && AddToParty(member, partyID))
                {
                    responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
                }
            }
            else if(partyID == 0)
            {
                // If the target doesn't have a party and the requestor did not
                // supply the party ID, handle like an invite from the requestor
                AddToParty(member, 0);
                auto channel = mServer.lock()->GetChannelConnectionByID(targetLogin->GetChannelID());
                if(channel)
                {
                    libcomp::Packet relay;
                    WorldServer::GetRelayPacket(relay, targetLogin->GetWorldCID());
                    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_INVITED);
                    relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
                        member->GetName(), true);
                    relay.WriteU32Little(0);

                    channel->SendPacket(relay);

                    return true;
                }
            }
        }

        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, member->GetWorldCID());
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_JOIN);
        relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
            targetName, true);
        relay.WriteU16Little(responseCode);

        sourceConnection->QueuePacket(relay);
    }

    if(responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS)
    {
        SendPartyMember(member, partyID, newParty, false,
            sourceConnection);
    }

    sourceConnection->FlushOutgoing();

    return responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS;
}

bool CharacterManager::PartyRecruit(
    std::shared_ptr<objects::PartyCharacter> member,
    const libcomp::String targetName, uint32_t partyID,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection)
{
    bool newParty = false;
    uint16_t responseCode = (uint16_t)PartyErrorCodes_t::INVALID_OR_OFFLINE;
    std::shared_ptr<objects::PartyCharacter> targetMember;
    if(!targetName.IsEmpty())
    {
        // Recruit request response
        auto targetLogin = GetCharacterLogin(targetName);
        if(targetLogin && targetLogin->GetChannelID() >= 0)
        {
            targetMember = GetPartyMember(targetLogin->GetWorldCID());
            if(targetMember)
            {
                bool valid = true;
                if(partyID == 0)
                {
                    partyID = CreateParty(member);
                    newParty = true;
                }
                else if(GetCharacterLogin(
                    member->GetWorldCID())->GetPartyID() != partyID)
                {
                    responseCode = (uint16_t)PartyErrorCodes_t::INVALID_PARTY;
                    valid = false;
                }

                if(valid && AddToParty(targetMember, partyID))
                {
                    responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
                }
            }
        }

        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, member->GetWorldCID());
        relay.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_PARTY_RECRUIT);
        relay.WriteString16Little(libcomp::Convert::Encoding_t::
            ENCODING_CP932, targetName, true);
        relay.WriteU16Little(responseCode);

        sourceConnection->QueuePacket(relay);
    }

    if(responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS)
    {
        SendPartyMember(targetMember, partyID, newParty, false,
            sourceConnection);
    }

    sourceConnection->FlushOutgoing();

    return responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS;
}

void CharacterManager::PartyLeave(std::shared_ptr<objects::CharacterLogin> cLogin,
    std::shared_ptr<libcomp::TcpConnection> requestConnection)
{
    uint32_t partyID = cLogin->GetPartyID();
    auto party = GetParty(partyID);
    if(!party && !requestConnection)
    {
        return;
    }

    auto partyLogins = GetRelatedCharacterLogins(cLogin, RELATED_PARTY);

    uint16_t responseCode = (uint16_t)PartyErrorCodes_t::GENERIC_ERROR;
    if(RemoveFromParty(cLogin, partyID))
    {
        responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
        cLogin->SetPartyID(0);
    }

    if(requestConnection)
    {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_LEAVE);
        relay.WriteU16Little(responseCode);

        requestConnection->QueuePacket(relay);
    }

    if(responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS)
    {
        std::list<int32_t> includeCIDs = { cLogin->GetWorldCID() };
        SendPartyInfo(party->GetID(), includeCIDs);

        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LEAVE);
        request.WriteU8(0); // Not a response
        request.WriteS32Little(cLogin->GetWorldCID());

        partyLogins.push_back(cLogin);
        SendToCharacters(request, partyLogins, 1);

        auto memberIDs = party->GetMemberIDs();
        if(memberIDs.size() <= 1)
        {
            // Cannot exist with one or zero members
            PartyDisband(partyID, cLogin->GetWorldCID());
        }
        else if(cLogin->GetWorldCID() == party->GetLeaderCID())
        {
            // If the leader left, take the next person who joined
            PartyLeaderUpdate(party->GetID(), cLogin->GetWorldCID(),
                nullptr, *memberIDs.begin());
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

    uint16_t responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
    std::list<std::shared_ptr<objects::CharacterLogin>> partyLogins;

    if(partyID)
    {
        for(auto cid : party->GetMemberIDs())
        {
            auto login = GetCharacterLogin(cid);
            if(login)
            {
                partyLogins.push_back(login);

                if(partyID && RemoveFromParty(login, partyID))
                {
                    login->SetPartyID(0);
                }
                else
                {
                    responseCode = (uint16_t)PartyErrorCodes_t::GENERIC_ERROR;
                    break;
                }
            }
        }
    }
    else
    {
        responseCode = (uint16_t)PartyErrorCodes_t::NO_PARTY;
    }

    if(requestConnection)
    {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, sourceCID);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DISBAND);
        relay.WriteU16Little(responseCode);

        requestConnection->QueuePacket(relay);
    }

    if(responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS)
    {
        {
            std::lock_guard<std::mutex> lock(mLock);
            mParties.erase(party->GetID());
        }

        std::list<int32_t> includeCIDs;
        for(auto login : partyLogins)
        {
            includeCIDs.push_back(login->GetWorldCID());
        }

        SendPartyInfo(party->GetID(), includeCIDs);

        libcomp::Packet relay;
        auto cidOffset = WorldServer::GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DISBANDED);

        SendToCharacters(relay, partyLogins, cidOffset);
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

    uint16_t responseCode = (uint16_t)PartyErrorCodes_t::GENERIC_ERROR;
    if(party->MemberIDsContains(targetCID))
    {
        party->SetLeaderCID(targetCID);
        responseCode = (uint16_t)PartyErrorCodes_t::SUCCESS;
    }

    if(requestConnection)
    {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, sourceCID);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_LEADER_UPDATE);
        relay.WriteU16Little(responseCode);

        requestConnection->QueuePacket(relay);
    }

    if(responseCode == (uint16_t)PartyErrorCodes_t::SUCCESS)
    {
        SendPartyInfo(partyID);

        libcomp::Packet request;
        request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
        request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_LEADER_UPDATE);
        request.WriteU8(0); // Not a response
        request.WriteS32Little(targetCID);

        std::list<std::shared_ptr<objects::CharacterLogin>> partyLogins;
        for(auto cid : party->GetMemberIDs())
        {
            partyLogins.push_back(GetCharacterLogin(cid));
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

    auto targetLogin = GetCharacterLogin(targetCID);
    auto partyLogins = GetRelatedCharacterLogins(cLogin, RELATED_PARTY);
    if(targetLogin)
    {
        RemoveFromParty(targetLogin, party->GetID());
        targetLogin->SetPartyID(0);
    }

    std::list<int32_t> includeCIDs = { targetCID };
    SendPartyInfo(party->GetID(), includeCIDs);

    if(party->MemberIDsCount() <= 1)
    {
        PartyDisband(party->GetID());
    }

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
    request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_GROUP_KICK);
    request.WriteS32Little(targetCID);

    partyLogins.push_back(cLogin);
    SendToCharacters(request, partyLogins, 1);
}

void CharacterManager::SendPartyInfo(uint32_t partyID, const std::list<int32_t>& cids)
{
    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
    request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_UPDATE);
    request.WriteU32Little(partyID);

    std::list<std::shared_ptr<objects::CharacterLogin>> logins;
    for(auto cid : cids)
    {
        logins.push_back(GetCharacterLogin(cid));
    }

    auto party = GetParty(partyID);
    if(party)
    {
        request.WriteU8(1); // Party set
        party->SavePacket(request);

        for(auto cid : party->GetMemberIDs())
        {
            logins.push_back(GetCharacterLogin(cid));
        }
    }
    else
    {
        request.WriteU8(0); // Party not set
    }

    SendToCharacters(request, logins, 1);
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
    newMember->SetClan(clan->GetUUID());
    newMember->SetMemberType(objects::ClanMember::MemberType_t::NORMAL);
    newMember->SetCharacter(character->GetUUID());

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
    auto cidOffset = WorldServer::GetRelayPacket(relay);
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

bool CharacterManager::ClanLeave(std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t clanID, std::shared_ptr<libcomp::TcpConnection> requestConnection)
{
    auto clanLogins = GetRelatedCharacterLogins(cLogin, RELATED_CLAN);
    clanLogins.push_back(cLogin);

    if(requestConnection)
    {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_LEAVE);
        relay.WriteS8(0);   // Response code doesn't seem to matter

        requestConnection->SendPacket(relay);
    }

    auto clanInfo = GetClan(clanID);
    auto member = clanInfo->GetMemberMap(cLogin->GetWorldCID()).Get();
    if(RemoveFromClan(cLogin, clanID))
    {
        libcomp::Packet relay;
        auto cidOffset = WorldServer::GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_LEFT);
        relay.WriteS32Little(clanID);
        relay.WriteS32Little(cLogin->GetWorldCID());

        SendToCharacters(relay, clanLogins, cidOffset);
        RecalculateClanLevel(clanID);

        std::list<int32_t> cids = { cLogin->GetWorldCID() };
        SendClanInfo(0, 0x0F, cids);

        if(member && member->GetMemberType() ==
            objects::ClanMember::MemberType_t::MASTER)
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
                auto newMasterLogin = GetCharacterLogin(newMaster->GetCharacter());

                auto server = mServer.lock();
                auto worldDB = server->GetWorldDatabase();
                newMaster->SetMemberType(objects::ClanMember::MemberType_t::MASTER);
                newMaster->Update(worldDB);

                relay.Clear();
                cidOffset = WorldServer::GetRelayPacket(relay);
                relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CLAN_MASTER_UPDATED);
                relay.WriteS32Little(clanID);
                relay.WriteS32Little(newMasterLogin->GetWorldCID());

                SendToRelatedCharacters(relay, newMasterLogin->GetWorldCID(), cidOffset,
                    RELATED_CLAN, true);
            }
        }

        return true;
    }

    return false;
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
        WorldServer::GetRelayPacket(relay, sourceCID);
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
                objects::Character>(worldDB, member->GetCharacter(), true);
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
        auto cidOffset = WorldServer::GetRelayPacket(relay);
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
    if(requestConnection)
    {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
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
        auto cidOffset = WorldServer::GetRelayPacket(relay);
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
        for(auto memberRef : clan->GetMembers())
        {
            auto member = memberRef.Get(db);
            auto character = member
                ? libcomp::PersistentObject::LoadObjectByUUID<
                objects::Character>(db, member->GetCharacter()) : nullptr;

            if(!character)
            {
                LogCharacterManagerWarning([&]()
                {
                    return libcomp::String("Invalid clan member encountered"
                        " on clan '%1' with UID: %2\n").Arg(clan->GetName())
                        .Arg(memberRef.GetUUID().ToString());
                });

                continue;
            }

            totalPoints = (uint64_t)(totalPoints + (uint64_t)
                character->GetLoginPoints());
        }

        uint32_t newLevel = 0;
        for(newLevel = 10; newLevel > 1; newLevel--)
        {
            size_t idx = (size_t)(newLevel - 1);
            if(libcomp::CLAN_POINT_REQUIREMENT[idx] <= totalPoints)
            {
                break;
            }
        }

        if(newLevel == 0)
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
    WorldServer::GetRelayPacket(relay, cLogin->GetWorldCID());
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

            // If any data cannot be loaded from a character, send default
            // values and move on. Any broken pointers to clan data should
            // be handled via a cleanup process.
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

            if(memberLogin)
            {
                relay.WriteS8((int8_t)memberLogin->GetStatus());
                relay.WriteU8(memberLogin->GetWorldCID() == cLogin->GetWorldCID()
                    ? 1 : 0);
                relay.WriteS8(memberLogin->GetChannelID());
                relay.WriteS32Little(memberLogin->GetZoneID()
                    ? (int32_t)memberLogin->GetZoneID() : -1);
                relay.WriteS32Little(memberChar
                    ? (int32_t)memberChar->GetLastLogin() : 0);
            }
            else
            {
                relay.WriteBlank(11);
            }

            relay.WriteS8(stats ? stats->GetLevel() : 0);
            relay.WriteS32Little(memberChar
                ? memberChar->GetLoginPoints() : 0);
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
        libcomp::Packet relay;
        auto cidOffset = WorldServer::GetRelayPacket(relay);
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
            auto worldDB = mServer.lock()->GetWorldDatabase();
            relay.WriteS8(cLogin->GetCharacter()->LoadCoreStats(worldDB)->GetLevel());
        }

        SendToRelatedCharacters(relay, cLogin->GetWorldCID(), cidOffset,
            RELATED_CLAN, true);
    }
}

std::shared_ptr<objects::Team> CharacterManager::GetTeam(int32_t teamID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mTeams.find(teamID);
    return it != mTeams.end() ? it->second : nullptr;
}

size_t CharacterManager::GetTeamMaxSize(int8_t category) const
{
    switch((objects::Team::Category_t)category)
    {
    case objects::Team::Category_t::PVP:
        return MAX_TEAM_SIZE_PVP;
    case objects::Team::Category_t::DIASPORA:
        return MAX_TEAM_SIZE_DIASPORA;
    case objects::Team::Category_t::CATHEDRAL:
        return MAX_TEAM_SIZE_CATHEDRAL;
    default:
        return 1;
    }
}

int32_t CharacterManager::AddToTeam(int32_t worldCID, int32_t teamID)
{
    auto login = GetCharacterLogin(worldCID);
    if(!login)
    {
        return 0;
    }

    if(login->GetTeamID())
    {
        return teamID == 0 ? login->GetTeamID() : 0;
    }

    if(teamID)
    {
        // Add to existing team
        auto team = GetTeam(teamID);

        std::lock_guard<std::mutex> lock(mLock);
        if(!team || team->MemberIDsCount() >=
            GetTeamMaxSize((int8_t)team->GetCategory()))
        {
            // Cannot add to team
            return 0;
        }

        login->SetTeamID(teamID);
        team->InsertMemberIDs(worldCID);
    }
    else
    {
        // Create new team
        std::lock_guard<std::mutex> lock(mLock);

        teamID = ++mMaxTeamID;
        login->SetTeamID(teamID);

        auto team = std::make_shared<objects::Team>();
        team->SetID(teamID);
        team->SetLeaderCID(worldCID);
        team->InsertMemberIDs(worldCID);
        mTeams[teamID] = team;
    }

    return teamID;
}

bool CharacterManager::TeamJoin(int32_t worldCID, int32_t teamID,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection)
{
    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;

    auto team = GetTeam(teamID);
    auto cLogin = GetCharacterLogin(worldCID);
    if(!team)
    {
        errorCode = (int8_t)TeamErrorCodes_t::INVALID_TEAM;
    }
    else if(cLogin && AddToTeam(worldCID, teamID))
    {
        errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
    }

    libcomp::Packet relay;
    WorldServer::GetRelayPacket(relay, worldCID);
    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_ANSWER);
    relay.WriteS32Little(teamID);
    relay.WriteS8(1);   // Accepted
    relay.WriteS8(errorCode);

    sourceConnection->QueuePacket(relay);

    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        // Tell everyone in the team, including the character who just joined
        // that the join has happened
        relay.Clear();
        auto cidOffset = WorldServer::GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_MEMBER_ADD);
        relay.WriteS32Little(teamID);
        relay.WriteS32Little(worldCID);
        relay.WriteString16Little(libcomp::Convert::Encoding_t::ENCODING_CP932,
            cLogin->GetCharacter()->GetName(), true);

        SendToRelatedCharacters(relay, worldCID, cidOffset, RELATED_TEAM, true);

        SendTeamInfo(teamID);

        TeamZiotiteUpdate(teamID);
    }

    sourceConnection->FlushOutgoing();

    return errorCode == (int8_t)TeamErrorCodes_t::SUCCESS;
}

void CharacterManager::TeamLeave(std::shared_ptr<objects::CharacterLogin> cLogin)
{
    int32_t teamID = cLogin->GetTeamID();
    auto team = GetTeam(teamID);
    if(!team)
    {
        return;
    }

    auto teamLogins = GetRelatedCharacterLogins(cLogin, RELATED_TEAM);

    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
    if(RemoveFromTeam(cLogin, teamID))
    {
        errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
        cLogin->SetTeamID(0);
    }

    libcomp::Packet relay;
    auto cidOffset = WorldServer::GetRelayPacket(relay);
    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_LEAVE);
    relay.WriteS32Little(teamID);
    relay.WriteS8(errorCode);

    SendToCharacters(relay, { cLogin }, cidOffset);

    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        std::list<int32_t> includeCIDs = { cLogin->GetWorldCID() };
        SendTeamInfo(teamID, includeCIDs);

        relay.Clear();
        cidOffset = WorldServer::GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_LEFT);
        relay.WriteS32Little(teamID);
        relay.WriteS32Little(cLogin->GetWorldCID());

        SendToCharacters(relay, teamLogins, cidOffset);

        auto memberIDs = team->GetMemberIDs();
        if(memberIDs.size() == 0)
        {
            // Cannot exist with no members
            TeamDisband(teamID, cLogin->GetWorldCID());
            return;
        }
        else if(cLogin->GetWorldCID() == team->GetLeaderCID())
        {
            // If the leader left, take the next person who joined
            TeamLeaderUpdate(team->GetID(), 0, nullptr, *memberIDs.begin());
        }

        // Send the new ziotite count
        TeamZiotiteUpdate(team->GetID());
    }
}

void CharacterManager::TeamDisband(int32_t teamID, int32_t sourceCID,
    bool toDiaspora)
{
    (void)sourceCID;

    auto team = GetTeam(teamID);

    bool success = true;

    std::list<std::shared_ptr<objects::CharacterLogin>> teamLogins;
    if(teamID)
    {
        for(auto cid : team->GetMemberIDs())
        {
            auto login = GetCharacterLogin(cid);
            if(login)
            {
                teamLogins.push_back(login);

                if(RemoveFromTeam(login, teamID))
                {
                    login->SetTeamID(0);
                }
                else
                {
                    success = false;
                    break;
                }
            }
        }
    }
    else
    {
        success = false;
    }

    if(success)
    {
        {
            std::lock_guard<std::mutex> lock(mLock);
            mTeams.erase(teamID);
        }

        std::list<int32_t> includeCIDs;
        for(auto login : teamLogins)
        {
            includeCIDs.push_back(login->GetWorldCID());
        }

        SendTeamInfo(teamID, includeCIDs);

        if(toDiaspora)
        {
            libcomp::Packet relay;
            auto cidOffset = WorldServer::GetRelayPacket(relay);
            relay.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_DIASPORA_TEAM_READY);
            relay.WriteS32Little(team->GetID());
            relay.WriteS8((int8_t)team->GetType());

            SendToCharacters(relay, teamLogins, cidOffset);
        }

        libcomp::Packet relay;
        auto cidOffset = WorldServer::GetRelayPacket(relay);
        relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TEAM_DISBAND);

        SendToCharacters(relay, teamLogins, cidOffset);
    }
}

void CharacterManager::TeamLeaderUpdate(int32_t teamID, int32_t sourceCID,
    std::shared_ptr<libcomp::TcpConnection> requestConnection,
    int32_t targetCID)
{
    auto team = GetTeam(teamID);

    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
    if(team)
    {
        auto cLogin = sourceCID ? GetCharacterLogin(sourceCID) : nullptr;
        if(sourceCID && (!cLogin || cLogin->GetTeamID() != teamID))
        {
            errorCode = (int8_t)TeamErrorCodes_t::INVALID_TEAM;
        }
        else if(cLogin && team->GetLeaderCID() != cLogin->GetWorldCID())
        {
            errorCode = (int8_t)TeamErrorCodes_t::LEADER_REQUIRED;
        }
        else if(!team->MemberIDsContains(targetCID))
        {
            errorCode = (int8_t)TeamErrorCodes_t::INVALID_TARGET;
        }
        else
        {
            team->SetLeaderCID(targetCID);
            errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
        }
    }

    if(requestConnection)
    {
        libcomp::Packet relay;
        WorldServer::GetRelayPacket(relay, sourceCID);
        relay.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TEAM_LEADER_UPDATE);
        relay.WriteS32Little(teamID);
        relay.WriteS8(errorCode);

        requestConnection->QueuePacket(relay);
    }

    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        SendTeamInfo(teamID);

        libcomp::Packet relay;
        auto cidOffset = WorldServer::GetRelayPacket(relay);
        relay.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TEAM_LEADER_UPDATED);
        relay.WriteS32Little(teamID);
        relay.WriteS32Little(targetCID);

        std::list<std::shared_ptr<objects::CharacterLogin>> teamLogins;
        for(auto cid : team->GetMemberIDs())
        {
            teamLogins.push_back(GetCharacterLogin(cid));
        }

        SendToCharacters(relay, teamLogins, cidOffset);
    }

    if(requestConnection)
    {
        requestConnection->FlushOutgoing();
    }
}

void CharacterManager::TeamKick(std::shared_ptr<objects::CharacterLogin> cLogin,
    int32_t targetCID, int32_t teamID)
{
    auto team = GetTeam(cLogin->GetTeamID());

    int8_t errorCode = (int8_t)TeamErrorCodes_t::GENERIC_ERROR;
    if(team)
    {
        if(cLogin->GetTeamID() != teamID)
        {
            errorCode = (int8_t)TeamErrorCodes_t::INVALID_TEAM;
        }
        else if(team->GetLeaderCID() != cLogin->GetWorldCID())
        {
            errorCode = (int8_t)TeamErrorCodes_t::LEADER_REQUIRED;
        }
        else if(!team->MemberIDsContains(targetCID))
        {
            errorCode = (int8_t)TeamErrorCodes_t::INVALID_TARGET;
        }
        else
        {
            auto targetLogin = GetCharacterLogin(targetCID);
            if(targetLogin && RemoveFromTeam(targetLogin, teamID))
            {
                targetLogin->SetTeamID(0);

                libcomp::Packet relay;
                auto cidOffset = WorldServer::GetRelayPacket(relay);
                relay.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_TEAM_KICKED);
                relay.WriteS32Little(teamID);
                relay.WriteS32Little(targetCID);

                SendToCharacters(relay, { targetLogin }, cidOffset);

                errorCode = (int8_t)TeamErrorCodes_t::SUCCESS;
            }
        }
    }

    // Notify remaining members (or source only if not success)
    libcomp::Packet relay;
    auto cidOffset = WorldServer::GetRelayPacket(relay);
    relay.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_TEAM_KICK);
    relay.WriteS32Little(teamID);
    relay.WriteS8(errorCode);
    relay.WriteS32Little(targetCID);

    std::list<std::shared_ptr<objects::CharacterLogin>> teamLogins;
    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        teamLogins = GetRelatedCharacterLogins(cLogin, RELATED_TEAM);
    }

    teamLogins.push_back(cLogin);

    SendToCharacters(relay, teamLogins, cidOffset);

    if(errorCode == (int8_t)TeamErrorCodes_t::SUCCESS)
    {
        std::list<int32_t> includeCIDs = { targetCID };
        SendTeamInfo(team->GetID(), includeCIDs);

        // Send the new ziotite count
        TeamZiotiteUpdate(team->GetID());
    }
}

bool CharacterManager::TeamZiotiteUpdate(int32_t teamID,
    const std::shared_ptr<objects::CharacterLogin>& source,
    int32_t sZiotite, int8_t lZiotite)
{
    auto team = GetTeam(teamID);
    if(!team || (source && source->GetTeamID() != teamID))
    {
        LogCharacterManagerError([&]()
        {
            return libcomp::String("Ziotite could not be updated"
                " for invalid team from character: %1\n").Arg(source
                ? source->GetCharacter().GetUUID().ToString() : "NONE");
        });

        return false;
    }
    else if(team->GetCategory() != objects::Team::Category_t::CATHEDRAL)
    {
        // No ziotite
        return sZiotite == 0 && lZiotite == 0;
    }

    int32_t newSAmount = 0;
    int8_t newLAmount = 0;
    {
        std::lock_guard<std::mutex> lock(mLock);
        newSAmount = team->GetSmallZiotite() + sZiotite;
        newLAmount = (int8_t)(team->GetLargeZiotite() + lZiotite);

        // Let the channel handle this check for spending
        if(newSAmount < 0)
        {
            newSAmount = 0;
        }

        if(newLAmount < 0)
        {
            newLAmount = 0;
        }

        // Apply limits
        if(newLAmount > 3)
        {
            newLAmount = 3;
        }

        int32_t sLimit = (int32_t)(team->MemberIDsCount() * 10000);
        if(newSAmount > sLimit)
        {
            newSAmount = sLimit;
        }

        if((sZiotite || lZiotite) &&
            newSAmount == team->GetSmallZiotite() &&
            newLAmount == team->GetLargeZiotite())
        {
            // No update
            return false;
        }

        team->SetSmallZiotite(newSAmount);
        team->SetLargeZiotite(newLAmount);
    }

    // Send the ziotite update directly
    std::list<std::shared_ptr<objects::CharacterLogin>> logins;
    for(auto cid : team->GetMemberIDs())
    {
        logins.push_back(GetCharacterLogin(cid));
    }

    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_TEAM_UPDATE);
    request.WriteU8((uint8_t)
        InternalPacketAction_t::PACKET_ACTION_TEAM_ZIOTITE);
    request.WriteS32Little(teamID);
    request.WriteS32Little(newSAmount);
    request.WriteS8(newLAmount);

    SendToCharacters(request, logins, 1);

    return true;
}

void CharacterManager::SendTeamInfo(int32_t teamID, const std::list<int32_t>& cids)
{
    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_TEAM_UPDATE);
    request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_UPDATE);
    request.WriteS32Little(teamID);

    std::list<std::shared_ptr<objects::CharacterLogin>> logins;
    for(auto cid : cids)
    {
        logins.push_back(GetCharacterLogin(cid));
    }

    auto team = GetTeam(teamID);
    if(team)
    {
        request.WriteU8(1); // Team set
        team->SavePacket(request);

        for(auto cid : team->GetMemberIDs())
        {
            logins.push_back(GetCharacterLogin(cid));
        }
    }
    else
    {
        request.WriteU8(0); // Team not set
    }

    SendToCharacters(request, logins, 1);
}

uint32_t CharacterManager::CreateParty(std::shared_ptr<objects::PartyCharacter> member)
{
    auto cid = member->GetWorldCID();
    auto login = GetCharacterLogin(cid);

    uint32_t partyID = 0;
    if(!login)
    {
        return 0;
    }
    else
    {
        std::lock_guard<std::mutex> lock(mLock);
        partyID = login->GetPartyID();
        if(!partyID)
        {
            mParties[0]->RemoveMemberIDs(cid);
            partyID = ++mMaxPartyID;
            login->SetPartyID(partyID);

            auto party = std::make_shared<objects::Party>();
            party->SetID(partyID);
            party->SetLeaderCID(cid);
            party->InsertMemberIDs(cid);
            mParties[partyID] = party;
            mPartyCharacters[cid] = member;
        }
    }

    if(partyID && login->GetTeamID())
    {
        // When creating a party, all teams must be left
        TeamLeave(login);
    }

    return partyID;
}

void CharacterManager::SendPartyMember(
    std::shared_ptr<objects::PartyCharacter> member, uint32_t partyID,
    bool newParty, bool isRefresh,
    std::shared_ptr<libcomp::TcpConnection> sourceConnection)
{
    SendPartyInfo(partyID);

    auto cLogin = GetCharacterLogin(member->GetWorldCID());
    auto party = GetParty(partyID);
    auto partyMemberIDs = party->GetMemberIDs();

    // All members
    libcomp::Packet request;
    request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
    request.WriteU8((uint8_t)InternalPacketAction_t::PACKET_ACTION_ADD);
    request.WriteU32Little(partyID);
    request.WriteU8((uint8_t)partyMemberIDs.size());
    for(auto cid : partyMemberIDs)
    {
        auto login = GetCharacterLogin(cid);
        auto partyMember = GetPartyMember(cid);
        partyMember->SavePacket(request, false);
        request.WriteU32Little(login->GetZoneID());
        request.WriteU8(party->GetLeaderCID() == cid ? 1 : 0);
    }

    if(newParty)
    {
        if(!isRefresh)
        {
            // Send everyone to everyone
            SendToRelatedCharacters(request, member->GetWorldCID(),
                1, RELATED_PARTY, true);
        }
    }
    else
    {
        // Send everyone to the new member
        ConvertToTargetCIDPacket(request, 1, 1);
        request.WriteS32Little(member->GetWorldCID());
        sourceConnection->SendPacket(request);

        if(!isRefresh)
        {
            // Send new member to everyone else
            request.Clear();
            request.WritePacketCode(InternalPacketCode_t::PACKET_PARTY_UPDATE);
            request.WriteU8((uint8_t)
                InternalPacketAction_t::PACKET_ACTION_ADD);
            request.WriteU32Little(partyID);
            request.WriteU8(1);
            member->SavePacket(request, false);
            request.WriteU32Little(cLogin->GetZoneID());
            request.WriteU8(0);

            SendToRelatedCharacters(request, member->GetWorldCID(),
                1, RELATED_PARTY);
        }
    }

    libcomp::Packet relay;
    auto cidOffset = WorldServer::GetRelayPacket(relay);
    relay.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTY_DROP_RULE_SET);
    relay.WriteU8((uint8_t)party->GetDropRule());

    // Send to everyone if the party is new
    SendToRelatedCharacters(relay, member->GetWorldCID(),
        cidOffset, newParty ? RELATED_PARTY : 0, true);
}

bool CharacterManager::RemoveFromParty(std::shared_ptr<objects::CharacterLogin> cLogin,
    uint32_t partyID)
{
    std::lock_guard<std::mutex> lock(mLock);
    auto cid = cLogin->GetWorldCID();
    auto it = mParties.find(partyID);
    if(it != mParties.end() && it->second->MemberIDsContains(cid))
    {
        it->second->RemoveMemberIDs(cid);
        mPartyCharacters.erase(cid);
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

        auto server = mServer.lock();
        auto worldDB = server->GetWorldDatabase();

        size_t idx = 0;
        std::shared_ptr<objects::ClanMember> member;
        if(clan)
        {
            for(auto mRef : clan->GetMembers())
            {
                auto m = mRef.Get(worldDB);
                if(m)
                {
                    if(m->GetCharacter() == cLogin->GetCharacter().GetUUID())
                    {
                        member = m;
                        clan->RemoveMembers(idx);
                        break;
                    }
                }
                else
                {
                    LogCharacterManagerError([&]()
                    {
                        return libcomp::String("Invalid clan member %1"
                            " encountered on clan %1\n")
                            .Arg(mRef.GetUUID().ToString())
                            .Arg(clan->GetUUID().ToString());
                    });
                }

                idx++;
            }
        }

        auto dbChanges = libcomp::DatabaseChangeSet::Create();

        if(member)
        {
            dbChanges->Update(clan);
            dbChanges->Delete(member);
        }

        auto character = cLogin->LoadCharacter(worldDB);
        if(character && character->GetClan().GetUUID() == clanInfo
            ->GetClan().GetUUID())
        {
            character->SetClan(NULLUUID);
            dbChanges->Update(character);
        }

        return worldDB->ProcessChangeSet(dbChanges);
    }

    return false;
}

bool CharacterManager::RemoveFromTeam(std::shared_ptr<
    objects::CharacterLogin> cLogin, int32_t teamID)
{
    bool success = false;

    auto cid = cLogin->GetWorldCID();
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = mTeams.find(teamID);
        if(it != mTeams.end() && it->second->MemberIDsContains(cid))
        {
            it->second->RemoveMemberIDs(cid);
            success = true;
        }
    }

    if(success)
    {
        // If a match entry exists, remove it
        auto syncManager = mServer.lock()->GetWorldSyncManager();
        auto entry = syncManager->GetMatchEntry(cid);
        if(entry && syncManager->RemoveRecord(entry, "MatchEntry"))
        {
            syncManager->SyncOutgoing();
        }
    }

    return success;
}
