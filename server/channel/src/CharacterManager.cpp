/**
 * @file server/channel/src/CharacterManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages characters on the channel.
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

#include "CharacterManager.h"

// libcomp Includes
#include <Constants.h>
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <Expertise.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiAcquisitionSkillData.h>
#include <MiDevilBattleData.h>
#include <MiDevilData.h>
#include <MiDevilLVUpData.h>
#include <MiDevilLVUpRateData.h>
#include <MiExpertData.h>
#include <MiExpertGrowthTbl.h>
#include <MiGrowthData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiNPCBasicData.h>
#include <MiPossessionData.h>
#include <MiSkillData.h>
#include <ServerZone.h>
#include <StatusEffect.h>

using namespace channel;

CharacterManager::CharacterManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

CharacterManager::~CharacterManager()
{
}

void CharacterManager::SendCharacterData(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto c = cState->GetEntity();
    auto cs = c->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHARACTER_DATA);

    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        c->GetName(), true);
    reply.WriteU32Little(0); // Special Title
    reply.WriteU8((uint8_t)c->GetGender());
    reply.WriteU8(c->GetSkinType());
    reply.WriteU8(c->GetHairType());
    reply.WriteU8(c->GetHairColor());
    reply.WriteU8(c->GetEyeType());
    reply.WriteU8(c->GetRightEyeColor());
    reply.WriteU8(c->GetFaceType());
    reply.WriteU8(c->GetLeftEyeColor());
    reply.WriteU8(0x00); // Unknown
    reply.WriteU8(0x01); // Unknown bool

    for(size_t i = 0; i < 15; i++)
    {
        auto equip = c->GetEquippedItems(i);

        if(!equip.IsNull())
        {
            reply.WriteU32Little(equip->GetType());
        }
        else
        {
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }
    }

    //Character status
    reply.WriteS16Little(cState->GetMaxHP());
    reply.WriteS16Little(cState->GetMaxMP());
    reply.WriteS16Little(cs->GetHP());
    reply.WriteS16Little(cs->GetMP());
    reply.WriteS64Little(cs->GetXP());
    reply.WriteS32Little(c->GetPoints());
    reply.WriteS8(cs->GetLevel());
    reply.WriteS16Little(c->GetLNC());

    GetEntityStatsPacketData(reply, cs, cState, false);

    reply.WriteS16(-5600); // Unknown
    reply.WriteS16(5600); // Unknown

    // Add status effects + 1 for testing effect below
    size_t statusEffectCount = c->StatusEffectsCount() + 1;
    reply.WriteU32Little(static_cast<uint32_t>(statusEffectCount));
    for(auto effect : c->GetStatusEffects())
    {
        reply.WriteU32Little(effect->GetEffect());
        // Expiration time is returned as a float OR int32 depending
        // on if it is a countdown in game seconds remaining or a
        // fixed time to expire.  This is dependent on the effect type.
        /// @todo: implement fixed time expiration
        reply.WriteFloat(state->ToClientTime(
            (ServerTime)effect->GetDuration()));
        reply.WriteU8(effect->GetStack());
    }

    // This is the COMP experience alpha status effect (hence +1)...
    reply.WriteU32Little(1055);
    reply.WriteU32Little(1325025608);   // Fixed time expiration
    reply.WriteU8(1);

    size_t skillCount = c->LearnedSkillsCount();
    reply.WriteU32(static_cast<uint32_t>(skillCount));
    for(auto skill : c->GetLearnedSkills())
    {
        reply.WriteU32Little(skill);
    }

    for(size_t i = 0; i < 38; i++)
    {
        auto expertise = c->GetExpertises(i);

        if(expertise.IsNull())
        {
            reply.WriteS32Little(0);
            reply.WriteS8((int8_t)i);
            reply.WriteU8(1);
        }
        else
        {
            reply.WriteS32Little(expertise->GetPoints());
            reply.WriteS8((int8_t)i);
            reply.WriteU8(expertise->GetDisabled() ? 1 : 0);
        }
    }

    reply.WriteU8(0);   // Unknown bool
    reply.WriteU8(0);   // Unknown bool
    reply.WriteU8(0);   // Unknown bool
    reply.WriteU8(0);   // Unknown bool

    auto activeDemon = c->GetActiveDemon();
    if(!activeDemon.IsNull())
    {
        reply.WriteS64Little(state->GetObjectID(
            activeDemon.GetUUID()));
    }
    else
    {
        reply.WriteS64Little(-1);
    }

    // Unknown
    reply.WriteS64Little(-1);
    reply.WriteS64Little(-1);

    auto zone = mServer.lock()->GetZoneManager()->GetZoneInstance(client);
    auto zoneDef = zone->GetDefinition();

    reply.WriteS32Little((int32_t)zone->GetID());
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteFloat(cState->GetDestinationX());
    reply.WriteFloat(cState->GetDestinationY());
    reply.WriteFloat(cState->GetDestinationRotation());

    reply.WriteU8(0);   //Unknown bool
    reply.WriteS32Little(0); // Homepoint zone
    reply.WriteFloat(0); // Homepoint X
    reply.WriteFloat(0); // Homepoint Y
    reply.WriteS8(0);   // Unknown
    reply.WriteS8(0);   // Unknown
    reply.WriteS8(0);   // Unknown

    /// @todo: Virtual Appearance
    size_t vaCount = 0;
    reply.WriteS32(static_cast<int32_t>(vaCount));
    for(size_t i = 0; i < vaCount; i++)
    {
        reply.WriteS8(0);   // Equipment Slot
        reply.WriteU32Little(0);    // VA Item Type
    }

    client->SendPacket(reply);
}

void CharacterManager::SendOtherCharacterData(const std::list<std::shared_ptr<
    ChannelClientConnection>> &clients, ClientState *otherState)
{
    if(clients.size() == 0)
    {
        return;
    }

    // Keep track of where client specific times need to be written
    std::unordered_map<uint32_t, ServerTime> timePositions;

    auto zone = mServer.lock()->GetZoneManager()->GetZoneInstance(clients.front());
    auto zoneDef = zone->GetDefinition();

    auto cState = otherState->GetCharacterState();
    auto c = cState->GetEntity();
    auto cs = c->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OTHER_CHARACTER_DATA);

    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        c->GetName(), true);
    reply.WriteU32Little(0); // Special Title
    reply.WriteS32Little(otherState->GetDemonState()->GetEntityID());
    reply.WriteU8((uint8_t)c->GetGender());
    reply.WriteU8(c->GetSkinType());
    reply.WriteU8(c->GetHairType());
    reply.WriteU8(c->GetHairColor());
    reply.WriteU8(c->GetEyeType());
    reply.WriteU8(c->GetRightEyeColor());
    reply.WriteU8(c->GetFaceType());
    reply.WriteU8(c->GetLeftEyeColor());
    reply.WriteU8(0x00); // Unknown
    reply.WriteU8(0x01); // Unknown bool

    for(size_t i = 0; i < 15; i++)
    {
        auto equip = c->GetEquippedItems(i);

        if(!equip.IsNull())
        {
            reply.WriteU32Little(equip->GetType());
        }
        else
        {
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }
    }

    reply.WriteS16Little(cState->GetMaxHP());
    reply.WriteS16Little(cState->GetMaxMP());
    reply.WriteS16Little(cs->GetHP());
    reply.WriteS16Little(cs->GetMP());
    reply.WriteS8(cs->GetLevel());
    reply.WriteS16Little(c->GetLNC());
    
    size_t statusEffectCount = c->StatusEffectsCount();
    reply.WriteU32Little(static_cast<uint32_t>(statusEffectCount));
    for(auto effect : c->GetStatusEffects())
    {
        reply.WriteU32Little(effect->GetEffect());
        timePositions[reply.Size()] = effect->GetDuration();
        reply.WriteFloat(0.f);
        reply.WriteU8(effect->GetStack());
    }

    // Unknown
    reply.WriteS64Little(-1);
    reply.WriteS64Little(-1);

    reply.WriteS32Little((int32_t)zone->GetID());
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteFloat(cState->GetDestinationX());
    reply.WriteFloat(cState->GetDestinationY());
    reply.WriteFloat(cState->GetDestinationRotation());

    reply.WriteU8(0);   //Unknown bool
    reply.WriteS8(0);   // Unknown
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        "Unknown", true);
    reply.WriteS8(0);   // Unknown
    reply.WriteS8(0);   // Unknown
    reply.WriteS8(0);   // Unknown

    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Unknown

    for(size_t i = 0; i < 13; i++)
    {
        reply.WriteS16Little(0);    //Unknown
    }

    reply.WriteU8(0);   //Unknown bool
    reply.WriteS8(0);   // Unknown
    reply.WriteS32(0);  // Unknown
    reply.WriteS8(0);   // Unknown
    
    /// @todo: Virtual Appearance
    size_t vaCount = 0;
    reply.WriteS32(static_cast<int32_t>(vaCount));
    for(size_t i = 0; i < vaCount; i++)
    {
        reply.WriteS8(0);   // Equipment Slot
        reply.WriteU32Little(0);    // VA Item Type
    }

    for(auto client : clients)
    {
        auto state = client->GetClientState();
        for(auto timePair : timePositions)
        {
            reply.Seek(timePair.first);
            reply.WriteFloat(state->ToClientTime(timePair.second));
        }

        client->SendPacket(reply);
    }
}

void CharacterManager::SendPartnerData(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();
    auto comp = character->GetCOMP();

    auto d = dState->GetEntity();
    if(d == nullptr)
    {
        return;
    }

    int8_t slot = -1;
    for(size_t i = 0; i < comp.size(); i++)
    {
        if(d == comp[i].Get())
        {
            slot = (int8_t)i;
            break;
        }
    }

    if(slot == -1)
    {
        LOG_ERROR(libcomp::String("Parter demon encountered that does not"
            " exist in the COMP of character: %1\n").Arg(
                character->GetUUID().ToString()));
        return;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetDevilData(d->GetType());

    dState->RecalculateStats(definitionManager);

    auto ds = d->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_DATA);
    reply.WriteS32Little(dState->GetEntityID());
    reply.WriteS8(slot);
    reply.WriteS64Little(state->GetObjectID(d->GetUUID()));
    reply.WriteU32Little(d->GetType());
    reply.WriteS16Little(dState->GetMaxHP());
    reply.WriteS16Little(dState->GetMaxMP());
    reply.WriteS16Little(ds->GetHP());
    reply.WriteS16Little(ds->GetMP());
    reply.WriteS64Little(ds->GetXP());
    reply.WriteS8(ds->GetLevel());
    reply.WriteS16Little(def->GetBasic()->GetLNC());

    GetEntityStatsPacketData(reply, ds, dState, false);

    size_t statusEffectCount = d->StatusEffectsCount();
    reply.WriteU32Little(static_cast<uint32_t>(statusEffectCount));
    for(auto effect : d->GetStatusEffects())
    {
        reply.WriteU32Little(effect->GetEffect());
        reply.WriteFloat(state->ToClientTime(
            (ServerTime)effect->GetDuration()));    //Registered as int32?
        reply.WriteU8(effect->GetStack());
    }

    //Learned skill count will always be static
    for(size_t i = 0; i < 8; i++)
    {
        reply.WriteU32Little(d->GetLearnedSkills(i));
    }

    size_t aSkillCount = d->AcquiredSkillsCount();
    reply.WriteU32Little(static_cast<uint32_t>(aSkillCount));
    for(auto aSkill : d->GetAcquiredSkills())
    {
        reply.WriteU32Little(aSkill);
    }

    size_t iSkillCount = d->InheritedSkillsCount();
    reply.WriteU32Little(static_cast<uint32_t>(iSkillCount));
    for(auto iSkill : d->GetInheritedSkills())
    {
        reply.WriteU32Little(iSkill->GetSkill());
        reply.WriteU32Little(static_cast<uint32_t>(
            iSkill->GetProgress() * 100));
    }

    // Unknown
    reply.WriteS64Little(-1);
    reply.WriteS64Little(-1);

    auto zone = server->GetZoneManager()->GetZoneInstance(client);
    auto zoneDef = zone->GetDefinition();

    reply.WriteS32Little((int32_t)zone->GetID());
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteFloat(dState->GetDestinationX());
    reply.WriteFloat(dState->GetDestinationY());
    reply.WriteFloat(dState->GetDestinationRotation());

    reply.WriteU8(0);   //Unknown bool
    reply.WriteU16Little(d->GetAttackSettings());
    reply.WriteU8(0);   //Loyalty?
    reply.WriteU16Little(d->GetGrowthType());
    reply.WriteU8(d->GetLocked() ? 1 : 0);

    // Reunion ranks
    for(size_t i = 0; i < 12; i++)
    {
        reply.WriteS8(d->GetReunion(i));
    }

    reply.WriteS8(0);   //Unknown
    reply.WriteS32Little(d->GetSoulPoints());

    reply.WriteS32Little(0);    //Force Gauge?
    for(size_t i = 0; i < 20; i++)
    {
        reply.WriteS32Little(0);    //Force Values?
    }

    //Force Stack?
    for(size_t i = 0; i < 8; i++)
    {
        reply.WriteU16Little(0);
    }

    //Force Stack Pending?
    reply.WriteU16Little(0);

    //Unknown
    reply.WriteU8(12);
    reply.WriteU8(1);

    //Reunion bonuses (12 * 8 ranks)
    for(size_t i = 0; i < 96; i++)
    {
        reply.WriteU8(0);
    }

    //Characteristics panel
    for(size_t i = 0; i < 4; i++)
    {
        reply.WriteS64Little(-1);    //Item object ID
        reply.WriteU32Little(static_cast<uint32_t>(-1));    //Item type
    }

    //Effect length in seconds
    reply.WriteS32Little(0);

    client->SendPacket(reply);
}

void CharacterManager::SendOtherPartnerData(const std::list<std::shared_ptr<
    ChannelClientConnection>> &clients, ClientState *otherState)
{
    if(clients.size() == 0)
    {
        return;
    }

    // Keep track of where client specific times need to be written
    std::unordered_map<uint32_t, ServerTime> timePositions;

    auto zone = mServer.lock()->GetZoneManager()->GetZoneInstance(clients.front());
    auto zoneDef = zone->GetDefinition();

    auto dState = otherState->GetDemonState();
    auto d = dState->GetEntity();
    auto ds = d->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OTHER_PARTNER_DATA);
    reply.WriteS32Little(dState->GetEntityID());
    reply.WriteU32Little(d->GetType());
    reply.WriteS32Little(otherState->GetDemonState()->GetEntityID());
    reply.WriteS16Little(dState->GetMaxHP());
    reply.WriteS16Little(ds->GetHP());
    reply.WriteS8(ds->GetLevel());
    
    size_t statusEffectCount = d->StatusEffectsCount();
    reply.WriteU32Little(static_cast<uint32_t>(statusEffectCount));
    for(auto effect : d->GetStatusEffects())
    {
        reply.WriteU32Little(effect->GetEffect());
        timePositions[reply.Size()] = effect->GetDuration();
        reply.WriteFloat(0.f);
        reply.WriteU8(effect->GetStack());
    }

    // Unknown
    reply.WriteS64Little(-1);
    reply.WriteS64Little(-1);

    reply.WriteS32Little((int32_t)zone->GetID());
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteFloat(dState->GetDestinationX());
    reply.WriteFloat(dState->GetDestinationY());
    reply.WriteFloat(dState->GetDestinationRotation());

    reply.WriteU8(0);   //Unknown bool

    reply.WriteS16Little(0);    //Unknown
    reply.WriteS16Little(0);    //Unknown
    reply.WriteU16Little(0);    //Unknown
    reply.WriteU8(0);   //Unknown

    for(auto client : clients)
    {
        auto state = client->GetClientState();
        for(auto timePair : timePositions)
        {
            reply.Seek(timePair.first);
            reply.WriteFloat(state->ToClientTime(timePair.second));
        }

        client->SendPacket(reply);
    }
}

void CharacterManager::SendCOMPDemonData(const std::shared_ptr<
    ChannelClientConnection>& client,
    int8_t box, int8_t slot, int64_t id)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto comp = cState->GetEntity()->GetCOMP();

    auto d = comp[(size_t)slot].Get();
    if(d == nullptr || state->GetObjectID(d->GetUUID()) != id)
    {
        return;
    }

    auto cs = d->GetCoreStats().Get();
    bool isSummoned = dState->GetEntity() == d;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_COMP_DEMON_DATA);
    reply.WriteS8(box);
    reply.WriteS8(slot);
    reply.WriteS64Little(id);
    reply.WriteU32Little(d->GetType());

    reply.WriteS16Little(cs->GetMaxHP());
    reply.WriteS16Little(cs->GetMaxMP());
    reply.WriteS16Little(cs->GetHP());
    reply.WriteS16Little(cs->GetMP());
    reply.WriteS64Little(cs->GetXP());
    reply.WriteS8(cs->GetLevel());

    GetEntityStatsPacketData(reply, cs, isSummoned ? dState : nullptr, false);

    //Learned skill count will always be static
    reply.WriteS32Little(8);
    for(size_t i = 0; i < 8; i++)
    {
        reply.WriteU32Little(d->GetLearnedSkills(i));
    }

    size_t aSkillCount = d->AcquiredSkillsCount();
    reply.WriteS32Little(static_cast<int32_t>(aSkillCount));
    for(auto aSkill : d->GetAcquiredSkills())
    {
        reply.WriteU32Little(aSkill);
    }

    size_t iSkillCount = d->InheritedSkillsCount();
    reply.WriteS32Little(static_cast<int32_t>(iSkillCount));
    for(auto iSkill : d->GetInheritedSkills())
    {
        reply.WriteU32Little(iSkill->GetSkill());
        reply.WriteS16Little(iSkill->GetProgress());
    }

    /// @todo: Find status effects and figure out what below
    /// here is setting the epitaph flag (both visible in COMP window)

    reply.WriteU16Little(d->GetAttackSettings());
    reply.WriteU8(0);   //Loyalty?
    reply.WriteU16Little(d->GetGrowthType());
    reply.WriteU8(d->GetLocked() ? 1 : 0);

    // Reunion ranks
    for(size_t i = 0; i < 12; i++)
    {
        reply.WriteS8(d->GetReunion(i));
    }

    reply.WriteS8(0);   //Unknown
    reply.WriteS32Little(d->GetSoulPoints());

    reply.WriteS32Little(0);    //Force Gauge?
    for(size_t i = 0; i < 20; i++)
    {
        reply.WriteS32Little(0);    //Force Values?
    }

    //Force Stack?
    for(size_t i = 0; i < 8; i++)
    {
        reply.WriteU16Little(0);
    }

    //Force Stack Pending?
    reply.WriteU16Little(0);

    //Unknown
    reply.WriteU8(0);
    reply.WriteU8(0);

    //Reunion bonuses (12 * 8 ranks)
    for(size_t i = 0; i < 96; i++)
    {
        reply.WriteU8(0);
    }

    //Characteristics panel?
    for(size_t i = 0; i < 4; i++)
    {
        reply.WriteS64Little(0);    //Item object ID?
        reply.WriteU32Little(0);    //Item type?
    }

    //Effect length in seconds remaining
    reply.WriteS32Little(0);

    client->SendPacket(reply);
}

void CharacterManager::SendStatusIcon(const std::shared_ptr<ChannelClientConnection>& client)
{
    /// @todo: implement icons
    uint8_t icon = 0;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_STATUS_ICON);
    reply.WriteU8(0);
    reply.WriteU8(icon);

    client->SendPacket(reply);

    /// @todo: broadcast to other players
}

void CharacterManager::SummonDemon(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int64_t demonID)
{
    StoreDemon(client);

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();

    auto demon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID)));
    if(nullptr == demon || character->GetUUID() != demon->GetCharacter().GetUUID())
    {
        return;
    }

    character->SetActiveDemon(demon);
    dState->SetEntity(demon);
    dState->SetDestinationX(cState->GetDestinationX());
    dState->SetDestinationY(cState->GetDestinationY());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_SUMMONED);
    reply.WriteS64Little(demonID);

    client->SendPacket(reply);

    auto otherClients = mServer.lock()->GetZoneManager()
        ->GetZoneConnections(client, false);
    SendOtherPartnerData(otherClients, state);
}

void CharacterManager::StoreDemon(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();

    auto demon = dState->GetEntity();
    if(nullptr == demon)
    {
        return;
    }

    dState->SetEntity(nullptr);
    character->SetActiveDemon(NULLUUID);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_OBJECT);
    reply.WriteS32Little(dState->GetEntityID());

    client->SendPacket(reply);

    // Remove the entity from other client's games
    reply.Clear();
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_ENTITY);
    reply.WriteS32Little(dState->GetEntityID());

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, reply, false);
}

void CharacterManager::SendItemBoxData(const std::shared_ptr<
    ChannelClientConnection>& client, int64_t boxID)
{
    std::list<uint16_t> allSlots = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                                    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                                    40, 41, 42, 43, 44, 45, 46, 47, 48, 49 };
    SendItemBoxData(client, boxID, allSlots);
}

void CharacterManager::SendItemBoxData(const std::shared_ptr<
    ChannelClientConnection>& client, int64_t boxID,
    const std::list<uint16_t>& slots)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto box = character->GetItemBoxes((size_t)boxID);

    bool updateMode = slots.size() < 50;

    libcomp::Packet reply;
    if(updateMode)
    {
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_UPDATE);
    }
    else
    {
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_BOX);
    }
    reply.WriteS8(box->GetType());
    reply.WriteS64(state->GetObjectID(box->GetUUID()));
    
    if(updateMode)
    {
        reply.WriteU32((uint32_t)slots.size());
    }
    else
    {
        reply.WriteS32(0);  //Unknown
        reply.WriteU16Little(50); // Max Item Count
        reply.WriteS32Little(0); // Unknown

        int32_t usedSlots = 0;
        for(auto item : box->GetItems())
        {
            if(!item.IsNull())
            {
                usedSlots++;
            }
        }

        reply.WriteS32Little(usedSlots);
    }

    auto server = mServer.lock();
    for(uint16_t slot : slots)
    {
        auto item = box->GetItems((size_t)slot);

        if(item.IsNull())
        {
            if(updateMode)
            {
                // Only send blanks when updating slots
                reply.WriteU16Little(slot);
                reply.WriteS64Little(-1);
            }
            continue;
        }
        else
        {
            reply.WriteU16Little(slot);

            int64_t objectID = state->GetObjectID(item.GetUUID());
            if(objectID == 0)
            {
                objectID = server->GetNextObjectID();
                state->SetObjectID(item.GetUUID(), objectID);
            }
            reply.WriteS64Little(objectID);
        }

        reply.WriteU32Little(item->GetType());
        reply.WriteU16Little(item->GetStackSize());
        reply.WriteU16Little(item->GetDurability());
        reply.WriteS8(item->GetMaxDurability());

        reply.WriteS16Little(item->GetTarot());
        reply.WriteS16Little(item->GetSoul());

        for(auto modSlot : item->GetModSlots())
        {
            reply.WriteU16Little(modSlot);
        }

        reply.WriteS32Little(0);    //Unknown
        /*reply.WriteU8(0);   //Unknown
        reply.WriteS16Little(0);   //Unknown
        reply.WriteS16Little(0);   //Unknown
        reply.WriteU8(0); // Failed Item Fuse 0 = OK | 1 = FAIL*/

        auto basicEffect = item->GetBasicEffect();
        if(basicEffect)
        {
            reply.WriteU32Little(basicEffect);
        }
        else
        {
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }

        auto specialEffect = item->GetSpecialEffect();
        if(specialEffect)
        {
            reply.WriteU32Little(specialEffect);
        }
        else
        {
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }

        for(auto bonus : item->GetFuseBonuses())
        {
            reply.WriteS8(bonus);
        }
    }

    client->SendPacket(reply);
}

std::list<std::shared_ptr<objects::Item>> CharacterManager::GetExistingItems(
    const std::shared_ptr<objects::Character>& character,
    uint32_t itemID)
{
    auto itemBox = character->GetItemBoxes(0).Get();

    std::list<std::shared_ptr<objects::Item>> existing;
    for(size_t i = 0; i < 50; i++)
    {
        auto item = itemBox->GetItems(i);
        if(!item.IsNull() && item->GetType() == itemID)
        {
            existing.push_back(item.Get());
        }
    }

    return existing;
}

std::shared_ptr<objects::Item> CharacterManager::GenerateItem(
    uint32_t itemID, uint16_t stackSize)
{
    auto server = mServer.lock();
    auto def = server->GetDefinitionManager()->GetItemData(itemID);
    if(nullptr == def)
    {
        return nullptr;
    }

    auto poss = def->GetPossession();

    auto item = libcomp::PersistentObject::New<
        objects::Item>();

    item->SetType(itemID);
    item->SetStackSize(stackSize);
    item->SetDurability(poss->GetDurability());
    item->SetMaxDurability((int8_t)poss->GetDurability());
    item->Register(item);

    return item;
}

bool CharacterManager::AddRemoveItem(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint32_t itemID,
    uint16_t quantity, bool add, int64_t skillTargetID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto itemBox = character->GetItemBoxes(0).Get();

    auto server = mServer.lock();
    auto db = server->GetWorldDatabase();
    auto def = server->GetDefinitionManager()->GetItemData(itemID);
    if(nullptr == def)
    {
        return false;
    }

    auto existing = GetExistingItems(character, itemID);

    std::list<uint16_t> updatedSlots;
    auto maxStack = def->GetPossession()->GetStackSize();
    if(add)
    {
        uint16_t quantityLeft = quantity;
        /*for(auto item : existing)
        {
            auto free = maxStack - item->GetStackSize();
            if(free > quantityLeft)
            {
                quantityLeft = 0;
            }
            else
            {
                quantityLeft = (uint16_t)(quantityLeft - free);
            }

            if(quantityLeft == 0)
            {
                break;
            }
        }*/

        std::list<size_t> freeSlots;
        for(size_t i = 0; i < 50; i++)
        {
            if(itemBox->GetItems(i).IsNull())
            {
                freeSlots.push_back(i);
            }
        }

        if(quantityLeft <= (freeSlots.size() * maxStack))
        {
            uint16_t added = 0;
            /*for(auto item : existing)
            {
                uint16_t free = (uint16_t)(maxStack - item->GetStackSize());
                if(added < quantity && free > 0)
                {
                    uint16_t delta = (uint16_t)(quantity - added);
                    if(free < delta)
                    {
                        delta = free;
                    }
                    item->SetStackSize((uint16_t)(item->GetStackSize() + delta));
                    updatedSlots.push_back((uint16_t)item->GetBoxSlot());
                    added = (uint16_t)(added + delta);
                }

                if(added == quantity)
                {
                    break;
                }
            }*/

            if(added < quantity)
            {
                for(auto freeSlot : freeSlots)
                {
                    uint16_t delta = maxStack;
                    if((delta + added) > quantity)
                    {
                        delta = (uint16_t)(quantity - added);
                    }
                    added = (uint16_t)(added + delta);

                    auto item = GenerateItem(itemID, delta);
                    item->SetItemBox(itemBox);
                    item->SetBoxSlot((int8_t)freeSlot);
                    
                    if(!item->Insert(db) ||
                        !itemBox->SetItems(freeSlot, item))
                    {
                        return false;
                    }
                    updatedSlots.push_back((uint16_t)freeSlot);

                    if(added == quantity)
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            // Not enough room
            return false;
        }
    }
    else
    {
        // Items should be remove from the end of the list first
        existing.reverse();

        uint16_t quantityLeft = quantity;
        for(auto item : existing)
        {
            if(item->GetStackSize() > quantityLeft)
            {
                quantityLeft = 0;
            }
            else
            {
                quantityLeft = (uint16_t)(quantityLeft - item->GetStackSize());
            }

            if(quantityLeft == 0)
            {
                break;
            }
        }

        if(quantityLeft > 0)
        {
            return false;
        }

        // Remove from the skill target first if its one of the items
        if(skillTargetID > 0)
        {
            auto skillTarget = std::dynamic_pointer_cast<objects::Item>(
                libcomp::PersistentObject::GetObjectByUUID(
                    state->GetObjectUUID(skillTargetID)));
            if(skillTarget != nullptr &&
                std::find(existing.begin(), existing.end(), skillTarget) != existing.end())
            {
                existing.erase(std::find(existing.begin(), existing.end(), skillTarget));
                existing.push_front(skillTarget);
            }
        }

        auto equipType = def->GetBasic()->GetEquipType();

        uint16_t removed = 0;
        for(auto item : existing)
        {
            // Unequip anything we're removing
            if(equipType != objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE &&
                character->GetEquippedItems((size_t)equipType).Get() == item)
            {
                EquipItem(client, state->GetObjectID(item->GetUUID()));
            }

            auto slot = item->GetBoxSlot();
            if(item->GetStackSize() <= (quantity - removed))
            {
                removed = (uint16_t)(removed + item->GetStackSize());
                
                if(!itemBox->SetItems((size_t)slot, NULLUUID) ||
                    !itemBox->Update(db) || !item->Delete(db))
                {
                    return false;
                }
            }
            else
            {
                item->SetStackSize((uint16_t)(item->GetStackSize() -
                    (quantity - removed)));
                if(!item->Update(db))
                {
                    return false;
                }
                removed = quantity;
            }
            updatedSlots.push_back((uint16_t)slot);

            if(removed == quantity)
            {
                break;
            }
        }
    }

    SendItemBoxData(client, 0, updatedSlots);

    if(!itemBox->Update(db))
    {
        return false;
    }

    return true;
}

void CharacterManager::EquipItem(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int64_t itemID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto equip = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(
            state->GetObjectUUID(itemID)));

    if(nullptr == equip ||
        equip->GetItemBox().Get() != character->GetItemBoxes(0).Get())
    {
        return;
    }

    auto slot = objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE;

    auto server = mServer.lock();
    auto def = server->GetDefinitionManager()->GetItemData(equip->GetType());
    if (nullptr != def)
    {
        slot = def->GetBasic()->GetEquipType();
    }

    if(slot == objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE)
    {
        return;
    }

    bool unequip = false;
    auto equipSlot = character->GetEquippedItems((size_t)slot);
    if(equipSlot.Get() == equip)
    {
        equipSlot.SetReference(nullptr);
        unequip = true;
    }
    else
    {
        equipSlot.SetReference(equip);
    }
    character->SetEquippedItems((size_t)slot, equipSlot);

    cState->RecalculateStats(server->GetDefinitionManager());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EQUIPMENT_CHANGED);
    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteU8((uint8_t)slot);

    if(unequip)
    {
        reply.WriteS64Little(-1);
        reply.WriteU32Little(static_cast<uint32_t>(-1));
    }
    else
    {
        reply.WriteS64Little(state->GetObjectID(equip->GetUUID()));
        reply.WriteU32Little(equip->GetType());
    }

    auto cs = character->GetCoreStats().Get();

    // Return updated stats in a format not like that seen in
    // GetEntityStatsPacketData
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSTR() - cs->GetSTR()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetMAGIC() - cs->GetMAGIC()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetVIT() - cs->GetVIT()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetINTEL() - cs->GetINTEL()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSPEED() - cs->GetSPEED()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetLUCK() - cs->GetLUCK()));
    reply.WriteS16Little(cs->GetMaxHP());
    reply.WriteS16Little(cs->GetMaxMP());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetCLSR() - cs->GetCLSR()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetLNGR() - cs->GetLNGR()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSPELL() - cs->GetSPELL()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSUPPORT() - cs->GetSUPPORT()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetPDEF() - cs->GetPDEF()));
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetMDEF() - cs->GetMDEF()));
    reply.WriteS16Little(cs->GetCLSR());
    reply.WriteS16Little(cs->GetLNGR());
    reply.WriteS16Little(cs->GetSPELL());
    reply.WriteS16Little(cs->GetSUPPORT());
    reply.WriteS16Little(cs->GetPDEF());
    reply.WriteS16Little(cs->GetMDEF());

    auto db = server->GetWorldDatabase();

    (void)character->Update(db);

    server->GetZoneManager()->BroadcastPacket(client, reply);
}

void CharacterManager::UpdateLNC(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int16_t lnc)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    character->SetLNC(lnc);

    auto server = mServer.lock();
    auto db = server->GetWorldDatabase();

    (void)character->Update(db);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LNC_POINTS);
    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteS16Little(character->GetLNC());

    client->SendPacket(reply);
}

std::shared_ptr<objects::Demon> CharacterManager::ContractDemon(
    const std::shared_ptr<objects::Character>& character,
    const std::shared_ptr<objects::MiDevilData>& demonData,
    const std::shared_ptr<objects::Demon>& demon)
{
    //Was valid demon data supplied?
    if(nullptr == demonData)
    {
        return nullptr;
    }

    //Find the next empty slot to add the demon to
    int8_t compSlot = -1;
    for(size_t i = 0; i < 10; i++)
    {
        if(character->GetCOMP(i).IsNull())
        {
            compSlot = (int8_t)i;
            break;
        }
    }

    //Return false if no slot is open
    if(compSlot == -1)
    {
        return nullptr;
    }

    std::shared_ptr<objects::Demon> d = nullptr;
    std::shared_ptr<objects::EntityStats> ds = nullptr;
    if(nullptr != demon)
    {
        //Copy the demon being passed in
        d = std::shared_ptr<objects::Demon>(new objects::Demon(*demon.get()));
        ds = std::shared_ptr<objects::EntityStats>(
            new objects::EntityStats(*d->GetCoreStats().Get()));
    }
    else
    {
        //Create a new demon from it's defaults
        auto growth = demonData->GetGrowth();

        d = std::shared_ptr<objects::Demon>(new objects::Demon);
        d->SetType(demonData->GetBasic()->GetID());

        ds = libcomp::PersistentObject::New<
            objects::EntityStats>();
        ds->SetLevel(static_cast<int8_t>(growth->GetBaseLevel()));

        CalculateDemonBaseStats(ds, demonData);
        d->SetLearnedSkills(growth->GetSkills());
    }

    d->SetLocked(false);
    d->SetAccount(character->GetAccount());
    d->SetCharacter(character);

    d->Register(d);
    ds->Register(ds);
    d->SetCoreStats(ds);
    ds->SetEntity(std::dynamic_pointer_cast<
        libcomp::PersistentObject>(d));
    character->SetCOMP((size_t)compSlot, d);

    auto server = mServer.lock();
    auto db = server->GetWorldDatabase();
    if(!ds->Insert(db) || !d->Insert(db))
    {
        return nullptr;
    }

    return d;
}

void CharacterManager::ExperienceGain(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint64_t xpGain, int32_t entityID)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    auto eState = state->GetEntityState(entityID);
    if(nullptr == eState)
    {
        return;
    }

    bool isDemon = false;
    std::shared_ptr<objects::MiDevilData> demonData;
    if(eState == dState)
    {
        isDemon = true;
        demonData = definitionManager->GetDevilData(demon->GetType());
    }

    auto stats = eState->GetCoreStats();
    auto level = stats->GetLevel();
    if(level == 99)
    {
        return;
    }

    int64_t xpDelta = stats->GetXP() + (int64_t)xpGain;
    while(level < 99 && xpDelta >= (int64_t)libcomp::LEVEL_XP_REQUIREMENTS[level])
    {
        xpDelta = xpDelta - (int64_t)libcomp::LEVEL_XP_REQUIREMENTS[level];

        level++;

        stats->SetLevel(level);

        libcomp::Packet reply;
        if(isDemon)
        {
            auto growth = demonData->GetGrowth();
            for(auto acSkill : growth->GetAcquisitionSkills())
            {
                if(acSkill->GetLevel() == (uint32_t)level)
                {
                    demon->AppendAcquiredSkills(acSkill->GetID());
                }
            }

            CalculateDemonBaseStats(stats, demonData);
            dState->RecalculateStats(definitionManager);
            stats->SetHP(dState->GetMaxHP());
            stats->SetMP(dState->GetMaxMP());

            reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_LEVEL_UP);
            reply.WriteS32Little(entityID);
            reply.WriteS8(level);
            reply.WriteS64Little(state->GetObjectID(demon->GetUUID()));
            GetEntityStatsPacketData(reply, stats, dState, true);

            size_t aSkillCount = demon->AcquiredSkillsCount();
            reply.WriteU32Little(static_cast<uint32_t>(aSkillCount));
            for(auto aSkill : demon->GetAcquiredSkills())
            {
                reply.WriteU32Little(aSkill);
            }
        }
        else
        {
            CalculateCharacterBaseStats(stats);
            cState->RecalculateStats(definitionManager);
            stats->SetHP(cState->GetMaxHP());
            stats->SetMP(cState->GetMaxMP());

            int32_t points = (int32_t)(floorl((float)level / 5) + 2);
            character->SetPoints(character->GetPoints() + points);

            reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHARACTER_LEVEL_UP);
            reply.WriteS32Little(entityID);
            reply.WriteS32(0);  //Unknown
            reply.WriteS8(level);
            reply.WriteS64(xpDelta);
            reply.WriteS16Little(stats->GetHP());
            reply.WriteS16Little(stats->GetMP());
            reply.WriteS32Little(points);
        }

        server->GetZoneManager()->BroadcastPacket(client, reply, true);
    }

    stats->SetXP(xpDelta);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_XP_UPDATE);
    reply.WriteS32Little(entityID);
    reply.WriteS64(xpDelta);
    reply.WriteS32Little((int32_t)xpGain);
    reply.WriteS32Little(0);    //Unknown

    /// @todo: send to all players in the zone?
    client->SendPacket(reply);
}

void CharacterManager::LevelUp(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int8_t level, int32_t entityID)
{
    if(level < 2 || level > 99)
    {
        return;
    }

    auto state = client->GetClientState();
    auto eState = state->GetEntityState(entityID);
    if(nullptr == eState)
    {
        return;
    }

    auto stats = eState->GetCoreStats();
    uint64_t xpGain = 0;
    for(int8_t i = stats->GetLevel(); i < level; i++)
    {
        if(xpGain == 0)
        {
            xpGain += libcomp::LEVEL_XP_REQUIREMENTS[i] - (uint64_t)stats->GetXP();
        }
        else
        {
            xpGain += libcomp::LEVEL_XP_REQUIREMENTS[i];
        }
    }

    ExperienceGain(client, xpGain, entityID);
}

void CharacterManager::UpdateExpertise(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint32_t skillID)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto skill = definitionManager->GetSkillData(skillID);
    if(nullptr == skill)
    {
        LOG_WARNING(libcomp::String("Unknown skill ID encountered in"
            " UpdateExpertise: %1").Arg(skillID));
        return;
    }

    std::list<std::pair<int8_t, int32_t>> updated;
    for(auto expertGrowth : skill->GetExpertGrowth())
    {
        auto expertise = character->GetExpertises(expertGrowth->GetExpertiseID()).Get();

        // If it hasn't been created, it is disabled
        if(nullptr == expertise || expertise->GetDisabled()) continue;

        auto expDef = definitionManager->GetExpertClassData(expertGrowth->GetExpertiseID());

        // Should never happen
        if(nullptr == expDef) continue;

        int32_t maxPoints = (expDef->GetMaxClass() * 100 * 1000)
            + (expDef->GetMaxRank() * 100 * 100);

        int32_t points = expertise->GetPoints();
        int8_t currentRank = (int8_t)floorl((float)points * 0.0001f);

        if(points == maxPoints) continue;

        // Calculate the floating point gain
        /// @todo: validate
        float fGain = static_cast<float>(3954.482803f /
            (((float)expertise->GetPoints() * 0.01f) + 158.1808409f)
            * expertGrowth->GetGrowthRate());

        points += (int32_t)(fGain * 100.0f + 0.5f);

        if(points > maxPoints)
        {
            points = maxPoints;
        }

        expertise->SetPoints(points);
        updated.push_back(std::pair<int8_t, int32_t>((int8_t)expDef->GetID(), points));

        int8_t newRank = (int8_t)((float)points * 0.0001f);
        if(currentRank != newRank)
        {
            libcomp::Packet reply;
            reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EXPERTISE_RANK_UP);
            reply.WriteS32Little(cState->GetEntityID());
            reply.WriteS8((int8_t)expDef->GetID());
            reply.WriteS8(newRank);

            server->GetZoneManager()->BroadcastPacket(client, reply);
        }
    }

    if(updated.size() > 0)
    {
        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EXPERTISE_POINT_UPDATE);
        reply.WriteS32Little(cState->GetEntityID());
        reply.WriteS32Little((int32_t)updated.size());
        for(auto update : updated)
        {
            reply.WriteS8(update.first);
            reply.WriteS32Little(update.second);
        }

        client->SendPacket(reply);
    }
}

void CharacterManager::CalculateCharacterBaseStats(const std::shared_ptr<objects::EntityStats>& cs)
{
    std::unordered_map<uint8_t, int16_t> stats = GetCharacterBaseStatMap(cs);

    CalculateDependentStats(stats, cs->GetLevel(), false);

    cs->SetMaxHP(stats[libcomp::CORRECT_MAXHP]);
    cs->SetMaxMP(stats[libcomp::CORRECT_MAXMP]);
    cs->SetCLSR(stats[libcomp::CORRECT_CLSR]);
    cs->SetLNGR(stats[libcomp::CORRECT_LNGR]);
    cs->SetSPELL(stats[libcomp::CORRECT_SPELL]);
    cs->SetSUPPORT(stats[libcomp::CORRECT_SUPPORT]);
    cs->SetPDEF(stats[libcomp::CORRECT_PDEF]);
    cs->SetMDEF(stats[libcomp::CORRECT_MDEF]);
}

void CharacterManager::CalculateDemonBaseStats(const std::shared_ptr<
    objects::EntityStats>& ds, const std::shared_ptr<
    objects::MiDevilData>& demonData)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto basicData = demonData->GetBasic();
    auto battleData = demonData->GetBattleData();
    auto growthData = demonData->GetGrowth();
    auto baseLevelRate = definitionManager->GetDevilLVUpRateData(
        demonData->GetGrowth()->GetGrowthType());

    int8_t level = ds->GetLevel();
    uint8_t boostLevel = static_cast<uint8_t>((level + 3) / 4);
    uint8_t boostStage = static_cast<uint8_t>((boostLevel - 1) / 5);
    
	/*
	 * A | 1
	 * A | 5,  9,  13, 17, 21,
	 * B | 25, 29, 33, 37, 41,
	 * C | 45, 49, 53, 57, 61,
	 * D | 65, 69, 73, 77, 81,
	 * D | 85, 89, 93, 97
	 */

    std::unordered_map<uint8_t, int16_t> stats;
    stats[libcomp::CORRECT_STR] = battleData->GetCorrect(libcomp::CORRECT_STR);
    stats[libcomp::CORRECT_MAGIC] = battleData->GetCorrect(libcomp::CORRECT_MAGIC);
    stats[libcomp::CORRECT_VIT] = battleData->GetCorrect(libcomp::CORRECT_VIT);
    stats[libcomp::CORRECT_INTEL] = battleData->GetCorrect(libcomp::CORRECT_INTEL);
    stats[libcomp::CORRECT_SPEED] = battleData->GetCorrect(libcomp::CORRECT_SPEED);
    stats[libcomp::CORRECT_LUCK] = battleData->GetCorrect(libcomp::CORRECT_LUCK);
    stats[libcomp::CORRECT_MAXHP] = battleData->GetCorrect(libcomp::CORRECT_MAXHP);
    stats[libcomp::CORRECT_MAXMP] = battleData->GetCorrect(libcomp::CORRECT_MAXMP);
    stats[libcomp::CORRECT_CLSR] = battleData->GetCorrect(libcomp::CORRECT_CLSR);
    stats[libcomp::CORRECT_LNGR] = battleData->GetCorrect(libcomp::CORRECT_LNGR);
    stats[libcomp::CORRECT_SPELL] = battleData->GetCorrect(libcomp::CORRECT_SPELL);
    stats[libcomp::CORRECT_SUPPORT] = battleData->GetCorrect(libcomp::CORRECT_SUPPORT);
    stats[libcomp::CORRECT_PDEF] = battleData->GetCorrect(libcomp::CORRECT_PDEF);
    stats[libcomp::CORRECT_MDEF] = battleData->GetCorrect(libcomp::CORRECT_MDEF);

    switch(boostStage)
    {
        case 0:
        case 1:
            // stats = A * boostLevel;
            BoostStats(stats, baseLevelRate->GetLevelUpData(0), boostLevel);
            break;
        case 2:
            // stats = A * 6 + B * (boostLevel - 6);
            BoostStats(stats, baseLevelRate->GetLevelUpData(0), 6);
            BoostStats(stats, baseLevelRate->GetLevelUpData(1), boostLevel - 6);
            break;
        case 3:
            // stats = A * 6 + B * 5 + C * (boostLevel - 11);
            BoostStats(stats, baseLevelRate->GetLevelUpData(0), 6);
            BoostStats(stats, baseLevelRate->GetLevelUpData(1), 5);
            BoostStats(stats, baseLevelRate->GetLevelUpData(2), boostLevel - 11);
            break;
        case 4:
            // stats = A * 6 + B * 5 + C * 5 + D * (boostLevel - 16);
            BoostStats(stats, baseLevelRate->GetLevelUpData(0), 6);
            BoostStats(stats, baseLevelRate->GetLevelUpData(1), 5);
            BoostStats(stats, baseLevelRate->GetLevelUpData(2), 5);
            BoostStats(stats, baseLevelRate->GetLevelUpData(3), boostLevel - 16);
            break;
        default:
            break;
    }

    /// @todo: apply reunion and loyalty boosts

    CalculateDependentStats(stats, level, true);

    // Set anything that overflowed as int16_t max
    for(auto stat : stats)
    {
        if(stat.second < 0)
        {
            stat.second = 0x7FFF;
        }
    }

    ds->SetMaxHP(stats[libcomp::CORRECT_MAXHP]);
    ds->SetMaxMP(stats[libcomp::CORRECT_MAXMP]);
    ds->SetHP(stats[libcomp::CORRECT_MAXHP]);
    ds->SetMP(stats[libcomp::CORRECT_MAXMP]);
    ds->SetSTR(stats[libcomp::CORRECT_STR]);
    ds->SetMAGIC(stats[libcomp::CORRECT_MAGIC]);
    ds->SetVIT(stats[libcomp::CORRECT_VIT]);
    ds->SetINTEL(stats[libcomp::CORRECT_INTEL]);
    ds->SetSPEED(stats[libcomp::CORRECT_SPEED]);
    ds->SetLUCK(stats[libcomp::CORRECT_LUCK]);
    ds->SetCLSR(stats[libcomp::CORRECT_CLSR]);
    ds->SetLNGR(stats[libcomp::CORRECT_LNGR]);
    ds->SetSPELL(stats[libcomp::CORRECT_SPELL]);
    ds->SetSUPPORT(stats[libcomp::CORRECT_SUPPORT]);
    ds->SetPDEF(stats[libcomp::CORRECT_PDEF]);
    ds->SetMDEF(stats[libcomp::CORRECT_MDEF]);
}

std::unordered_map<uint8_t, int16_t> CharacterManager::GetCharacterBaseStatMap(
    const std::shared_ptr<objects::EntityStats>& cs)
{
    std::unordered_map<uint8_t, int16_t> stats;
    stats[libcomp::CORRECT_STR] = cs->GetSTR();
    stats[libcomp::CORRECT_MAGIC] = cs->GetMAGIC();
    stats[libcomp::CORRECT_VIT] = cs->GetVIT();
    stats[libcomp::CORRECT_INTEL] = cs->GetINTEL();
    stats[libcomp::CORRECT_SPEED] = cs->GetSPEED();
    stats[libcomp::CORRECT_LUCK] = cs->GetLUCK();
    stats[libcomp::CORRECT_MAXHP] = 70;
    stats[libcomp::CORRECT_MAXMP] = 10;
    stats[libcomp::CORRECT_CLSR] = 0;
    stats[libcomp::CORRECT_LNGR] = 0;
    stats[libcomp::CORRECT_SPELL] = 0;
    stats[libcomp::CORRECT_SUPPORT] = 0;
    stats[libcomp::CORRECT_PDEF] = 0;
    stats[libcomp::CORRECT_MDEF] = 0;
    return stats;
}

void CharacterManager::CalculateDependentStats(
    std::unordered_map<uint8_t, int16_t>& stats, int8_t level, bool isDemon)
{
    /// @todo: fix: close but not quite right
    if(isDemon)
    {
        // Round up each part
        stats[libcomp::CORRECT_MAXHP] = (int16_t)(stats[libcomp::CORRECT_MAXHP] +
            (int16_t)ceill(stats[libcomp::CORRECT_MAXHP] * 0.03 * level) +
            (int16_t)ceill(stats[libcomp::CORRECT_STR] * 0.3) +
            (int16_t)ceill(((stats[libcomp::CORRECT_MAXHP] * 0.01) + 0.5) * stats[libcomp::CORRECT_VIT]));
        stats[libcomp::CORRECT_MAXMP] = (int16_t)(stats[libcomp::CORRECT_MAXMP] +
            (int16_t)ceill(stats[libcomp::CORRECT_MAXMP] * 0.03 * level) +
            (int16_t)ceill(stats[libcomp::CORRECT_MAGIC] * 0.3) +
            (int16_t)ceill(((stats[libcomp::CORRECT_MAXMP] * 0.01) + 0.5) * stats[libcomp::CORRECT_INTEL]));

        // Round the result, adjusting by 0.5
        stats[libcomp::CORRECT_CLSR] = (int16_t)(stats[libcomp::CORRECT_CLSR] +
            (int16_t)roundl(((stats[libcomp::CORRECT_STR]) * 0.5) + 0.5 + (level * 0.1)));
        stats[libcomp::CORRECT_LNGR] = (int16_t)(stats[libcomp::CORRECT_LNGR] +
            (int16_t)roundl(((stats[libcomp::CORRECT_SPEED]) * 0.5) + 0.5 + (level * 0.1)));
        stats[libcomp::CORRECT_SPELL] = (int16_t)(stats[libcomp::CORRECT_SPELL] +
            (int16_t)roundl(((stats[libcomp::CORRECT_MAGIC]) * 0.5) + 0.5 + (level * 0.1)));
        stats[libcomp::CORRECT_SUPPORT] = (int16_t)(stats[libcomp::CORRECT_SUPPORT] +
            (int16_t)roundl(((stats[libcomp::CORRECT_INTEL]) * 0.5) + 0.5 + (level * 0.1)));
        stats[libcomp::CORRECT_PDEF] = (int16_t)(stats[libcomp::CORRECT_PDEF] +
            (int16_t)roundl(((stats[libcomp::CORRECT_VIT]) * 0.1) + 0.5 + (level * 0.1)));
        stats[libcomp::CORRECT_MDEF] = (int16_t)(stats[libcomp::CORRECT_MDEF] +
            (int16_t)roundl(((stats[libcomp::CORRECT_INTEL]) * 0.1) + 0.5 + (level * 0.1)));
    }
    else
    {
        // Round each part
        stats[libcomp::CORRECT_MAXHP] = (int16_t)(stats[libcomp::CORRECT_MAXHP] +
            (int16_t)roundl(stats[libcomp::CORRECT_MAXHP] * 0.03 * level) +
            (int16_t)roundl(stats[libcomp::CORRECT_STR] * 0.3) +
            (int16_t)roundl(((stats[libcomp::CORRECT_MAXHP] * 0.01) + 0.5) * stats[libcomp::CORRECT_VIT]));
        stats[libcomp::CORRECT_MAXMP] = (int16_t)(stats[libcomp::CORRECT_MAXMP] +
            (int16_t)roundl(stats[libcomp::CORRECT_MAXMP] * 0.03 * level) +
            (int16_t)roundl(stats[libcomp::CORRECT_MAGIC] * 0.3) +
            (int16_t)roundl(((stats[libcomp::CORRECT_MAXMP] * 0.01) + 0.5) * stats[libcomp::CORRECT_INTEL]));

        // Round the results down
        stats[libcomp::CORRECT_CLSR] = (int16_t)(stats[libcomp::CORRECT_CLSR] +
            (int16_t)floorl(((stats[libcomp::CORRECT_STR]) * 0.5) + (level * 0.1)));
        stats[libcomp::CORRECT_LNGR] = (int16_t)(stats[libcomp::CORRECT_LNGR] +
            (int16_t)floorl(((stats[libcomp::CORRECT_SPEED]) * 0.5) + (level * 0.1)));
        stats[libcomp::CORRECT_SPELL] = (int16_t)(stats[libcomp::CORRECT_SPELL] +
            (int16_t)floorl(((stats[libcomp::CORRECT_MAGIC]) * 0.5) + (level * 0.1)));
        stats[libcomp::CORRECT_SUPPORT] = (int16_t)(stats[libcomp::CORRECT_SUPPORT] +
            (int16_t)floorl(((stats[libcomp::CORRECT_INTEL]) * 0.5) + (level * 0.1)));
        stats[libcomp::CORRECT_PDEF] = (int16_t)(stats[libcomp::CORRECT_PDEF] +
            (int16_t)floorl(((stats[libcomp::CORRECT_VIT]) * 0.1) + (level * 0.1)));
        stats[libcomp::CORRECT_MDEF] = (int16_t)(stats[libcomp::CORRECT_MDEF] +
            (int16_t)floorl(((stats[libcomp::CORRECT_INTEL]) * 0.1) + (level * 0.1)));
    }
}

void CharacterManager::GetCOMPSlotPacketData(libcomp::Packet& p,
    const std::shared_ptr<channel::ChannelClientConnection>& client, size_t slot)
{
    auto state = client->GetClientState();
    auto demon = state->GetCharacterState()->GetEntity()->GetCOMP(slot).Get();

    p.WriteS8(static_cast<int8_t>(slot)); // Slot
    p.WriteS64Little(nullptr != demon
        ? state->GetObjectID(demon->GetUUID()) : -1);

    if(nullptr != demon)
    {
        auto cs = demon->GetCoreStats();
        p.WriteU32Little(demon->GetType());
        p.WriteS16Little(cs->GetMaxHP());
        p.WriteS16Little(cs->GetMaxMP());
        p.WriteS16Little(cs->GetHP());
        p.WriteS16Little(cs->GetMP());
        p.WriteS8(cs->GetLevel());
        p.WriteU8(demon->GetLocked() ? 1 : 0);

        size_t statusEffectCount = demon->StatusEffectsCount();
        p.WriteS32Little(static_cast<int32_t>(statusEffectCount));
        for(auto effect : demon->GetStatusEffects())
        {
            p.WriteU32Little(effect->GetEffect());
        }

        p.WriteS8(0);   //Unknown

        //Epitaph/Mitama fusion flag
        p.WriteS8(0);

        //Effect length in seconds
        p.WriteS32Little(0);
    }
}

void CharacterManager::GetEntityStatsPacketData(libcomp::Packet& p,
    const std::shared_ptr<objects::EntityStats>& coreStats,
    const std::shared_ptr<ActiveEntityState>& state,
    bool boostFormat)
{
    auto baseOnly = state == nullptr;

    p.WriteS16Little(coreStats->GetSTR());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetSTR() - coreStats->GetSTR())));
    p.WriteS16Little(coreStats->GetMAGIC());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetMAGIC() - coreStats->GetMAGIC())));
    p.WriteS16Little(coreStats->GetVIT());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetVIT() - coreStats->GetVIT())));
    p.WriteS16Little(coreStats->GetINTEL());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetINTEL() - coreStats->GetINTEL())));
    p.WriteS16Little(coreStats->GetSPEED());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetSPEED() - coreStats->GetSPEED())));
    p.WriteS16Little(coreStats->GetLUCK());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetLUCK() - coreStats->GetLUCK())));

    if(boostFormat)
    {
        p.WriteS16Little(static_cast<int16_t>(
            baseOnly ? coreStats->GetMaxHP() : state->GetMaxHP()));
        p.WriteS16Little(static_cast<int16_t>(
            baseOnly ? coreStats->GetMaxMP() : state->GetMaxMP()));
    }

    p.WriteS16Little(coreStats->GetCLSR());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetCLSR() - coreStats->GetCLSR())));
    p.WriteS16Little(coreStats->GetLNGR());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetLNGR() - coreStats->GetLNGR())));
    p.WriteS16Little(coreStats->GetSPELL());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetSPELL() - coreStats->GetSPELL())));
    p.WriteS16Little(coreStats->GetSUPPORT());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetSUPPORT() - coreStats->GetSUPPORT())));
    p.WriteS16Little(coreStats->GetPDEF());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetPDEF() - coreStats->GetPDEF())));
    p.WriteS16Little(coreStats->GetMDEF());
    p.WriteS16Little(static_cast<int16_t>(
        baseOnly ? 0 : (state->GetMDEF() - coreStats->GetMDEF())));
}

void CharacterManager::BoostStats(std::unordered_map<uint8_t, int16_t>& stats,
    const std::shared_ptr<objects::MiDevilLVUpData>& data, int boostLevel)
{
    stats[libcomp::CORRECT_STR] = (int16_t)(stats[libcomp::CORRECT_STR] +
        (int16_t)(data->GetSTR() * boostLevel));
    stats[libcomp::CORRECT_MAGIC] = (int16_t)(stats[libcomp::CORRECT_MAGIC] +
        (int16_t)(data->GetMAGIC() * boostLevel));
    stats[libcomp::CORRECT_VIT] = (int16_t)(stats[libcomp::CORRECT_VIT] +
        (int16_t)(data->GetVIT() * boostLevel));
    stats[libcomp::CORRECT_INTEL] = (int16_t)(stats[libcomp::CORRECT_INTEL] +
        (int16_t)(data->GetINTEL() * boostLevel));
    stats[libcomp::CORRECT_SPEED] = (int16_t)(stats[libcomp::CORRECT_SPEED] +
        (int16_t)(data->GetSPEED() * boostLevel));
    stats[libcomp::CORRECT_LUCK] = (int16_t)(stats[libcomp::CORRECT_LUCK] +
        (int16_t)(data->GetLUCK() * boostLevel));
}
