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
#include <Randomizer.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "AIState.h"
#include "ChannelServer.h"

// object Includes
#include <AccountWorldData.h>
#include <CharacterProgress.h>
#include <Clan.h>
#include <DemonBox.h>
#include <Expertise.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <MiAcquisitionSkillData.h>
#include <MiCancelData.h>
#include <MiDevilBattleData.h>
#include <MiDevilData.h>
#include <MiDevilFamiliarityData.h>
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
#include <MiStatusData.h>
#include <ServerZone.h>
#include <StatusEffect.h>
#include <TradeSession.h>

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

    GetEntityStatsPacketData(reply, cs, cState, 0);

    reply.WriteS16(-5600); // Unknown
    reply.WriteS16(5600); // Unknown

    auto statusEffects = cState->GetCurrentStatusEffectStates(
        mServer.lock()->GetDefinitionManager());

    reply.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for(auto ePair : statusEffects)
    {
        reply.WriteU32Little(ePair.first->GetEffect());
        reply.WriteS32Little((int32_t)ePair.second);
        reply.WriteU8(ePair.first->GetStack());
    }

    auto skills = cState->GetCurrentSkills();
    reply.WriteU32(static_cast<uint32_t>(skills.size()));
    for(auto skill : skills)
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

    auto zone = cState->GetZone();
    auto zoneDef = zone->GetDefinition();

    reply.WriteS32Little((int32_t)zone->GetID());
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteFloat(cState->GetDestinationX());
    reply.WriteFloat(cState->GetDestinationY());
    reply.WriteFloat(cState->GetDestinationRotation());

    reply.WriteU8(0);   //Unknown bool

    // Homepoint
    reply.WriteS32Little((int32_t)c->GetHomepointZone());
    reply.WriteFloat(c->GetHomepointX());
    reply.WriteFloat(c->GetHomepointY());

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

    auto cState = otherState->GetCharacterState();
    auto c = cState->GetEntity();
    auto cs = c->GetCoreStats().Get();

    auto zone = cState->GetZone();
    auto zoneDef = zone->GetDefinition();

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

    auto statusEffects = cState->GetCurrentStatusEffectStates(
        mServer.lock()->GetDefinitionManager());

    reply.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for(auto ePair : statusEffects)
    {
        reply.WriteU32Little(ePair.first->GetEffect());
        reply.WriteS32Little((int32_t)ePair.second);
        reply.WriteU8(ePair.first->GetStack());
    }

    // Unknown
    reply.WriteS64Little(-1);
    reply.WriteS64Little(-1);

    reply.WriteS32Little((int32_t)zone->GetID());
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteFloat(cState->GetDestinationX());
    reply.WriteFloat(cState->GetDestinationY());
    reply.WriteFloat(cState->GetDestinationRotation());

    reply.WriteU8(otherState->GetAcceptRevival() ? 1 : 0);
    reply.WriteS8(0);   // Unknown

    auto clan = c->GetClan().Get();
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        clan ? clan->GetName() : "", true);
    reply.WriteS8(otherState->GetStatusIcon());
    reply.WriteS8(0);   // Unknown

    if(clan)
    {
        reply.WriteS8(clan->GetLevel());
        reply.WriteU8(clan->GetEmblemBase());
        reply.WriteU8(clan->GetEmblemSymbol());
        reply.WriteU8(clan->GetEmblemColorR1());
        reply.WriteU8(clan->GetEmblemColorG1());
        reply.WriteU8(clan->GetEmblemColorB1());
        reply.WriteU8(clan->GetEmblemColorR2());
        reply.WriteU8(clan->GetEmblemColorG2());
        reply.WriteU8(clan->GetEmblemColorB2());
    }
    else
    {
        reply.WriteBlank(9);
    }

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

    ChannelClientConnection::BroadcastPacket(clients, reply);
}

void CharacterManager::SendPartnerData(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();

    auto d = dState->GetEntity();
    if(d == nullptr)
    {
        return;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetDevilData(d->GetType());
    auto ds = d->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_DATA);
    reply.WriteS32Little(dState->GetEntityID());
    reply.WriteS8(d->GetBoxSlot());
    reply.WriteS64Little(state->GetObjectID(d->GetUUID()));
    reply.WriteU32Little(d->GetType());
    reply.WriteS16Little(dState->GetMaxHP());
    reply.WriteS16Little(dState->GetMaxMP());
    reply.WriteS16Little(ds->GetHP());
    reply.WriteS16Little(ds->GetMP());
    reply.WriteS64Little(ds->GetXP());
    reply.WriteS8(ds->GetLevel());
    reply.WriteS16Little(def->GetBasic()->GetLNC());

    GetEntityStatsPacketData(reply, ds, dState, 0);

    auto statusEffects = dState->GetCurrentStatusEffectStates(
        mServer.lock()->GetDefinitionManager());

    reply.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for(auto ePair : statusEffects)
    {
        reply.WriteU32Little(ePair.first->GetEffect());
        reply.WriteS32Little((int32_t)ePair.second);
        reply.WriteU8(ePair.first->GetStack());
    }

    //Learned skill count will always be static
    for(size_t i = 0; i < 8; i++)
    {
        auto skillID = d->GetLearnedSkills(i);
        reply.WriteU32Little(skillID == 0 ? (uint32_t)-1 : skillID);
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
        reply.WriteU32Little((uint32_t)iSkill->GetProgress());
    }

    // Unknown
    reply.WriteS64Little(-1);
    reply.WriteS64Little(-1);

    auto zone = dState->GetZone();
    auto zoneDef = zone->GetDefinition();

    reply.WriteS32Little((int32_t)zone->GetID());
    reply.WriteS32Little((int32_t)zoneDef->GetID());
    reply.WriteFloat(dState->GetDestinationX());
    reply.WriteFloat(dState->GetDestinationY());
    reply.WriteFloat(dState->GetDestinationRotation());

    reply.WriteU8(0);   //Unknown bool
    reply.WriteU16Little(d->GetAttackSettings());
    reply.WriteU8(0);   //Loyalty?
    reply.WriteU16Little(d->GetFamiliarity());
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

    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Mitama type

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

    auto dState = otherState->GetDemonState();
    auto d = dState->GetEntity();
    auto ds = d->GetCoreStats().Get();

    auto zone = dState->GetZone();
    auto zoneDef = zone->GetDefinition();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_OTHER_PARTNER_DATA);
    reply.WriteS32Little(dState->GetEntityID());
    reply.WriteU32Little(d->GetType());
    reply.WriteS32Little(otherState->GetCharacterState()->GetEntityID());
    reply.WriteS16Little(dState->GetMaxHP());
    reply.WriteS16Little(ds->GetHP());
    reply.WriteS8(ds->GetLevel());

    auto statusEffects = dState->GetCurrentStatusEffectStates(
        mServer.lock()->GetDefinitionManager());

    reply.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for(auto ePair : statusEffects)
    {
        reply.WriteU32Little(ePair.first->GetEffect());
        reply.WriteS32Little((int32_t)ePair.second);
        reply.WriteU8(ePair.first->GetStack());
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
    reply.WriteU16Little(d->GetFamiliarity());
    reply.WriteU8(0);   //Unknown

    ChannelClientConnection::BroadcastPacket(clients, reply);
}

void CharacterManager::SendDemonData(const std::shared_ptr<
    ChannelClientConnection>& client,
    int8_t boxID, int8_t slot, int64_t demonID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto box = GetDemonBox(state, boxID);

    auto d = box->GetDemons((size_t)slot).Get();
    if(d == nullptr || state->GetObjectID(d->GetUUID()) != demonID)
    {
        return;
    }

    auto cs = d->GetCoreStats().Get();
    bool isSummoned = dState->GetEntity() == d;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_BOX_DATA);
    reply.WriteS8(boxID);
    reply.WriteS8(slot);
    reply.WriteS64Little(demonID);
    reply.WriteU32Little(d->GetType());

    reply.WriteS16Little(cs->GetMaxHP());
    reply.WriteS16Little(cs->GetMaxMP());
    reply.WriteS16Little(cs->GetHP());
    reply.WriteS16Little(cs->GetMP());
    reply.WriteS64Little(cs->GetXP());
    reply.WriteS8(cs->GetLevel());

    GetEntityStatsPacketData(reply, cs, isSummoned ? dState : nullptr, 0);

    //Learned skill count will always be static
    reply.WriteS32Little(8);
    for(size_t i = 0; i < 8; i++)
    {
        auto skillID = d->GetLearnedSkills(i);
        reply.WriteU32Little(skillID == 0 ? (uint32_t)-1 : skillID);
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
    reply.WriteU16Little(d->GetFamiliarity());
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

    reply.WriteU8(0);   //Unknown
    reply.WriteU8(0);   //Mitama type

    //Reunion bonuses (12 * 8 ranks)
    for(size_t i = 0; i < 96; i++)
    {
        reply.WriteU8(0);
    }

    //Characteristics panel?
    for(size_t i = 0; i < 4; i++)
    {
        reply.WriteS64Little(-1);    //Item object ID?
        reply.WriteU32Little(static_cast<uint32_t>(-1));    //Item type?
    }

    //Effect length in seconds remaining
    reply.WriteS32Little(0);

    client->SendPacket(reply);
}

uint8_t CharacterManager::RecalculateStats(std::shared_ptr<
    ChannelClientConnection> client, int32_t entityID,
    bool updateSourceClient)
{
    auto state = client
        ? client->GetClientState()
        : ClientState::GetEntityClientState(entityID, false);
    if(!state)
    {
        return 0;
    }

    auto eState = state->GetEntityState(entityID);
    if(!eState || !eState->Ready())
    {
        return 0;
    }

    auto definitionManager = mServer.lock()->GetDefinitionManager();
    uint8_t result = eState->RecalculateStats(definitionManager);
    if(result & ENTITY_CALC_SKILL)
    {
        auto cState = std::dynamic_pointer_cast<CharacterState>(eState);
        if(cState)
        {
            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_LIST_UPDATED);
            p.WriteS32Little(cState->GetEntityID());

            auto skills = cState->GetCurrentSkills();
            p.WriteU32Little((uint32_t)skills.size());
            for(uint32_t skillID : skills)
            {
                p.WriteU32Little(skillID);
            }

            client->QueuePacket(p);
        }
    }

    if(result & ENTITY_CALC_STAT_LOCAL)
    {
        SendEntityStats(client, entityID, updateSourceClient);

        if((result & ENTITY_CALC_STAT_WORLD) && state->GetPartyID())
        {
            libcomp::Packet request;
            if(std::dynamic_pointer_cast<CharacterState>(eState))
            {
                state->GetPartyCharacterPacket(request);
            }
            else
            {
                state->GetPartyDemonPacket(request);
            }
            mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
        }
    }

    if(client)
    {
        client->FlushOutgoing();
    }

    return result;
}

void CharacterManager::SendEntityStats(std::shared_ptr<
    ChannelClientConnection> client, int32_t entityID,
    bool includeSelf)
{
    auto server = mServer.lock();
    if(!client)
    {
        client = server->GetManagerConnection()
            ->GetEntityClient(entityID, false);
        if(!client)
        {
            return;
        }
    }

    auto state = client->GetClientState();
    auto eState = state->GetEntityState(entityID);

    if(!eState || !eState->Ready())
    {
        return;
    }

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENTITY_STATS);
    p.WriteS32Little(eState->GetEntityID());

    GetEntityStatsPacketData(p, eState->GetCoreStats(), eState, 3);

    p.WriteS32Little((int32_t)eState->GetMaxHP());

    server->GetZoneManager()->BroadcastPacket(client, p, includeSelf);
}

bool CharacterManager::GetEntityRevivalPacket(libcomp::Packet& p,
    const std::shared_ptr<ActiveEntityState>& eState, int8_t action)
{
    auto cs = eState->GetCoreStats();
    if(cs)
    {
        bool syncXP = cs->GetLevel() >= 10;

        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REVIVE_ENTITY);
        p.WriteS32Little(eState->GetEntityID());
        p.WriteS8(action);
        p.WriteS32Little(cs->GetHP());
        p.WriteS32Little(cs->GetMP());
        p.WriteS64Little(syncXP ? cs->GetXP() : 0);

        return true;
    }

    return false;
}

void CharacterManager::SetStatusIcon(const std::shared_ptr<ChannelClientConnection>& client, int8_t icon)
{
    auto state = client->GetClientState();

    if(state->GetStatusIcon() == icon)
    {
        return;
    }

    state->SetStatusIcon(icon);

    // Send icon to the client
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_STATUS_ICON);
    p.WriteS8(0);
    p.WriteS8(icon);

    client->SendPacket(p);

    // Send icon to others in the zone
    p.Clear();
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_STATUS_ICON_OTHER);
    p.WriteS32Little(state->GetCharacterState()->GetEntityID());
    p.WriteS8(icon);

    mServer.lock()->GetZoneManager()->BroadcastPacket(client, p, false);
}

void CharacterManager::SummonDemon(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int64_t demonID, bool updatePartyState)
{
    StoreDemon(client, false);

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetEntity();

    auto demon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID)));
    if(!demon)
    {
        return;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetDevilData(demon->GetType());
    if(!def)
    {
        return;
    }

    character->SetActiveDemon(demon);
    dState->SetEntity(demon);

    // If the character and demon share alignment, apply summon sync
    if(cState->GetLNCType() == dState->GetLNCType(definitionManager))
    {
        uint32_t syncStatusType = 0;
        if(demon->GetFamiliarity() == MAX_FAMILIARITY)
        {
            syncStatusType = SVR_CONST.STATUS_SUMMON_SYNC_3;
        }
        else if(demon->GetFamiliarity() > 4000)
        {
            syncStatusType = SVR_CONST.STATUS_SUMMON_SYNC_2;
        }
        else
        {
            syncStatusType = SVR_CONST.STATUS_SUMMON_SYNC_1;
        }

        AddStatusEffectMap m;
        m[syncStatusType] = std::pair<uint8_t, bool>(1, true);
        dState->AddStatusEffects(m, definitionManager);
    }

    // If the demon's current familiarity is lower than the top 2
    // ranks, boost familiarity slightly
    if(GetFamiliarityRank(demon->GetFamiliarity()) < 3)
    {
        UpdateFamiliarity(client, 2, true, false);
    }

    dState->RecalculateStats(definitionManager);
    dState->SetStatusEffectsActive(true, definitionManager);
    dState->SetDestinationX(cState->GetDestinationX());
    dState->SetDestinationY(cState->GetDestinationY());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_SUMMONED);
    reply.WriteS64Little(demonID);

    client->SendPacket(reply);

    auto otherClients = server->GetZoneManager()
        ->GetZoneConnections(client, false);
    SendOtherPartnerData(otherClients, state);

    if(updatePartyState && state->GetPartyID())
    {
        libcomp::Packet request;
        state->GetPartyDemonPacket(request);
        server->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
    }
}

void CharacterManager::StoreDemon(const std::shared_ptr<
    channel::ChannelClientConnection>& client, bool updatePartyState,
    int32_t removeMode)
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

    // Remove all opponents
    AddRemoveOpponent(false, dState, nullptr);

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto zoneManager = server->GetZoneManager();
    auto zone = zoneManager->GetZoneInstance(client);

    dState->SetStatusEffectsActive(false, definitionManager);

    // Apply special cancel event for summon sync effects
    const static std::set<uint32_t> summonSyncs =
        {
            SVR_CONST.STATUS_SUMMON_SYNC_1,
            SVR_CONST.STATUS_SUMMON_SYNC_2,
            SVR_CONST.STATUS_SUMMON_SYNC_3
        };
    dState->ExpireStatusEffects(summonSyncs);

    UpdateStatusEffects(dState, true);
    dState->SetEntity(nullptr);
    character->SetActiveDemon(NULLUUID);

    std::list<int32_t> removeIDs = { dState->GetEntityID() };

    //Remove the entity from each client's zone
    zoneManager->RemoveEntitiesFromZone(zone, removeIDs, removeMode);

    if(updatePartyState && state->GetPartyID())
    {
        libcomp::Packet request;
        state->GetPartyDemonPacket(request);
        mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
    }
}

void CharacterManager::SendDemonBoxData(const std::shared_ptr<
    ChannelClientConnection>& client, int8_t boxID, std::set<int8_t> slots)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto box = GetDemonBox(state, boxID);

    auto character = cState->GetEntity();
    auto progress = character->GetProgress();

    uint32_t expiration = 0;
    int32_t count = 0;
    size_t maxSlots = boxID == 0 ? (size_t)progress->GetMaxCOMPSlots() : 50;
    if(box)
    {
        for(size_t i = 0; i < maxSlots; i++)
        {
            count += !box->GetDemons(i).IsNull() ? 1 : 0;
        }
        expiration = box->GetRentalExpiration();
    }

    libcomp::Packet reply;
    if(slots.size() > 0)
    {
        // Just send the specified slots
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_BOX_UPDATE);

        reply.WriteS8(0);
        reply.WriteS32Little((int32_t)slots.size());
        for(int8_t slot : slots)
        {
            GetDemonPacketData(reply, client, box, slot);
        }
        reply.WriteS8((int8_t)maxSlots);
    }
    else
    {
        // Send the whole thing
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_BOX);

        reply.WriteS8(boxID);
        reply.WriteS32Little(0);   //Unknown
        reply.WriteS32Little(expiration == 0 || box == nullptr
            ? -1 : ChannelServer::GetExpirationInSeconds(expiration));
        reply.WriteS32Little(count);

        if(box)
        {
            for(size_t i = 0; i < maxSlots; i++)
            {
                if (box->GetDemons(i).IsNull()) continue;

                GetDemonPacketData(reply, client, box, (int8_t)i);
                reply.WriteU8(0);   //Unknown
            }
        }
    }

    reply.WriteU8((uint8_t)maxSlots);

    client->SendPacket(reply);
}

std::shared_ptr<objects::DemonBox> CharacterManager::GetDemonBox(
    ClientState* state, int8_t boxID)
{
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto worldData = state->GetAccountWorldData();

    return boxID == 0
        ? character->GetCOMP().Get()
        : worldData->GetDemonBoxes((size_t)(boxID-1)).Get();
}

std::shared_ptr<objects::ItemBox> CharacterManager::GetItemBox(
    ClientState* state, int8_t boxType, int64_t boxID)
{
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto worldData = state->GetAccountWorldData();

    std::shared_ptr<objects::ItemBox> box;
    switch((objects::ItemBox::Type_t)boxType)
    {
        case objects::ItemBox::Type_t::INVENTORY:
            box = character->GetItemBoxes((size_t)boxID).Get();
            break;
        case objects::ItemBox::Type_t::ITEM_DEPO:
            box = worldData->GetItemBoxes((size_t)boxID).Get();
            break;
        default:
            if(nullptr == box)
            {
                LOG_ERROR(libcomp::String("Attempted to retrieve unknown"
                    " item box of type %1, with ID %2\n").Arg(boxType).Arg(boxID));
            }
            break;
    }

    return box;
}

void CharacterManager::SendItemBoxData(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<objects::ItemBox>& box)
{
    std::list<uint16_t> allSlots = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                                    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                                    40, 41, 42, 43, 44, 45, 46, 47, 48, 49 };
    SendItemBoxData(client, box, allSlots);
}

void CharacterManager::SendItemBoxData(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<objects::ItemBox>& box,
    const std::list<uint16_t>& slots)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

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
    reply.WriteS8((int8_t)box->GetType());
    reply.WriteS64(box->GetBoxID());

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
        reply.WriteU32Little(basicEffect ? basicEffect
            : static_cast<uint32_t>(-1));

        auto specialEffect = item->GetSpecialEffect();
        reply.WriteU32Little(specialEffect ? specialEffect
            : static_cast<uint32_t>(-1));

        for(auto bonus : item->GetFuseBonuses())
        {
            reply.WriteS8(bonus);
        }
    }

    client->SendPacket(reply);
}

std::list<std::shared_ptr<objects::Item>> CharacterManager::GetExistingItems(
    const std::shared_ptr<objects::Character>& character,
    uint32_t itemID, std::shared_ptr<objects::ItemBox> box)
{
    if(box == nullptr)
    {
        box = character->GetItemBoxes(0).Get();
    }

    std::list<std::shared_ptr<objects::Item>> existing;
    for(size_t i = 0; i < 50; i++)
    {
        auto item = box->GetItems(i);
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

bool CharacterManager::AddRemoveItems(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    std::unordered_map<uint32_t, uint16_t> itemCounts, bool add,
    int64_t skillTargetID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto itemBox = character->GetItemBoxes(0).Get();

    auto server = mServer.lock();
    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        state->GetAccountUID());

    std::list<uint16_t> updatedSlots;
    for(auto itemPair : itemCounts)
    {
        uint32_t itemID = itemPair.first;
        uint16_t quantity = itemPair.second;

        auto def = server->GetDefinitionManager()->GetItemData(itemID);
        if(nullptr == def)
        {
            return false;
        }

        auto existing = GetExistingItems(character, itemID);
        auto maxStack = def->GetPossession()->GetStackSize();
        if(add)
        {
            uint16_t quantityLeft = quantity;
            for(auto item : existing)
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
            }

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
                for(auto item : existing)
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
                }

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

                        if(!itemBox->SetItems(freeSlot, item))
                        {
                            return false;
                        }
                        updatedSlots.push_back((uint16_t)freeSlot);
                        dbChanges->Insert(item);

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
                    item->GetStackSize() <= quantity &&
                    character->GetEquippedItems((size_t)equipType).Get() == item)
                {
                    EquipItem(client, state->GetObjectID(item->GetUUID()));
                }

                auto slot = item->GetBoxSlot();
                if(item->GetStackSize() <= (quantity - removed))
                {
                    removed = (uint16_t)(removed + item->GetStackSize());

                    if(!itemBox->SetItems((size_t)slot, NULLUUID))
                    {
                        return false;
                    }

                    dbChanges->Delete(item);
                }
                else
                {
                    item->SetStackSize((uint16_t)(item->GetStackSize() -
                        (quantity - removed)));
                    removed = quantity;

                    dbChanges->Update(item);
                }
                updatedSlots.push_back((uint16_t)slot);

                if(removed == quantity)
                {
                    break;
                }
            }
        }
    }

    SendItemBoxData(client, itemBox, updatedSlots);

    dbChanges->Update(itemBox);

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    return true;
}

bool CharacterManager::CreateLootFromDrops(const std::shared_ptr<objects::LootBox>& box,
    const std::list<std::shared_ptr<objects::ItemDrop>>& drops, int16_t luck, bool minLast)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    std::list<std::shared_ptr<objects::Loot>> lootItems;
    for(auto drop : drops)
    {
        // Each point of luck adds 0.2% chance to get the drop
        uint16_t dropRate = (uint16_t)((drop->GetRate() * 100)
            + (float)(luck * 20));
        if(RNG(uint16_t, 0, 10000) < dropRate ||
            (minLast && lootItems.size() == 0 && drops.back() == drop))
        {
            auto itemDef = definitionManager->GetItemData(
                drop->GetItemType());
            uint16_t minStack = drop->GetMinStack();
            uint16_t maxStack = drop->GetMaxStack();

            // The drop rate is affected by luck but the stack size is not
            uint16_t stackSize = RNG(uint16_t, minStack, maxStack);

            uint16_t maxStackSize = itemDef->GetPossession()->GetStackSize();
            uint8_t stackCount = (uint8_t)ceill((double)stackSize /
                (double)maxStackSize);

            for(uint8_t i = 0; i < stackCount &&
                lootItems.size() < box->LootCount(); i++)
            {
                uint16_t stack = stackSize <= maxStackSize
                    ? stackSize : maxStackSize;
                stackSize = (uint16_t)(stackSize - stack);

                auto loot = std::make_shared<objects::Loot>();
                loot->SetType(drop->GetItemType());
                loot->SetCount(stack);
                lootItems.push_back(loot);
            }

            if(lootItems.size() == 12)
            {
                break;
            }
        }
    }

    bool added = false;
    if(lootItems.size() > 0)
    {
        for(size_t i = 0; i < box->LootCount(); i++)
        {
            if(lootItems.size() > 0)
            {
                auto loot = lootItems.front();
                lootItems.pop_front();
                box->SetLoot(i, loot);
                added = true;
            }
            else
            {
                break;
            }
        }
    }

    return added;
}

void CharacterManager::SendLootItemData(const std::list<std::shared_ptr<
    ChannelClientConnection>>& clients, const std::shared_ptr<LootBoxState>& lState,
    bool queue)
{
    auto lootBox = lState->GetEntity();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LOOT_ITEM_DATA);
    p.WriteS32Little(0);    // Entity ID (written later)
    p.WriteS32Little(lState->GetEntityID());
    p.WriteFloat(0.f);  // Loot time (written later)

    for(auto loot : lootBox->GetLoot())
    {
        if(loot && loot->GetCount() > 0)
        {
            p.WriteU32Little(loot->GetType());
            p.WriteU16Little(loot->GetCount());
        }
        else
        {
            p.WriteU32Little(static_cast<uint32_t>(-1));
            p.WriteU16Little(0);
        }
        p.WriteS8(3);
    }

    for(auto client : clients)
    {
        auto state = client->GetClientState();
        if(lootBox->ValidLooterIDsCount() == 0 ||
            lootBox->ValidLooterIDsContains(state->GetWorldCID()))
        {
            auto cState = state->GetCharacterState();

            p.Seek(2);
            p.WriteS32Little(cState->GetEntityID());
            p.Seek(10);
            p.WriteFloat(state->ToClientTime(lootBox->GetLootTime()));

            if(queue)
            {
                client->QueuePacketCopy(p);
            }
            else
            {
                client->SendPacketCopy(p);
            }
        }
    }
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

    RecalculateStats(client, cState->GetEntityID(), false);

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
    GetEntityStatsPacketData(reply, cs, cState, 2);

    server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

    client->SendPacket(reply);

    // Now update the other players
    reply.Clear();
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_OTHER_CHARACTER_EQUIPMENT_CHANGED);
    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteU8((uint8_t)slot);

    if(unequip)
    {
        reply.WriteU32Little(static_cast<uint32_t>(-1));
    }
    else
    {
        reply.WriteU32Little(equip->GetType());
    }

    reply.WriteS16Little(cState->GetMaxHP());
    reply.WriteS16Little(cState->GetMaxMP());

    server->GetZoneManager()->BroadcastPacket(client, reply, false);
}

bool CharacterManager::UnequipItem(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::shared_ptr<objects::Item>& item)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto server = mServer.lock();
    auto def = server->GetDefinitionManager()->GetItemData(item->GetType());
    if(def)
    {
        int8_t equipType = (int8_t)def->GetBasic()->GetEquipType();
        if(equipType >= 0 &&
            character->GetEquippedItems((size_t)equipType).Get() == item)
        {
            auto objID = state->GetObjectID(item->GetUUID());
            EquipItem(client, objID);
            return true;
        }
    }

    return false;
}

void  CharacterManager::EndTrade(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int32_t outcome)
{
    auto state = client->GetClientState();

    // Reset the session
    auto newSession = std::make_shared<objects::TradeSession>();
    newSession->SetOtherCharacterState(nullptr);
    state->SetTradeSession(newSession);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_ENDED);
    reply.WriteS32Little(outcome);
    client->QueuePacket(reply);
    SetStatusIcon(client, 0);
}

void CharacterManager::UpdateLNC(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int16_t lnc)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(lnc > 10000)
    {
        lnc = 10000;
    }
    else if(lnc < -10000)
    {
        lnc = -10000;
    }

    character->SetLNC(lnc);

    auto server = mServer.lock();
    server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LNC_POINTS);
    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteS16Little(character->GetLNC());

    client->SendPacket(reply);
}

std::shared_ptr<objects::Demon> CharacterManager::ContractDemon(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::shared_ptr<objects::MiDevilData>& demonData,
    int32_t sourceEntityID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto demon = ContractDemon(character, demonData);

    if(!demon)
    {
        LOG_ERROR("Failed to contract demon!\n");

        return {};
    }

    auto demonID = mServer.lock()->GetNextObjectID();
    state->SetObjectID(demon->GetUUID(), demonID);

    if(sourceEntityID != 0)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CONTRACT_COMPLETED);
        p.WriteS32Little(sourceEntityID);
        p.WriteS32Little(cState->GetEntityID());

        mServer.lock()->GetZoneManager()->BroadcastPacket(client, p);
    }

    int8_t slot = demon->GetBoxSlot();
    SendDemonData(client, 0, slot, demonID);

    return demon;
}

std::shared_ptr<objects::Demon> CharacterManager::ContractDemon(
    const std::shared_ptr<objects::Character>& character,
    const std::shared_ptr<objects::MiDevilData>& demonData)
{
    auto comp = character->GetCOMP().Get();
    auto progress = character->GetProgress();

    //Find the next empty slot to add the demon to
    int8_t compSlot = -1;
    size_t maxCompSlots = (size_t)progress->GetMaxCOMPSlots();
    for(size_t i = 0; i < maxCompSlots; i++)
    {
        if(comp->GetDemons(i).IsNull())
        {
            compSlot = (int8_t)i;
            break;
        }
    }

    //Return false if no slot is open
    if(compSlot == -1)
    {
        LOG_ERROR("No free slot to contract demon.\n");

        return nullptr;
    }

    auto d = GenerateDemon(demonData);
    if(!d)
    {
        LOG_ERROR("Failed to generate demon.\n");

        return nullptr;
    }

    auto ds = d->GetCoreStats().Get();

    d->SetDemonBox(comp);
    d->SetBoxSlot(compSlot);

    comp->SetDemons((size_t)compSlot, d);

    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        character->GetAccount().GetUUID());
    dbChanges->Insert(d);
    dbChanges->Insert(ds);
    dbChanges->Update(comp);

    auto server = mServer.lock();
    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    return d;
}

std::shared_ptr<objects::Demon> CharacterManager::GenerateDemon(
    const std::shared_ptr<objects::MiDevilData>& demonData)
{
    //Was valid demon data supplied?
    if(nullptr == demonData)
    {
        LOG_ERROR("Data for demon not found.\n");

        return nullptr;
    }

    //Create a new demon from it's defaults
    auto growth = demonData->GetGrowth();

    auto d = libcomp::PersistentObject::New<
        objects::Demon>(true);
    d->SetType(demonData->GetBasic()->GetID());

    auto ds = libcomp::PersistentObject::New<
        objects::EntityStats>(true);
    ds->SetLevel(static_cast<int8_t>(growth->GetBaseLevel()));
    d->SetCoreStats(ds);

    CalculateDemonBaseStats(d);
    d->SetLearnedSkills(growth->GetSkills());

    ds->SetEntity(d->GetUUID());

    return d;
}

int8_t CharacterManager::GetFamiliarityRank(uint16_t familiarity) const
{
    if(familiarity <= 1000)
    {
        return (familiarity <= 500) ? -3 : -2;
    }
    else if(familiarity <= 2000)
    {
        return -1;
    }
    else if(familiarity == MAX_FAMILIARITY)
    {
        return 4;
    }
    else
    {
        return (int8_t)floor((float)(familiarity - 2001)/2000.f);
    }
}

void CharacterManager::UpdateFamiliarity(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int32_t familiarity,
    bool isAdjust, bool sendPacket)
{
    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        return;
    }

    uint16_t current = demon->GetFamiliarity();
    int32_t newFamiliarity = isAdjust ? 0 : (int32_t)familiarity;
    if(isAdjust)
    {
        newFamiliarity = current + familiarity;
    }

    if(newFamiliarity > MAX_FAMILIARITY)
    {
        newFamiliarity = MAX_FAMILIARITY;
    }

    if(newFamiliarity < 0)
    {
        newFamiliarity = 0;
    }

    if(current != (uint16_t)newFamiliarity)
    {
        auto server = mServer.lock();

        int8_t oldRank = GetFamiliarityRank(current);
        int8_t newRank = GetFamiliarityRank((uint16_t)newFamiliarity);

        demon->SetFamiliarity((uint16_t)newFamiliarity);

        // Rank adjustments will change base stats
        if(oldRank != newRank)
        {
            CalculateDemonBaseStats(dState->GetEntity());
            RecalculateStats(client, dState->GetEntityID());
        }

        if(sendPacket)
        {
            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_FAMILIARITY_UPDATE);
            p.WriteS32Little(dState->GetEntityID());
            p.WriteU16Little((uint16_t)newFamiliarity);

            server->GetZoneManager()->BroadcastPacket(client, p);
        }

        server->GetWorldDatabase()->QueueUpdate(demon, state->GetAccountUID());
    }
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
    int32_t fType = 0;
    if(eState == dState)
    {
        isDemon = true;
        demonData = definitionManager->GetDevilData(demon->GetType());
        fType = demonData->GetFamiliarity()->GetFamiliarityType();
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
            std::list<uint32_t> newSkills;
            auto growth = demonData->GetGrowth();
            for(auto acSkill : growth->GetAcquisitionSkills())
            {
                if(acSkill->GetLevel() == (uint32_t)level)
                {
                    demon->AppendAcquiredSkills(acSkill->GetID());
                    newSkills.push_back(acSkill->GetID());
                }
            }

            CalculateDemonBaseStats(dState->GetEntity());
            RecalculateStats(client, entityID, false);
            stats->SetHP(dState->GetMaxHP());
            stats->SetMP(dState->GetMaxMP());

            reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_LEVEL_UP);
            reply.WriteS32Little(entityID);
            reply.WriteS8(level);
            reply.WriteS64Little(state->GetObjectID(demon->GetUUID()));
            GetEntityStatsPacketData(reply, stats, dState, 1);

            size_t newSkillCount = newSkills.size();
            reply.WriteU32Little(static_cast<uint32_t>(newSkillCount));
            for(auto aSkill : newSkills)
            {
                reply.WriteU32Little(aSkill);
            }

            // Familiarity is adjusted based on the demon's familiarity type
            // and level achieved
            const uint8_t fTypeMultiplier[17] =
                {
                    10,     // Type 0
                    15,     // Type 1
                    40,     // Type 2
                    40,     // Type 3
                    50,     // Type 4
                    150,    // Type 5
                    50,     // Type 6
                    40,     // Type 7
                    50,     // Type 8
                    120,    // Type 9
                    200,    // Type 10
                    100,    // Type 11
                    40,     // Type 12
                    50,     // Type 13
                    0,      // Type 14 (invalid)
                    0,      // Type 15 (invalid)
                    100     // Type 16
                };

            int32_t familiarityGain = (int32_t)(level * fTypeMultiplier[fType]);
            UpdateFamiliarity(client, familiarityGain, true);
        }
        else
        {
            CalculateCharacterBaseStats(stats);
            RecalculateStats(client, entityID, false);
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

    client->SendPacket(reply);

    server->GetWorldDatabase()->QueueUpdate(stats, state->GetAccountUID());
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
    auto stats = character->GetCoreStats();

    auto skill = definitionManager->GetSkillData(skillID);
    if(nullptr == skill)
    {
        LOG_WARNING(libcomp::String("Unknown skill ID encountered in"
            " UpdateExpertise: %1").Arg(skillID));
        return;
    }

    int32_t maxTotalPoints = 1700000 + (int32_t)(floorl((float)stats->GetLevel() * 0.1) *
        1000 * 100);
    int32_t currentPoints = 0;
    for(auto expertise : character->GetExpertises())
    {
        if(!expertise.IsNull())
        {
            currentPoints = currentPoints + expertise->GetPoints();
        }
    }

    if(maxTotalPoints <= currentPoints)
    {
        return;
    }

    std::list<std::pair<int8_t, int32_t>> updated;
    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
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

        // Calculate the point gain
        /// @todo: validate
        int32_t gain = (int32_t)((float)(3954.482803f /
            (((float)expertise->GetPoints() * 0.01f) + 158.1808409f)
            * expertGrowth->GetGrowthRate()) * 100.f);

        // Don't exceed the max total points
        if((currentPoints + gain) > maxTotalPoints)
        {
            gain = maxTotalPoints - currentPoints;
        }

        if(gain <= 0) continue;

        currentPoints = currentPoints + gain;

        points += gain;

        if(points > maxPoints)
        {
            points = maxPoints;
        }

        expertise->SetPoints(points);
        updated.push_back(std::pair<int8_t, int32_t>((int8_t)expDef->GetID(), points));
        dbChanges->Update(expertise);

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

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);
    }
}

bool CharacterManager::LearnSkill(const std::shared_ptr<channel::ChannelClientConnection>& client,
    int32_t entityID, uint32_t skillID)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto state = client->GetClientState();
    auto eState = state->GetEntityState(entityID);

    auto def = definitionManager->GetSkillData(skillID);
    if(nullptr == eState || nullptr == def)
    {
        return false;
    }

    auto dState = state->GetDemonState();
    if(eState == dState)
    {
        // Check if the skill is available anywhere for the demon
        auto demon = dState->GetEntity();
        auto learnedSkills = demon->GetLearnedSkills();
        auto inheritedSkills = demon->GetInheritedSkills();

        std::list<uint32_t> skills = demon->GetAcquiredSkills();
        for(auto s : learnedSkills)
        {
            skills.push_back(s);
        }

        for(auto s : inheritedSkills)
        {
            if(!s.IsNull())
            {
                skills.push_back(s->GetSkill());
            }
        }

        if(std::find(skills.begin(), skills.end(), skillID) != skills.end())
        {
            // Skill already exists
            return true;
        }

        demon->AppendAcquiredSkills(skillID);

        SendPartnerData(client);

        server->GetWorldDatabase()->QueueUpdate(demon, state->GetAccountUID());
    }
    else
    {
        // Check if the skill has already been learned
        auto character = state->GetCharacterState()->GetEntity();
        if(character->LearnedSkillsContains(skillID))
        {
            // Skill already exists
            return true;
        }

        character->InsertLearnedSkills(skillID);

        libcomp::Packet reply;
        reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LEARN_SKILL);
        reply.WriteS32Little(entityID);
        reply.WriteU32Little(skillID);

        client->SendPacket(reply);

        server->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());
    }

    RecalculateStats(client, entityID);

    return true;
}

void CharacterManager::ConvertIDToMaskValues(uint16_t id, size_t& index,
    uint8_t& shiftVal)
{
    index = (size_t)floor(id / 8);
    shiftVal = (uint8_t)(1 << (index ? (id % (uint16_t)index) : id));
}

bool CharacterManager::AddMap(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint16_t mapID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    size_t index;
    uint8_t shiftVal;
    ConvertIDToMaskValues(mapID, index, shiftVal);

    if(index >= progress->GetMaps().size())
    {
        return false;
    }

    auto oldValue = progress->GetMaps(index);
    uint8_t newValue = static_cast<uint8_t>(oldValue | shiftVal);

    if(oldValue != newValue)
    {
        progress->SetMaps((size_t)index, newValue);

        SendMapFlags(client);

        mServer.lock()->GetWorldDatabase()->QueueUpdate(progress, state->GetAccountUID());
    }

    return true;
}

void CharacterManager::SendMapFlags(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto maps = character->GetProgress()->GetMaps();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MAP_FLAG);
    reply.WriteU16Little((uint16_t)maps.size());
    reply.WriteArray(&maps, (uint32_t)maps.size());

    client->SendPacket(reply);
}

bool CharacterManager::AddRemoveValuable(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint16_t valuableID, bool remove)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    size_t index;
    uint8_t shiftVal;
    ConvertIDToMaskValues(valuableID, index, shiftVal);

    if(index >= progress->GetValuables().size())
    {
        return false;
    }

    auto oldValue = progress->GetValuables(index);
    uint8_t newValue = remove
        ? static_cast<uint8_t>(oldValue ^ shiftVal) : static_cast<uint8_t>(oldValue | shiftVal);

    if(oldValue != newValue)
    {
        progress->SetValuables((size_t)index, newValue);

        SendValuableFlags(client);

        mServer.lock()->GetWorldDatabase()->QueueUpdate(progress, state->GetAccountUID());
    }

    return true;
}

void CharacterManager::SendValuableFlags(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto valuables = character->GetProgress()->GetValuables();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_VALUABLE_LIST);
    reply.WriteU16Little((uint16_t)valuables.size());
    reply.WriteArray(&valuables, (uint32_t)valuables.size());

    client->SendPacket(reply);
}

bool CharacterManager::AddPlugin(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint16_t pluginID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    size_t index;
    uint8_t shiftVal;
    ConvertIDToMaskValues(pluginID, index, shiftVal);

    if(index >= progress->GetPlugins().size())
    {
        return false;
    }

    auto oldValue = progress->GetPlugins(index);
    uint8_t newValue = static_cast<uint8_t>(oldValue | shiftVal);

    if(oldValue != newValue)
    {
        progress->SetPlugins((size_t)index, newValue);

        SendPluginFlags(client);

        mServer.lock()->GetWorldDatabase()->QueueUpdate(progress, state->GetAccountUID());
    }

    return true;
}

void CharacterManager::SendPluginFlags(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto plugins = character->GetProgress()->GetPlugins();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_UNION_FLAG);
    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteU16Little((uint16_t)plugins.size());
    reply.WriteArray(&plugins, (uint32_t)plugins.size());

    client->SendPacket(reply);
}

bool CharacterManager::UpdateStatusEffects(const std::shared_ptr<
    ActiveEntityState>& eState, bool queueSave)
{
    auto cState = std::dynamic_pointer_cast<CharacterState>(eState);
    auto dState = std::dynamic_pointer_cast<DemonState>(eState);
    if(!cState && (!dState || !dState->GetEntity()))
    {
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto changes = libcomp::DatabaseChangeSet::Create(cState
            ? cState->GetEntity()->GetAccount().GetUUID()
            : dState->GetEntity()->GetDemonBox()->GetAccount().GetUUID());

    auto effectMap = eState->GetStatusEffects();
    std::unordered_map<uint32_t, bool> effectStates;
    for(auto ePair : effectMap)
    {
        // Default to insert
        effectStates[ePair.first] = true;
    }

    auto previous = cState
        ? cState->GetEntity()->GetStatusEffects()
        : dState->GetEntity()->GetStatusEffects();
    for(auto p : previous)
    {
        uint32_t effectType = p->GetEffect();
        if(effectStates.find(effectType) == effectStates.end())
        {
            // Delete
            changes->Delete(p.Get());
        }
        else
        {
            // Update
            effectStates[effectType] = false;
        }
    }

    std::set<uint32_t> removed;
    std::list<libcomp::ObjectReference<objects::StatusEffect>> updated;
    for(auto ePair : effectStates)
    {
        auto effectType = ePair.first;
        auto effect = effectMap[effectType];

        // If the effect can expire but somehow was not removed yet, remove it
        auto def = definitionManager->GetStatusData(effectType);
        bool expire = def->GetCancel()->GetDuration() > 0
            && effect->GetExpiration() == 0;

        if(expire)
        {
            removed.insert(effectType);
        }
        else
        {
            updated.push_back(effect);
        }

        if(!ePair.second)
        {
            if(expire)
            {
                changes->Delete(effect);
            }
            else
            {
                changes->Update(effect);
            }
        }
        else if(!expire)
        {
            changes->Insert(effect);
        }
    }

    if(removed.size() > 0)
    {
        eState->ExpireStatusEffects(removed);
    }

    if(cState)
    {
        cState->GetEntity()->SetStatusEffects(updated);
        changes->Update(cState->GetEntity());
    }
    else
    {
        dState->GetEntity()->SetStatusEffects(updated);
        changes->Update(dState->GetEntity());
    }

    auto db = server->GetWorldDatabase();
    if(queueSave)
    {
        return db->QueueChangeSet(changes);
    }
    else
    {
        return db->ProcessChangeSet(changes);
    }
}

void CharacterManager::CancelStatusEffects(const std::shared_ptr<
    ChannelClientConnection>& client, uint8_t cancelFlags)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();

    cState->CancelStatusEffects(cancelFlags);
    dState->CancelStatusEffects(cancelFlags);

    auto definitionManager = mServer.lock()->GetDefinitionManager();
    for(auto demon : cState->GetEntity()->GetCOMP()->GetDemons())
    {
        if(!demon.Get() || demon.Get() == dState->GetEntity()) continue;

        auto effects = demon->GetStatusEffects();

        std::set<size_t> cancelled;
        for(auto effect : effects)
        {
            auto cancel = definitionManager->
                GetStatusData(effect->GetEffect())->GetCancel();
            if(cancel->GetCancelTypes() & cancelFlags)
            {
                cancelled.insert(effect->GetEffect());
            }
        }

        effects.remove_if([cancelled]
            (libcomp::ObjectReference<objects::StatusEffect>& effect)
            {
                return cancelled.find(effect->GetEffect())
                    != cancelled.end();
            });

        if(cancelled.size() > 0)
        {
            demon->SetStatusEffects(effects);
        }
    }
}

bool CharacterManager::AddRemoveOpponent(bool add, const std::shared_ptr<
    ActiveEntityState>& eState1, const std::shared_ptr<ActiveEntityState>& eState2)
{
    auto zone = eState1->GetZone();
    if(!zone || (add && !eState2))
    {
        return false;
    }

    if(add && eState1->HasOpponent(eState2->GetEntityID()))
    {
        return true;
    }

    if(!add && eState1->GetOpponentIDs().size() == 0)
    {
        return true;
    }

    std::list<libcomp::Packet> packets;
    if(add)
    {
        // If one isn't alive, stop here
        if(!eState1->IsAlive() || !eState2->IsAlive())
        {
            return true;
        }

        // If either are a client entity, get both the character
        // and partner demon
        std::list<std::shared_ptr<ActiveEntityState>> e1s;
        std::list<std::shared_ptr<ActiveEntityState>> e2s;

        for(size_t i = 0; i < 2; i++)
        {
            auto entity = i == 0 ? eState1 : eState2;
            auto state = entity ? ClientState::GetEntityClientState(
                entity->GetEntityID()) : nullptr;
            auto& l = (i == 0) ? e1s : e2s;
            if(state)
            {
                l.push_back(state->GetCharacterState());
                l.push_back(state->GetDemonState());
            }
            else
            {
                l.push_back(entity);
            }
        }

        std::list<std::shared_ptr<ActiveEntityState>> battleStarted;
        std::list<std::shared_ptr<ActiveEntityState>> activatedEnemies;
        for(auto e1 : e1s)
        {
            for(auto e2 : e2s)
            {
                size_t opponentCount = e1->AddRemoveOpponent(true, e2->GetEntityID());
                if(opponentCount == 1)
                {
                    battleStarted.push_back(e1);
                    auto enemy = std::dynamic_pointer_cast<EnemyState>(e1);
                    if(enemy)
                    {
                        activatedEnemies.push_back(enemy);
                    }
                }

                opponentCount = e2->AddRemoveOpponent(true, e1->GetEntityID());
                if(opponentCount == 1)
                {
                    battleStarted.push_back(e2);
                    auto enemy = std::dynamic_pointer_cast<EnemyState>(e1);
                    if(enemy)
                    {
                        activatedEnemies.push_back(enemy);
                    }
                }
            }
        }

        for(auto entity : battleStarted)
        {
            auto aiState = entity->GetAIState();
            if(aiState)
            {
                aiState->SetStatus(AIStatus_t::COMBAT);
            }

            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BATTLE_STARTED);
            p.WriteS32Little(entity->GetEntityID());
            p.WriteFloat(300.f);    // Combat run speed
            packets.push_back(p);
        }

        for(auto enemy : activatedEnemies)
        {
            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENEMY_ACTIVATED);
            p.WriteS32Little(eState1->GetEntityID());
            p.WriteS32Little(enemy->GetEntityID());
            packets.push_back(p);
        }
    }
    else
    {
        int32_t e1ID = eState1->GetEntityID();
        std::list<std::shared_ptr<ActiveEntityState>> opponents;
        if(eState2)
        {
            opponents.push_back(eState2);
            eState1->AddRemoveOpponent(false, eState2->GetEntityID());
        }
        else
        {
            auto opponentIDs = eState1->GetOpponentIDs();
            for(int32_t oppID : opponentIDs)
            {
                eState1->AddRemoveOpponent(false, oppID);

                auto opponent = zone->GetActiveEntity(oppID);
                if(opponent)
                {
                    opponents.push_back(opponent);
                }
            }
        }

        std::list<std::shared_ptr<ActiveEntityState>> battleStopped;
        if(eState1->GetOpponentIDs().size() == 0)
        {
            battleStopped.push_back(eState1);
        }

        for(auto opponent : opponents)
        {
            opponent->AddRemoveOpponent(false, e1ID);
            if(opponent->GetOpponentIDs().size() == 0)
            {
                battleStopped.push_back(opponent);
            }
        }

        for(auto entity : battleStopped)
        {
            auto aiState = entity->GetAIState();
            if(aiState)
            {
                aiState->SetStatus(aiState->GetDefaultStatus());
            }

            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_BATTLE_STOPPED);
            p.WriteS32Little(entity->GetEntityID());
            p.WriteFloat(300.f);    // Run speed
            packets.push_back(p);
        }
    }

    if(packets.size() > 0)
    {
        auto zoneConnections = zone->GetConnectionList();
        ChannelClientConnection::BroadcastPackets(zoneConnections, packets);
    }

    return true;
}

void CharacterManager::UpdateWorldDisplayState(
    const std::set<std::shared_ptr<ActiveEntityState>>& entities)
{
    if(entities.size() > 0)
    {
        auto worldConnection = mServer.lock()->GetManagerConnection()
            ->GetWorldConnection();
        for(auto entity : entities)
        {
            auto entityClientState = ClientState::GetEntityClientState(
                entity->GetEntityID());
            if(!entityClientState || !entityClientState->GetPartyID()) continue;

            libcomp::Packet packet;
            if(entity->GetEntityType() ==
                objects::EntityStateObject::EntityType_t::PARTNER_DEMON)
            {
                entityClientState->GetPartyDemonPacket(packet);
            }
            else
            {
                entityClientState->GetPartyCharacterPacket(packet);
            }
            worldConnection->QueuePacket(packet);
        }

        worldConnection->FlushOutgoing();
    }
}

void CharacterManager::CalculateCharacterBaseStats(const std::shared_ptr<objects::EntityStats>& cs)
{
    auto stats = GetCharacterBaseStatMap(cs);

    CalculateDependentStats(stats, cs->GetLevel(), false);

    cs->SetMaxHP(stats[CorrectTbl::HP_MAX]);
    cs->SetMaxMP(stats[CorrectTbl::MP_MAX]);
    cs->SetCLSR(stats[CorrectTbl::CLSR]);
    cs->SetLNGR(stats[CorrectTbl::LNGR]);
    cs->SetSPELL(stats[CorrectTbl::SPELL]);
    cs->SetSUPPORT(stats[CorrectTbl::SUPPORT]);
    cs->SetPDEF(stats[CorrectTbl::PDEF]);
    cs->SetMDEF(stats[CorrectTbl::MDEF]);
}

void CharacterManager::CalculateDemonBaseStats(
    const std::shared_ptr<objects::Demon>& demon,
    std::shared_ptr<objects::EntityStats> ds,
    std::shared_ptr<objects::MiDevilData> demonData)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    if(demon)
    {
        ds = demon->GetCoreStats().Get();
        demonData = definitionManager->GetDevilData(demon->GetType());
    }

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

    libcomp::EnumMap<CorrectTbl, int16_t> stats;
    stats[CorrectTbl::STR] = battleData->GetCorrect((size_t)CorrectTbl::STR);
    stats[CorrectTbl::MAGIC] = battleData->GetCorrect((size_t)CorrectTbl::MAGIC);
    stats[CorrectTbl::VIT] = battleData->GetCorrect((size_t)CorrectTbl::VIT);
    stats[CorrectTbl::INT] = battleData->GetCorrect((size_t)CorrectTbl::INT);
    stats[CorrectTbl::SPEED] = battleData->GetCorrect((size_t)CorrectTbl::SPEED);
    stats[CorrectTbl::LUCK] = battleData->GetCorrect((size_t)CorrectTbl::LUCK);
    stats[CorrectTbl::HP_MAX] = battleData->GetCorrect((size_t)CorrectTbl::HP_MAX);
    stats[CorrectTbl::MP_MAX] = battleData->GetCorrect((size_t)CorrectTbl::MP_MAX);
    stats[CorrectTbl::CLSR] = battleData->GetCorrect((size_t)CorrectTbl::CLSR);
    stats[CorrectTbl::LNGR] = battleData->GetCorrect((size_t)CorrectTbl::LNGR);
    stats[CorrectTbl::SPELL] = battleData->GetCorrect((size_t)CorrectTbl::SPELL);
    stats[CorrectTbl::SUPPORT] = battleData->GetCorrect((size_t)CorrectTbl::SUPPORT);
    stats[CorrectTbl::PDEF] = battleData->GetCorrect((size_t)CorrectTbl::PDEF);
    stats[CorrectTbl::MDEF] = battleData->GetCorrect((size_t)CorrectTbl::MDEF);

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

    if(demon)
    {
        int8_t familiarityRank = GetFamiliarityRank(demon->GetFamiliarity());
        if(familiarityRank < 0)
        {
            // Ranks below zero have boost data 0-2 subtracted
            for(int8_t i = familiarityRank; i < 0; i++)
            {
                size_t famBoost = (size_t)(abs(i) - 1);
                BoostStats(stats, baseLevelRate->GetLevelUpData(famBoost), -1);
            }
        }
        else if(familiarityRank > 0)
        {
            // Ranks above zero have boost data 0-3 added
            for(int8_t i = 0; i < familiarityRank; i++)
            {
                size_t famBoost = (size_t)i;
                BoostStats(stats, baseLevelRate->GetLevelUpData(famBoost), 1);
            }
        }

        /// @todo: apply reunion boosts
    }

    CalculateDependentStats(stats, level, true);

    // Set anything that overflowed as int16_t max
    for(auto stat : stats)
    {
        if(stat.second < 0)
        {
            stat.second = 0x7FFF;
        }
    }

    ds->SetMaxHP(stats[CorrectTbl::HP_MAX]);
    ds->SetMaxMP(stats[CorrectTbl::MP_MAX]);
    ds->SetHP(stats[CorrectTbl::HP_MAX]);
    ds->SetMP(stats[CorrectTbl::MP_MAX]);
    ds->SetSTR(stats[CorrectTbl::STR]);
    ds->SetMAGIC(stats[CorrectTbl::MAGIC]);
    ds->SetVIT(stats[CorrectTbl::VIT]);
    ds->SetINTEL(stats[CorrectTbl::INT]);
    ds->SetSPEED(stats[CorrectTbl::SPEED]);
    ds->SetLUCK(stats[CorrectTbl::LUCK]);
    ds->SetCLSR(stats[CorrectTbl::CLSR]);
    ds->SetLNGR(stats[CorrectTbl::LNGR]);
    ds->SetSPELL(stats[CorrectTbl::SPELL]);
    ds->SetSUPPORT(stats[CorrectTbl::SUPPORT]);
    ds->SetPDEF(stats[CorrectTbl::PDEF]);
    ds->SetMDEF(stats[CorrectTbl::MDEF]);
}

libcomp::EnumMap<CorrectTbl, int16_t> CharacterManager::GetCharacterBaseStatMap(
    const std::shared_ptr<objects::EntityStats>& cs)
{
    libcomp::EnumMap<CorrectTbl, int16_t> stats;
    for(size_t i = 0; i < 126; i++)
    {
        CorrectTbl tblID = (CorrectTbl)i;
        stats[tblID] = 0;
    }

    stats[CorrectTbl::STR] = cs->GetSTR();
    stats[CorrectTbl::MAGIC] = cs->GetMAGIC();
    stats[CorrectTbl::VIT] = cs->GetVIT();
    stats[CorrectTbl::INT] = cs->GetINTEL();
    stats[CorrectTbl::SPEED] = cs->GetSPEED();
    stats[CorrectTbl::LUCK] = cs->GetLUCK();
    stats[CorrectTbl::HP_MAX] = 70;
    stats[CorrectTbl::MP_MAX] = 10;
    stats[CorrectTbl::HP_REGEN] = 1;
    stats[CorrectTbl::MP_REGEN] = 1;
    stats[CorrectTbl::MOVE1] = 15;
    stats[CorrectTbl::MOVE2] = 30;
    stats[CorrectTbl::SUMMON_SPEED] = 100;
    stats[CorrectTbl::KNOCKBACK_RESIST] = 100;
    stats[CorrectTbl::COOLDOWN_TIME] = 100;
    stats[CorrectTbl::BOOST_LB_DAMAGE] = 100;

    // Default all the rates to 100%
    for(uint8_t i = (uint8_t)CorrectTbl::RATE_XP;
        i < (uint8_t)CorrectTbl::RATE_SUPPORT_TAKEN; i++)
    {
        stats[(CorrectTbl)i] = 100;
    }

    return stats;
}

void CharacterManager::CalculateDependentStats(
    libcomp::EnumMap<CorrectTbl, int16_t>& stats, int8_t level, bool isDemon)
{
    /// @todo: fix: close but not quite right
    if(isDemon)
    {
        // Round up each part
        stats[CorrectTbl::HP_MAX] = (int16_t)(stats[CorrectTbl::HP_MAX] +
            (int16_t)ceill(stats[CorrectTbl::HP_MAX] * 0.03 * level) +
            (int16_t)ceill(stats[CorrectTbl::STR] * 0.3) +
            (int16_t)ceill(((stats[CorrectTbl::HP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::VIT]));
        stats[CorrectTbl::MP_MAX] = (int16_t)(stats[CorrectTbl::MP_MAX] +
            (int16_t)ceill(stats[CorrectTbl::MP_MAX] * 0.03 * level) +
            (int16_t)ceill(stats[CorrectTbl::MAGIC] * 0.3) +
            (int16_t)ceill(((stats[CorrectTbl::MP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::INT]));

        // Round the result, adjusting by 0.5
        stats[CorrectTbl::CLSR] = (int16_t)(stats[CorrectTbl::CLSR] +
            (int16_t)roundl((stats[CorrectTbl::STR] * 0.5) + 0.5 + (level * 0.1)));
        stats[CorrectTbl::LNGR] = (int16_t)(stats[CorrectTbl::LNGR] +
            (int16_t)roundl((stats[CorrectTbl::SPEED] * 0.5) + 0.5 + (level * 0.1)));
        stats[CorrectTbl::SPELL] = (int16_t)(stats[CorrectTbl::SPELL] +
            (int16_t)roundl((stats[CorrectTbl::MAGIC] * 0.5) + 0.5 + (level * 0.1)));
        stats[CorrectTbl::SUPPORT] = (int16_t)(stats[CorrectTbl::SUPPORT] +
            (int16_t)roundl((stats[CorrectTbl::INT] * 0.5) + 0.5 + (level * 0.1)));
        stats[CorrectTbl::PDEF] = (int16_t)(stats[CorrectTbl::PDEF] +
            (int16_t)roundl((stats[CorrectTbl::VIT] * 0.1) + 0.5 + (level * 0.1)));
        stats[CorrectTbl::MDEF] = (int16_t)(stats[CorrectTbl::MDEF] +
            (int16_t)roundl((stats[CorrectTbl::INT] * 0.1) + 0.5 + (level * 0.1)));
    }
    else
    {
        // Round each part
        stats[CorrectTbl::HP_MAX] = (int16_t)(stats[CorrectTbl::HP_MAX] +
            (int16_t)roundl(stats[CorrectTbl::HP_MAX] * 0.03 * level) +
            (int16_t)roundl(stats[CorrectTbl::STR] * 0.3) +
            (int16_t)roundl(((stats[CorrectTbl::HP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::VIT]));
        stats[CorrectTbl::MP_MAX] = (int16_t)(stats[CorrectTbl::MP_MAX] +
            (int16_t)roundl(stats[CorrectTbl::MP_MAX] * 0.03 * level) +
            (int16_t)roundl(stats[CorrectTbl::MAGIC] * 0.3) +
            (int16_t)roundl(((stats[CorrectTbl::MP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::INT]));

        // Round the results down
        stats[CorrectTbl::CLSR] = (int16_t)(stats[CorrectTbl::CLSR] +
            (int16_t)floorl((stats[CorrectTbl::STR] * 0.5) + (level * 0.1)));
        stats[CorrectTbl::LNGR] = (int16_t)(stats[CorrectTbl::LNGR] +
            (int16_t)floorl((stats[CorrectTbl::SPEED] * 0.5) + (level * 0.1)));
        stats[CorrectTbl::SPELL] = (int16_t)(stats[CorrectTbl::SPELL] +
            (int16_t)floorl((stats[CorrectTbl::MAGIC] * 0.5) + (level * 0.1)));
        stats[CorrectTbl::SUPPORT] = (int16_t)(stats[CorrectTbl::SUPPORT] +
            (int16_t)floorl((stats[CorrectTbl::INT] * 0.5) + (level * 0.1)));
        stats[CorrectTbl::PDEF] = (int16_t)(stats[CorrectTbl::PDEF] +
            (int16_t)floorl((stats[CorrectTbl::VIT] * 0.1) + (level * 0.1)));
        stats[CorrectTbl::MDEF] = (int16_t)(stats[CorrectTbl::MDEF] +
            (int16_t)floorl((stats[CorrectTbl::INT] * 0.1) + (level * 0.1)));
    }

    // Always round down
    stats[CorrectTbl::HP_REGEN] = (int16_t)(stats[CorrectTbl::HP_REGEN] +
        (int16_t)floorl(((stats[CorrectTbl::VIT] * 3) + stats[CorrectTbl::HP_MAX]) * 0.01));
    stats[CorrectTbl::MP_REGEN] = (int16_t)(stats[CorrectTbl::MP_REGEN] +
        (int16_t)floorl(((stats[CorrectTbl::INT] * 3) + stats[CorrectTbl::MP_MAX]) * 0.01));
}

void CharacterManager::AdjustStatBounds(libcomp::EnumMap<CorrectTbl, int16_t>& stats)
{
    static libcomp::EnumMap<CorrectTbl, int16_t> minStats =
        {
            { CorrectTbl::HP_MAX, 1 },
            { CorrectTbl::MP_MAX, 0 },
            { CorrectTbl::STR, 1 },
            { CorrectTbl::MAGIC, 1 },
            { CorrectTbl::VIT, 1 },
            { CorrectTbl::INT, 1 },
            { CorrectTbl::SPEED, 1 },
            { CorrectTbl::LUCK, 1 },
            { CorrectTbl::CLSR, 0 },
            { CorrectTbl::LNGR, 0 },
            { CorrectTbl::SPELL, 0 },
            { CorrectTbl::SUPPORT, 0 },
            { CorrectTbl::PDEF, 0 },
            { CorrectTbl::MDEF, 0 },
            { CorrectTbl::HP_REGEN, 0 },
            { CorrectTbl::MP_REGEN, 0 }
        };

    for(auto pair : minStats)
    {
        auto it = stats.find(pair.first);
        if(it != stats.end() && it->second < pair.second)
        {
            stats[pair.first] = pair.second;
        }
    }
}

void CharacterManager::GetDemonPacketData(libcomp::Packet& p,
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::shared_ptr<objects::DemonBox>& box, int8_t slot)
{
    auto state = client->GetClientState();
    auto demon = box->GetDemons((size_t)slot).Get();

    p.WriteS8(slot);
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
    uint8_t format)
{
    auto baseOnly = state == nullptr;

    switch(format)
    {
    case 0:
    case 1:
        {
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

            if(format == 1)
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
        break;
    case 2:
    case 3:
        {
            // Non-adjusted recalc format makes no sense
            if(baseOnly) break;

            p.WriteS16Little(static_cast<int16_t>(
                state->GetSTR() - coreStats->GetSTR()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetMAGIC() - coreStats->GetMAGIC()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetVIT() - coreStats->GetVIT()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetINTEL() - coreStats->GetINTEL()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetSPEED() - coreStats->GetSPEED()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetLUCK() - coreStats->GetLUCK()));
            p.WriteS16Little(state->GetMaxHP());
            p.WriteS16Little(state->GetMaxMP());
            p.WriteS16Little(static_cast<int16_t>(
                state->GetCLSR() - coreStats->GetCLSR()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetLNGR() - coreStats->GetLNGR()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetSPELL() - coreStats->GetSPELL()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetSUPPORT() - coreStats->GetSUPPORT()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetPDEF() - coreStats->GetPDEF()));
            p.WriteS16Little(static_cast<int16_t>(
                state->GetMDEF() - coreStats->GetMDEF()));

            if(format == 3)
            {
                // Unknown
                if(std::dynamic_pointer_cast<CharacterState>(state))
                {
                    p.WriteS16(-5600);
                    p.WriteS16(5600);
                }
                else
                {
                    p.WriteS16(0);
                    p.WriteS16(0);
                }
            }

            p.WriteS16Little(coreStats->GetCLSR());
            p.WriteS16Little(coreStats->GetLNGR());
            p.WriteS16Little(coreStats->GetSPELL());
            p.WriteS16Little(coreStats->GetSUPPORT());
            p.WriteS16Little(coreStats->GetPDEF());
            p.WriteS16Little(coreStats->GetMDEF());
        }
        break;
    default:
        break;
    }
}

void CharacterManager::DeleteDemon(const std::shared_ptr<objects::Demon>& demon,
    const std::shared_ptr<libcomp::DatabaseChangeSet>& changes)
{
    auto box = demon->GetDemonBox().Get();
    if(box->GetDemons((size_t)demon->GetBoxSlot()).Get() == demon)
    {
        box->SetDemons((size_t)demon->GetBoxSlot(), NULLUUID);
        changes->Update(box);
    }

    changes->Delete(demon);
    changes->Delete(demon->GetCoreStats().Get());

    for(auto iSkill : demon->GetInheritedSkills())
    {
        changes->Delete(iSkill.Get());
    }

    for(auto effect : demon->GetStatusEffects())
    {
        changes->Delete(effect.Get());
    }
}

void CharacterManager::BoostStats(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    const std::shared_ptr<objects::MiDevilLVUpData>& data, int boostLevel)
{
    stats[CorrectTbl::STR] = (int16_t)(stats[CorrectTbl::STR] +
        (int16_t)(data->GetSTR() * boostLevel));
    stats[CorrectTbl::MAGIC] = (int16_t)(stats[CorrectTbl::MAGIC] +
        (int16_t)(data->GetMAGIC() * boostLevel));
    stats[CorrectTbl::VIT] = (int16_t)(stats[CorrectTbl::VIT] +
        (int16_t)(data->GetVIT() * boostLevel));
    stats[CorrectTbl::INT] = (int16_t)(stats[CorrectTbl::INT] +
        (int16_t)(data->GetINTEL() * boostLevel));
    stats[CorrectTbl::SPEED] = (int16_t)(stats[CorrectTbl::SPEED] +
        (int16_t)(data->GetSPEED() * boostLevel));
    stats[CorrectTbl::LUCK] = (int16_t)(stats[CorrectTbl::LUCK] +
        (int16_t)(data->GetLUCK() * boostLevel));
}
