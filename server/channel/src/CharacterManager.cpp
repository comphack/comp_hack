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
#include <DefinitionManager.h>
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <limits>
#include <math.h>

// object Includes
#include <Account.h>
#include <AccountWorldData.h>
#include <ChannelConfig.h>
#include <CharacterProgress.h>
#include <Clan.h>
#include <DemonBox.h>
#include <DemonPresent.h>
#include <DemonQuest.h>
#include <EnchantSpecialData.h>
#include <Expertise.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <ItemDrop.h>
#include <Loot.h>
#include <LootBox.h>
#include <MiAcquisitionSkillData.h>
#include <MiCancelData.h>
#include <MiCategoryData.h>
#include <MiDCategoryData.h>
#include <MiDevilBattleData.h>
#include <MiDevilCrystalData.h>
#include <MiDevilData.h>
#include <MiDevilFamiliarityData.h>
#include <MiDevilLVUpData.h>
#include <MiDevilLVUpRateData.h>
#include <MiEnchantCharasticData.h>
#include <MiEnchantData.h>
#include <MiExpertData.h>
#include <MiExpertGrowthTbl.h>
#include <MiGrowthData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiNPCBasicData.h>
#include <MiPossessionData.h>
#include <MiRentalData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiStatusData.h>
#include <MiUnionData.h>
#include <MiUseRestrictionsData.h>
#include <PlayerExchangeSession.h>
#include <ServerZone.h>
#include <StatusEffect.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "AIState.h"
#include "ChannelServer.h"
#include "EventManager.h"
#include "FusionManager.h"
#include "ManagerConnection.h"
#include "SkillManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

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
    reply.WriteS16Little((int16_t)cState->GetMaxHP());
    reply.WriteS16Little((int16_t)cState->GetMaxMP());
    reply.WriteS16Little((int16_t)cs->GetHP());
    reply.WriteS16Little((int16_t)cs->GetMP());
    reply.WriteS64Little(cs->GetXP());
    reply.WriteS32Little(c->GetPoints());
    reply.WriteS8(cs->GetLevel());
    reply.WriteS16Little(c->GetLNC());

    GetEntityStatsPacketData(reply, cs, cState, 0);

    reply.WriteS16(-5600); // Unknown
    reply.WriteS16(5600); // Unknown

    auto statusEffects = cState->GetCurrentStatusEffectStates();

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
    float homeX = 0.f;
    float homeY = 0.f;
    float homeRot = 0.f;
    mServer.lock()->GetZoneManager()->GetSpotPosition(
        zoneDef->GetDynamicMapID(), c->GetHomepointSpotID(), homeX, homeY, homeRot);

    reply.WriteS32Little((int32_t)c->GetHomepointZone());
    reply.WriteFloat(homeX);
    reply.WriteFloat(homeY);

    reply.WriteS8(0);   // Unknown
    reply.WriteS8(0);   // Unknown
    reply.WriteS8(c->GetExpertiseExtension());

    reply.WriteS32((int32_t)c->EquippedVACount());
    for(uint8_t i = 0; i <= MAX_VA_INDEX; i++)
    {
        uint32_t va = c->GetEquippedVA(i);
        if(va)
        {
            reply.WriteS8((int8_t)i);
            reply.WriteU32Little(va);
        }
    }

    client->SendPacket(reply);

    if(cState->GetDisplayState() == ActiveDisplayState_t::DATA_NOT_SENT)
    {
        cState->SetDisplayState(ActiveDisplayState_t::DATA_SENT);
    }
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

    reply.WriteS16Little((int16_t)cState->GetMaxHP());
    reply.WriteS16Little((int16_t)cState->GetMaxMP());
    reply.WriteS16Little((int16_t)cs->GetHP());
    reply.WriteS16Little((int16_t)cs->GetMP());
    reply.WriteS8(cs->GetLevel());
    reply.WriteS16Little(c->GetLNC());

    auto statusEffects = cState->GetCurrentStatusEffectStates();

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

    size_t titleIdx = (size_t)(c->GetCurrentTitle() * MAX_TITLE_PARTS);
    auto customTitles = c->GetCustomTitles();

    for(size_t i = titleIdx; i < titleIdx + MAX_TITLE_PARTS; i++)
    {
        reply.WriteS16Little(customTitles[i]);
    }

    reply.WriteU8(c->GetTitlePrioritized() ? 1 : 0);

    reply.WriteS8(0);   // Unknown
    reply.WriteS32(0);  // Unknown
    reply.WriteS8(0);   // Unknown

    reply.WriteS32((int32_t)c->EquippedVACount());
    for(uint8_t i = 0; i <= MAX_VA_INDEX; i++)
    {
        uint32_t va = c->GetEquippedVA(i);
        if(va)
        {
            reply.WriteS8((int8_t)i);
            reply.WriteU32Little(va);
        }
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
    auto def = dState->GetDevilData();
    auto ds = d->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_DATA);
    reply.WriteS32Little(dState->GetEntityID());
    reply.WriteS8(d->GetBoxSlot());
    reply.WriteS64Little(state->GetObjectID(d->GetUUID()));
    reply.WriteU32Little(d->GetType());
    reply.WriteS16Little((int16_t)dState->GetMaxHP());
    reply.WriteS16Little((int16_t)dState->GetMaxMP());
    reply.WriteS16Little((int16_t)ds->GetHP());
    reply.WriteS16Little((int16_t)ds->GetMP());
    reply.WriteS64Little(ds->GetXP());
    reply.WriteS8(ds->GetLevel());
    reply.WriteS16Little(def->GetBasic()->GetLNC());

    GetEntityStatsPacketData(reply, ds, dState, 0);

    auto statusEffects = dState->GetCurrentStatusEffectStates();

    reply.WriteU32Little(static_cast<uint32_t>(statusEffects.size()));
    for(auto ePair : statusEffects)
    {
        reply.WriteU32Little(ePair.first->GetEffect());
        reply.WriteS32Little((int32_t)ePair.second);
        reply.WriteU8(ePair.first->GetStack());
    }

    // Learned skill count will always be static
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

    reply.WriteU8(0);   // Unknown bool
    reply.WriteU16Little(d->GetAttackSettings());
    reply.WriteU8(d->GetGrowthType());
    reply.WriteU16Little(d->GetFamiliarity());
    reply.WriteU8(d->GetLocked() ? 1 : 0);

    for(int8_t reunionRank : d->GetReunion())
    {
        reply.WriteS8(reunionRank);
    }

    reply.WriteS8(d->GetMagReduction());
    reply.WriteS32Little(d->GetSoulPoints());

    reply.WriteS32Little(d->GetBenefitGauge());
    for(int32_t forceValue : d->GetForceValues())
    {
        reply.WriteS32Little(forceValue);
    }

    for(uint16_t forceStack : d->GetForceStack())
    {
        reply.WriteU16Little(forceStack);
    }

    reply.WriteU16Little(d->GetForceStackPending());

    reply.WriteU8(0);   // Unknown
    reply.WriteU8(0);   // Mitama type

    // Reunion bonuses (12 * 8 ranks)
    for(size_t i = 0; i < 96; i++)
    {
        reply.WriteU8(0);
    }

    // Equipment
    for(size_t i = 0; i < 4; i++)
    {
        auto equip = d->GetEquippedItems(i).Get();
        if(equip)
        {
            reply.WriteS64Little(state->GetObjectID(equip->GetUUID()));
            reply.WriteU32Little(equip->GetType());
        }
        else
        {
            reply.WriteS64Little(-1);
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }
    }

    // Effect length in seconds
    reply.WriteS32Little(0);

    client->SendPacket(reply);

    if(dState->GetDisplayState() == ActiveDisplayState_t::DATA_NOT_SENT)
    {
        dState->SetDisplayState(ActiveDisplayState_t::DATA_SENT);
    }
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
    reply.WriteS16Little((int16_t)dState->GetMaxHP());
    reply.WriteS16Little((int16_t)ds->GetHP());
    reply.WriteS8(ds->GetLevel());

    auto statusEffects = dState->GetCurrentStatusEffectStates();

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

    reply.WriteS16Little((int16_t)dState->GetMaxMP());
    reply.WriteS16Little((int16_t)ds->GetMP());
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

    reply.WriteS16Little((int16_t)cs->GetMaxHP());
    reply.WriteS16Little((int16_t)cs->GetMaxMP());
    reply.WriteS16Little((int16_t)cs->GetHP());
    reply.WriteS16Little((int16_t)cs->GetMP());
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

    reply.WriteU16Little(d->GetAttackSettings());
    reply.WriteU8(d->GetGrowthType());
    reply.WriteU16Little(d->GetFamiliarity());
    reply.WriteU8(d->GetLocked() ? 1 : 0);

    for(int8_t reunionRank : d->GetReunion())
    {
        reply.WriteS8(reunionRank);
    }

    reply.WriteS8(d->GetMagReduction());
    reply.WriteS32Little(d->GetSoulPoints());

    reply.WriteS32Little(d->GetBenefitGauge());
    for(int32_t forceValue : d->GetForceValues())
    {
        reply.WriteS32Little(forceValue);
    }

    for(uint16_t forceStack : d->GetForceStack())
    {
        reply.WriteU16Little(forceStack);
    }

    reply.WriteU16Little(d->GetForceStackPending());

    reply.WriteU8(0);   // Unknown
    reply.WriteU8(0);   // Mitama type

    // Reunion bonuses (12 * 8 ranks)
    for(size_t i = 0; i < 96; i++)
    {
        reply.WriteU8(0);
    }

    // Equipment
    for(size_t i = 0; i < 4; i++)
    {
        auto equip = d->GetEquippedItems(i).Get();
        if(equip)
        {
            reply.WriteS64Little(state->GetObjectID(equip->GetUUID()));
            reply.WriteU32Little(equip->GetType());
        }
        else
        {
            reply.WriteS64Little(-1);
            reply.WriteU32Little(static_cast<uint32_t>(-1));
        }
    }

    // Effect length in seconds remaining
    reply.WriteS32Little(0);

    client->SendPacket(reply);
}

uint8_t CharacterManager::RecalculateStats(
    const std::shared_ptr<ActiveEntityState>& eState,
    std::shared_ptr<ChannelClientConnection> client, bool updateSourceClient)
{
    if(!eState || !eState->Ready(true))
    {
        return 0;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    uint8_t result = eState->RecalculateStats(definitionManager);

    if(result && !client)
    {
        client = server->GetManagerConnection()
            ->GetEntityClient(eState->GetEntityID(), false);
    }

    if(client)
    {
        if(result & ENTITY_CALC_MOVE_SPEED)
        {
            // Since speed updates are only sent to the player who
            // owns the entity, ignore enemies etc
            SendMovementSpeed(client, eState, false);
        }

        if(result & ENTITY_CALC_SKILL)
        {
            auto cState = std::dynamic_pointer_cast<CharacterState>(eState);
            if(cState)
            {
                libcomp::Packet p;
                p.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_SKILL_LIST_UPDATED);
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
            SendEntityStats(client, eState->GetEntityID(), updateSourceClient);

            auto state = client->GetClientState();
            if((result & ENTITY_CALC_STAT_WORLD) &&
                state && state->GetPartyID())
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

                mServer.lock()->GetManagerConnection()->GetWorldConnection()
                    ->SendPacket(request);
            }
        }

        client->FlushOutgoing();
    }

    return result;
}

uint8_t CharacterManager::RecalculateTokuseiAndStats(
    const std::shared_ptr<ActiveEntityState>& eState,
    std::shared_ptr<ChannelClientConnection> client)
{
    std::shared_ptr<ActiveEntityState> primaryEntity = eState;
    if(client)
    {
        primaryEntity = client->GetClientState()
            ->GetCharacterState();
    }

    mServer.lock()->GetTokuseiManager()->Recalculate(primaryEntity,
        true, std::set<int32_t>{ eState->GetEntityID() });
    return RecalculateStats(eState, client);
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

    p.WriteS32Little(eState->GetMaxHP());

    server->GetZoneManager()->BroadcastPacket(client, p, includeSelf);
}

bool CharacterManager::GetEntityRevivalPacket(libcomp::Packet& p,
    const std::shared_ptr<ActiveEntityState>& eState, int8_t action)
{
    auto cs = eState->GetCoreStats();
    if(cs)
    {
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REVIVE_ENTITY);
        p.WriteS32Little(eState->GetEntityID());
        p.WriteS8(action);
        p.WriteS32Little(cs->GetHP());
        p.WriteS32Little(cs->GetMP());
        p.WriteS64Little(cs->GetXP());

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

void CharacterManager::SendCharacterTitle(const std::shared_ptr<
    ChannelClientConnection>& client, bool includeSelf)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(character)
    {
        size_t titleIdx = (size_t)(character->GetCurrentTitle() *
            MAX_TITLE_PARTS);
        auto customTitles = character->GetCustomTitles();

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TITLE_ACTIVE);
        p.WriteS32Little(cState->GetEntityID());

        for(size_t i = titleIdx; i < titleIdx + MAX_TITLE_PARTS; i++)
        {
            p.WriteS16Little(customTitles[i]);
        }

        p.WriteU8(character->GetTitlePrioritized() ? 1 : 0);

        mServer.lock()->GetZoneManager()->BroadcastPacket(client, p, includeSelf);
    }
}

void CharacterManager::SendMovementSpeed(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::shared_ptr<ActiveEntityState>& eState, bool diffOnly,
    bool queue)
{
    if(eState && client && (!diffOnly || eState->GetSpeedBoost() ||
        eState->GetCorrectValue(CorrectTbl::MOVE2) != STAT_DEFAULT_SPEED))
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_RUN_SPEED);
        p.WriteS32Little(eState->GetEntityID());
        p.WriteFloat(eState->GetMovementSpeed());

        if(queue)
        {
            client->QueuePacket(p);
        }
        else
        {
            client->SendPacket(p);
        }
    }
}

void CharacterManager::SendAutoRecovery(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(character)
    {
        auto data = character->GetAutoRecovery();

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_AUTO_RECOVERY);

        // Data represents item type, percent 5 times
        p.WriteU8((uint8_t)(data.size() / 5));
        p.WriteArray(&data, (uint32_t)data.size());

        client->SendPacket(p);
    }
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
    auto tokuseiManager = server->GetTokuseiManager();
    auto def = definitionManager->GetDevilData(demon->GetType());
    if(!def)
    {
        return;
    }

    character->SetActiveDemon(demon);
    dState->SetEntity(demon, def);
    dState->RefreshLearningSkills(0, definitionManager);

    // Mark that the demon state has not been fully summoned yet so
    // the summon effect only displays once
    dState->SetDisplayState(ActiveDisplayState_t::AWAITING_SUMMON);

    // If the character and demon share alignment, apply summon sync
    if(cState->GetLNCType() == dState->GetLNCType())
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

        StatusEffectChange effect(syncStatusType, 1, true);

        double extend = tokuseiManager->GetAspectSum(cState,
            TokuseiAspectType::SUMMON_SYNC_EXTEND);
        if(extend > 0.0)
        {
            auto effectDef = definitionManager->GetStatusData(syncStatusType);
            if(effectDef)
            {
                effect.Duration = (uint32_t)((1.0 + (double)extend / 100.0) *
                    effectDef->GetCancel()->GetDuration());
            }
        }

        StatusEffectChanges effects;
        effects[syncStatusType] = effect;
        dState->AddStatusEffects(effects, definitionManager);
    }

    // If the demon's current familiarity is lower than the top 2
    // ranks, boost familiarity slightly
    if(GetFamiliarityRank(demon->GetFamiliarity()) < 3)
    {
        UpdateFamiliarity(client, 2, true, false);
    }

    // Apply initial tokusei/stat calculation
    tokuseiManager->Recalculate(cState, true,
        std::set<int32_t>{ dState->GetEntityID() });
    dState->RecalculateStats(definitionManager);

    // If HP/MP adjustments occur and the max value increases, keep
    // the same percentage of HP/MP after recalc
    auto cs = demon->GetCoreStats();
    int32_t maxHP = cs->GetMaxHP();
    int32_t maxMP = cs->GetMaxMP();
    float hpPercent = (float)cs->GetHP() / (float)cs->GetMaxHP();
    float mpPercent = (float)cs->GetMP() / (float)cs->GetMaxMP();

    dState->SetStatusEffectsActive(true, definitionManager);
    dState->SetDestinationX(cState->GetDestinationX());
    dState->SetDestinationY(cState->GetDestinationY());

    if(dState->GetMaxHP() > maxHP)
    {
        cs->SetHP((int32_t)((float)dState->GetMaxHP() * hpPercent));
    }

    if(dState->GetMaxMP() > maxMP)
    {
        cs->SetMP((int32_t)((float)dState->GetMaxMP() * mpPercent));
    }

    // Apply any extra summon status effects
    for(auto eState : { std::dynamic_pointer_cast<ActiveEntityState>(cState),
        std::dynamic_pointer_cast<ActiveEntityState>(dState) })
    {
        StatusEffectChanges effects;
        for(double val : tokuseiManager->GetAspectValueList(eState,
            TokuseiAspectType::SUMMON_STATUS))
        {
            effects[(uint32_t)val] = StatusEffectChange((uint32_t)val, 1,
                true);
        }

        if(effects.size() > 0)
        {
            eState->AddStatusEffects(effects, definitionManager);
            tokuseiManager->Recalculate(cState, true,
                std::set<int32_t>{ dState->GetEntityID() });
        }
    }

    // Perform final summon recalculation
    dState->RecalculateStats(definitionManager);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_SUMMONED);
    reply.WriteS64Little(demonID);

    client->QueuePacket(reply);

    client->FlushOutgoing();

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
    auto zone = zoneManager->GetCurrentZone(client);

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
    dState->SetEntity(nullptr, nullptr);
    dState->RefreshLearningSkills(0, definitionManager);

    character->SetActiveDemon(NULLUUID);

    std::list<int32_t> removeIDs = { dState->GetEntityID() };

    //Remove the entity from each client's zone
    zoneManager->RemoveEntitiesFromZone(zone, removeIDs, removeMode);

    if(updatePartyState)
    {
        // Recalc and send new HP/MP display
        SendDemonBoxData(client, 0, std::set<int8_t>{ demon->GetBoxSlot() });

        server->GetTokuseiManager()->Recalculate(cState, true);

        if(state->GetPartyID())
        {
            libcomp::Packet request;
            state->GetPartyDemonPacket(request);
            mServer.lock()->GetManagerConnection()->GetWorldConnection()->SendPacket(request);
        }
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

        reply.WriteS8(boxID);
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
    const std::list<uint16_t>& slots, bool adjustCounts)
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
        auto item = box->GetItems((size_t)slot).Get();

        if(!item)
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

            int64_t objectID = state->GetObjectID(item->GetUUID());
            if(objectID == 0)
            {
                objectID = server->GetNextObjectID();
                state->SetObjectID(item->GetUUID(), objectID);
            }
            reply.WriteS64Little(objectID);
        }

        GetItemDetailPacketData(reply, item);
    }

    client->SendPacket(reply);

    if(updateMode && adjustCounts)
    {
        // Recalculate the demon quest item count in case it changed
        auto dQuest = character->GetDemonQuest().Get();
        if(dQuest && box == character->GetItemBoxes(0).Get() &&
            dQuest->GetType() == objects::DemonQuest::Type_t::ITEM)
        {
            server->GetEventManager()->UpdateDemonQuestCount(client,
                objects::DemonQuest::Type_t::ITEM);
        }
    }
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
        auto item = box->GetItems(i).Get();
        if(item && item->GetType() == itemID)
        {
            existing.push_back(item);
        }
    }

    return existing;
}

uint32_t CharacterManager::GetExistingItemCount(
    const std::shared_ptr<objects::Character>& character,
    uint32_t itemID, std::shared_ptr<objects::ItemBox> box)
{
    uint32_t count = 0;

    auto existingItems = GetExistingItems(character, itemID, box);
    for(auto item : existingItems)
    {
        count = (uint32_t)(count + item->GetStackSize());
    }

    return count;
}

std::set<size_t> CharacterManager::GetFreeSlots(
    const std::shared_ptr<ChannelClientConnection>& client,
    std::shared_ptr<objects::ItemBox> box)
{
    std::set<size_t> slots;
    if(client)
    {
        if(box == nullptr)
        {
            auto cState = client->GetClientState()->GetCharacterState();
            auto character = cState->GetEntity();
            box = character ? character->GetItemBoxes(0).Get() : nullptr;
        }

        if(box)
        {
            for(size_t i = 0; i < 50; i++)
            {
                auto item = box->GetItems(i);
                if(item.IsNull())
                {
                    slots.insert(i);
                }
            }
        }
    }

    return slots;
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
    auto restr = def->GetRestriction();

    auto item = libcomp::PersistentObject::New<
        objects::Item>();

    for(uint8_t i = 0; i < restr->GetModSlots() && i < 5; i++)
    {
        item->SetModSlots((size_t)i, MOD_SLOT_NULL_EFFECT);
    }

    item->SetType(itemID);
    item->SetStackSize(stackSize);
    item->SetDurability((uint16_t)(poss->GetDurability() * 1000));
    item->SetMaxDurability((int8_t)poss->GetDurability());

    int32_t rentalTime = def->GetRental()->GetRental();
    if(rentalTime > 0)
    {
        item->SetRentalExpiration((uint32_t)(
            (int32_t)std::time(0) + rentalTime));
    }

    item->Register(item);

    return item;
}

bool CharacterManager::AddRemoveItems(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    std::unordered_map<uint32_t, uint32_t> itemCounts, bool add,
    int64_t skillTargetID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto itemBox = character->GetItemBoxes(0).Get();

    auto server = mServer.lock();
    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        state->GetAccountUID());

    const static bool autoCompressCurrency = server->GetWorldSharedConfig()
        ->GetAutoCompressCurrency();

    bool autoCompress = add && autoCompressCurrency;
    if(autoCompress)
    {
        // Compress macca
        auto it = itemCounts.find(SVR_CONST.ITEM_MACCA);
        if(it != itemCounts.end() && it->second >= ITEM_MACCA_NOTE_AMOUNT)
        {
            uint32_t maccaCount = it->second;
            uint32_t noteCount = (uint32_t)(maccaCount / ITEM_MACCA_NOTE_AMOUNT);
            maccaCount = (uint32_t)(maccaCount - (uint32_t)(noteCount *
                ITEM_MACCA_NOTE_AMOUNT));

            if(noteCount > 0)
            {
                itemCounts[SVR_CONST.ITEM_MACCA_NOTE] = noteCount;
            }

            if(maccaCount == 0)
            {
                itemCounts.erase(SVR_CONST.ITEM_MACCA);
            }
            else
            {
                itemCounts[SVR_CONST.ITEM_MACCA] = maccaCount;
            }
        }
        
        // Compress mag
        it = itemCounts.find(SVR_CONST.ITEM_MAGNETITE);
        if(it != itemCounts.end() && it->second >= ITEM_MAG_PRESSER_AMOUNT)
        {
            uint32_t magCount = it->second;
            uint32_t presserCount = (uint32_t)(magCount / ITEM_MAG_PRESSER_AMOUNT);
            magCount = (uint32_t)(magCount - (uint32_t)(presserCount *
                ITEM_MAG_PRESSER_AMOUNT));

            if(presserCount > 0)
            {
                itemCounts[SVR_CONST.ITEM_MAG_PRESSER] = presserCount;
            }

            if(magCount == 0)
            {
                itemCounts.erase(SVR_CONST.ITEM_MAGNETITE);
            }
            else
            {
                itemCounts[SVR_CONST.ITEM_MAGNETITE] = magCount;
            }
        }
    }

    // Loop until we're done
    std::list<uint16_t> updatedSlots;
    while(itemCounts.size() > 0)
    {
        auto itemPair = *itemCounts.begin();

        uint32_t itemType = itemPair.first;
        uint32_t quantity = itemPair.second;

        itemCounts.erase(itemType);

        auto def = server->GetDefinitionManager()->GetItemData(itemType);
        if(nullptr == def)
        {
            return false;
        }

        auto existing = GetExistingItems(character, itemType);
        uint32_t maxStack = (uint32_t)def->GetPossession()->GetStackSize();
        if(add)
        {
            bool compressible = autoCompress && (itemType == SVR_CONST.ITEM_MACCA ||
                itemType == SVR_CONST.ITEM_MAGNETITE);

            uint32_t quantityLeft = quantity;
            for(auto item : existing)
            {
                auto free = maxStack - item->GetStackSize();
                if(free > quantityLeft)
                {
                    quantityLeft = 0;
                }
                else
                {
                    quantityLeft = (uint32_t)(quantityLeft - free);
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
                uint32_t added = 0;
                for(auto item : existing)
                {
                    uint32_t free = (uint32_t)(maxStack - item->GetStackSize());

                    if(added < quantity && free > 0)
                    {
                        if(compressible)
                        {
                            uint32_t increaseItem = 0;
                            if(itemType == SVR_CONST.ITEM_MACCA &&
                                (uint32_t)(item->GetStackSize() + (quantity - added))
                                >= ITEM_MACCA_NOTE_AMOUNT)
                            {
                                increaseItem = SVR_CONST.ITEM_MACCA_NOTE;
                            }
                            else if(itemType == SVR_CONST.ITEM_MAGNETITE &&
                                (uint32_t)(item->GetStackSize() + (quantity - added))
                                >= ITEM_MAG_PRESSER_AMOUNT)
                            {
                                increaseItem = SVR_CONST.ITEM_MAG_PRESSER;
                            }

                            if(increaseItem)
                            {
                                // Remove the current item and add the compressed item
                                // to the set
                                itemBox->SetItems((size_t)item->GetBoxSlot(), NULLUUID);
                                updatedSlots.push_back((uint16_t)item->GetBoxSlot());
                                dbChanges->Delete(item);

                                // Free up the slot and re-sort
                                freeSlots.push_back((size_t)item->GetBoxSlot());
                                freeSlots.unique();
                                freeSlots.sort();

                                added = (uint32_t)(added + free);

                                if(itemCounts.find(increaseItem) == itemCounts.end())
                                {
                                    itemCounts[increaseItem] = 1;
                                }
                                else
                                {
                                    itemCounts[increaseItem] = (uint32_t)(
                                        itemCounts[increaseItem] + 1);
                                }

                                continue;
                            }
                        }

                        uint32_t delta = (uint32_t)(quantity - added);
                        if(free < delta)
                        {
                            delta = free;
                        }

                        item->SetStackSize((uint16_t)(item->GetStackSize() + delta));
                        updatedSlots.push_back((uint16_t)item->GetBoxSlot());
                        dbChanges->Update(item);

                        added = (uint32_t)(added + delta);
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
                        uint32_t delta = maxStack;
                        if((delta + added) > quantity)
                        {
                            delta = (uint32_t)(quantity - added);
                        }
                        added = (uint32_t)(added + delta);

                        auto item = GenerateItem(itemType, (uint16_t)delta);
                        item->SetItemBox(itemBox->GetUUID());
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

            uint32_t quantityLeft = quantity;
            for(auto item : existing)
            {
                if(item->GetStackSize() > quantityLeft)
                {
                    quantityLeft = 0;
                }
                else
                {
                    quantityLeft = (uint32_t)(quantityLeft - item->GetStackSize());
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

            uint32_t removed = 0;
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
                    removed = (uint32_t)(removed + item->GetStackSize());

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

uint64_t CharacterManager::GetTotalMacca(const std::shared_ptr<
    objects::Character>& character)
{
    auto macca = GetExistingItems(character, SVR_CONST.ITEM_MACCA,
        character->GetItemBoxes(0).Get());
    auto maccaNotes = GetExistingItems(character,
        SVR_CONST.ITEM_MACCA_NOTE, character->GetItemBoxes(0).Get());

    uint64_t totalMacca = 0;
    for(auto m : macca)
    {
        totalMacca += m->GetStackSize();
    }

    for(auto m : maccaNotes)
    {
        totalMacca += (uint64_t)(m->GetStackSize() * ITEM_MACCA_NOTE_AMOUNT);
    }

    return totalMacca;
}

bool CharacterManager::PayMacca(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint64_t amount)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    std::list<std::shared_ptr<objects::Item>> insertItems;
    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems;

    return CalculateMaccaPayment(client, amount, insertItems, stackAdjustItems)
        && UpdateItems(client, false, insertItems, stackAdjustItems);
}

bool CharacterManager::CalculateMaccaPayment(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint64_t amount,
    std::list<std::shared_ptr<objects::Item>>& insertItems,
    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t>& stackAdjustItems)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    auto macca = GetExistingItems(character, SVR_CONST.ITEM_MACCA, inventory);
    auto maccaNotes = GetExistingItems(character, SVR_CONST.ITEM_MACCA_NOTE,
        inventory);
    
    uint64_t totalMacca = 0;
    for(auto m : macca)
    {
        totalMacca += m->GetStackSize();
    }

    for(auto m : maccaNotes)
    {
        totalMacca += (uint64_t)(m->GetStackSize() * ITEM_MACCA_NOTE_AMOUNT);
    }

    if(totalMacca < amount)
    {
        return false;
    }

    // Remove last first, starting with macca
    macca.reverse();
    maccaNotes.reverse();

    uint16_t stackDecrease = 0;
    std::shared_ptr<objects::Item> updateItem;

    uint64_t amountLeft = amount;
    for(auto m : macca)
    {
        if(amountLeft == 0) break;

        auto stack = (uint64_t)m->GetStackSize();
        if(stack > amountLeft)
        {
            stackDecrease = (uint16_t)(stack - amountLeft);
            amountLeft = 0;
            updateItem = m;
        }
        else
        {
            amountLeft = (uint64_t)(amountLeft - stack);
            stackAdjustItems[m] = 0;
        }
    }

    for(auto m : maccaNotes)
    {
        if(amountLeft == 0) break;

        auto stack = m->GetStackSize();
        uint64_t stackAmount = (uint64_t)(stack * ITEM_MACCA_NOTE_AMOUNT);
        if(stackAmount > amountLeft)
        {
            int32_t maccaLeft = (int32_t)(stackAmount - amountLeft);

            stackDecrease = (uint16_t)(maccaLeft / ITEM_MACCA_NOTE_AMOUNT);
            maccaLeft = (int32_t)(maccaLeft % ITEM_MACCA_NOTE_AMOUNT);
            amountLeft = 0;

            if(stackDecrease == 0)
            {
                stackAdjustItems[m] = 0;
            }
            else
            {
                updateItem = m;
            }

            if(maccaLeft)
            {
                insertItems.push_back(
                    GenerateItem(SVR_CONST.ITEM_MACCA, (uint16_t)maccaLeft));
            }
        }
        else
        {
            amountLeft = (uint64_t)(amountLeft - stackAmount);
            stackAdjustItems[m] = 0;
        }
    }

    if(updateItem)
    {
        stackAdjustItems[updateItem] = stackDecrease;
    }

    return true;
}

uint64_t CharacterManager::CalculateItemRemoval(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint32_t itemID, uint64_t amount,
    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t>& stackAdjustItems)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto items = GetExistingItems(character, itemID);
    items.reverse();

    uint64_t left = amount;
    for(auto item : items)
    {
        if(!left) break;

        uint64_t stack = (uint64_t)item->GetStackSize();
        if(stack >= left)
        {
            stackAdjustItems[item] = (uint16_t)(stack - left);
            left = 0;
            break;
        }
        else
        {
            left = (uint64_t)(left - stack);
            stackAdjustItems[item] = 0;
        }
    }

    return left;
}

bool CharacterManager::UpdateItems(const std::shared_ptr<
    channel::ChannelClientConnection>& client, bool validateOnly,
    std::list<std::shared_ptr<objects::Item>>& insertItems,
    std::unordered_map<std::shared_ptr<objects::Item>, uint16_t> stackAdjustItems,
    bool notifyClient)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    std::list<int8_t> freeSlots;
    for(int8_t i = 0; i < 50; i++)
    {
        if(inventory->GetItems((size_t)i).IsNull())
        {
            freeSlots.push_back(i);
        }
    }

    // Determine new free slots from deletes
    for(auto pair : stackAdjustItems)
    {
        if(pair.second == 0)
        {
            freeSlots.push_back(pair.first->GetBoxSlot());
        }
    }

    freeSlots.unique();
    freeSlots.sort();

    if(freeSlots.size() < insertItems.size())
    {
        return false;
    }
    else if(validateOnly)
    {
        return true;
    }

    auto changes = libcomp::DatabaseChangeSet::Create();
    std::list<uint16_t> updatedSlots;

    for(auto iPair : stackAdjustItems)
    {
        auto item = iPair.first;
        if(iPair.second == 0)
        {
            // Delete the item
            UnequipItem(client, item);

            auto slot = item->GetBoxSlot();
            inventory->SetItems((size_t)slot, NULLUUID);
            changes->Delete(item);
            updatedSlots.push_back((uint16_t)slot);
        }
        else
        {
            // Update the stack size
            item->SetStackSize(iPair.second);
            changes->Update(item);
            updatedSlots.push_back((uint16_t)item->GetBoxSlot());
        }
    }

    for(auto item : insertItems)
    {
        auto slot = freeSlots.front();
        freeSlots.erase(freeSlots.begin());

        item->SetItemBox(inventory->GetUUID());
        item->SetBoxSlot(slot);
        inventory->SetItems((size_t)slot, item);
        changes->Insert(item);
        updatedSlots.push_back((uint16_t)slot);
    }

    changes->Update(inventory);

    // Process all changes as a transaction
    auto worldDB = mServer.lock()->GetWorldDatabase();
    if(!worldDB->ProcessChangeSet(changes))
    {
        return false;
    }

    updatedSlots.unique();
    updatedSlots.sort();

    if(notifyClient)
    {
        SendItemBoxData(client, inventory, updatedSlots);
    }

    return true;
}

std::list<std::shared_ptr<objects::ItemDrop>> CharacterManager::DetermineDrops(
    const std::list<std::shared_ptr<objects::ItemDrop>>& drops,
    int16_t luck, bool minLast)
{
    std::list<std::shared_ptr<objects::ItemDrop>> results;
    for(auto drop : drops)
    {
        double baseRate = (double)drop->GetRate();
        uint32_t dropRate = (uint32_t)(baseRate * 100.0);
        if(luck > 0)
        {
            // Scale drop rates based on luck, more for high drop rates and higher luck.
            // Estimates roughly to:
            // 75% base -> 76.47% at 10 luck, 87.26% at 30 luck, 100+% at 44+ luck
            // 50% base -> 51.83% at 20 luck, 57.05% at 40 luck, 100+% at 114+ luck
            // 10% base -> 10.57% at 40 luck, 22.7% at 200 luck, 100+% at 600+ luck
            // 1% base -> 3.33% at 300 luck, 6.83% at 500 luck, 12.78% at 750 luck
            // 0.1% base -> 0.89% at 600 luck, 1.39% at 800 luck, 1.95% at 999 luck
            double deltaDiff = (double)(100.0 - baseRate);
            dropRate = (uint32_t)(baseRate * (100.f +
                100.f * (float)(((double)luck / 30.0) * 10.0 * (double)luck) /
                (1000.0 + 7.0 * (double)luck + (deltaDiff * deltaDiff))));
        }

        float globalDropBonus = mServer.lock()->GetWorldSharedConfig()
            ->GetDropRateBonus();

        dropRate = (uint32_t)((double)dropRate * (double)(1.f + globalDropBonus));

        if(dropRate >= 10000 || RNG(uint16_t, 1, 10000) <= dropRate ||
            (minLast && results.size() == 0 && drops.back() == drop))
        {
            results.push_back(drop);
        }
    }

    return results;
}

bool CharacterManager::CreateLootFromDrops(const std::shared_ptr<objects::LootBox>& box,
    const std::list<std::shared_ptr<objects::ItemDrop>>& drops, int16_t luck, bool minLast)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    // Loop through the drops and sum up stacks
    std::unordered_map<uint32_t, uint32_t> itemStacks;
    for(auto drop : DetermineDrops(drops, luck, minLast))
    {
        uint16_t minStack = drop->GetMinStack();
        uint16_t maxStack = drop->GetMaxStack();

        // The drop rate is affected by luck but the stack size is not
        uint16_t stackSize = RNG(uint16_t, minStack, maxStack);

        auto it = itemStacks.find(drop->GetItemType());
        if(it != itemStacks.end())
        {
            it->second = (uint32_t)(it->second + stackSize);
        }
        else
        {
            itemStacks[drop->GetItemType()] = (uint32_t)stackSize;
        }
    }

    // Loop back through and create the items with the combined stacks
    std::set<uint32_t> addedItems;
    std::list<std::shared_ptr<objects::Loot>> lootItems;
    for(auto drop : drops)
    {
        auto it = itemStacks.find(drop->GetItemType());
        if(it != itemStacks.end() &&
            addedItems.find(drop->GetItemType()) == addedItems.end())
        {
            auto itemDef = definitionManager->GetItemData(
                drop->GetItemType());

            addedItems.insert(drop->GetItemType());

            uint32_t stackSize = it->second;
            uint16_t maxStackSize = itemDef->GetPossession()->GetStackSize();
            uint16_t stackCount = (uint8_t)ceill((double)stackSize /
                (double)maxStackSize);

            for(uint16_t i = 0; i < stackCount &&
                lootItems.size() < box->LootCount(); i++)
            {
                uint16_t stack = stackSize <= (uint32_t)maxStackSize
                    ? (uint16_t)stackSize : maxStackSize;
                stackSize = (uint32_t)(stackSize - (uint32_t)stack);

                auto loot = std::make_shared<objects::Loot>();
                loot->SetType(drop->GetItemType());
                loot->SetCount(stack);
                lootItems.push_back(loot);
            }

            if(lootItems.size() == 12)
            {
                break;
            }

            // Remove it from the set so its not generated twice
            itemStacks.erase(it);
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

    if(nullptr == equip)
    {
        return;
    }

    bool inInventory = equip->GetItemBox() == character->GetItemBoxes(0).GetUUID();

    auto slot = objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE;

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto def = definitionManager->GetItemData(equip->GetType());
    if (nullptr != def)
    {
        slot = def->GetBasic()->GetEquipType();
    }

    if(slot == objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE)
    {
        return;
    }

    uint8_t stockCount = cState->GetMaxFusionGaugeStocks();

    bool unequip = false;
    auto equipSlot = character->GetEquippedItems((size_t)slot);
    if(equipSlot.Get() == equip)
    {
        // Unequip from anywhere
        equipSlot.SetReference(nullptr);
        unequip = true;
    }
    else if(!inInventory)
    {
        // Only equip from inventory
        return;
    }
    else
    {
        equipSlot.SetReference(equip);
    }

    character->SetEquippedItems((size_t)slot, equipSlot);

    // Determine which complete sets are equipped now
    cState->RecalcEquipState(definitionManager);

    // Recalculate tokusei and stats to reflect equipment changes
    server->GetTokuseiManager()->Recalculate(cState, true,
        std::set<int32_t>{ cState->GetEntityID() });
    RecalculateStats(cState, client, false);

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

    reply.WriteS16Little((int16_t)cState->GetMaxHP());
    reply.WriteS16Little((int16_t)cState->GetMaxMP());

    server->GetZoneManager()->BroadcastPacket(client, reply, false);

    // If the stock count changed notify the client
    if(stockCount != cState->GetMaxFusionGaugeStocks())
    {
        stockCount = cState->GetMaxFusionGaugeStocks();
        if(character->GetFusionGauge() > (uint32_t)(stockCount * 10000))
        {
            // Reset to max
            character->SetFusionGauge((uint32_t)(stockCount * 10000));
        }

        SendFusionGauge(client);
    }
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

bool CharacterManager::UpdateDurability(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::shared_ptr<objects::Item>& item, int32_t points,
    bool isAdjust, bool updateMax, bool sendPacket)
{
    if(!item)
    {
        return false;
    }

    std::unordered_map<std::shared_ptr<objects::Item>, int32_t> items;
    items[item] = points;

    return UpdateDurability(client, items, isAdjust, updateMax, sendPacket);
}

bool CharacterManager::UpdateDurability(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::unordered_map<std::shared_ptr<objects::Item>, int32_t>& items,
    bool isAdjust, bool updateMax, bool sendPacket)
{
    if(!client || items.size() == 0)
    {
        return false;
    }

    auto server = mServer.lock();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    bool recalc = false;
    bool updated = false;
    for(auto itemPair : items)
    {
        auto item = itemPair.first;
        int32_t points = itemPair.second;

        bool update = false;
        if(updateMax)
        {
            int8_t current = item->GetMaxDurability();

            auto itemData = server->GetDefinitionManager()
                ->GetItemData(item->GetType());
            int32_t maxDurability = itemData ?
                (int32_t)itemData->GetPossession()->GetDurability() : 0;

            int32_t newValue = isAdjust ? (current + points) : points;
            if(newValue < 0)
            {
                newValue = 0;
            }
            else if(newValue > maxDurability)
            {
                newValue = maxDurability;
            }

            if(newValue == 0)
            {
                // Item is broken, remove it
                UnequipItem(client, item);
                item->SetDurability(0);
                item->SetMaxDurability(0);

                std::list<std::shared_ptr<objects::Item>> empty;
                std::unordered_map<std::shared_ptr<
                    objects::Item>, uint16_t> updateItems;
                updateItems[item] = 0;

                return UpdateItems(client, false, empty, updateItems);
            }
            else if(newValue != (int32_t)current)
            {
                // Max durability reduced
                item->SetMaxDurability((int8_t)newValue);

                // Reduce current durability if higher than new max
                if(item->GetDurability() > (uint16_t)(newValue * 1000))
                {
                    item->SetDurability((uint16_t)(newValue * 1000));
                }
                else if(newValue > (int32_t)current)
                {
                    // Increase the current durability by the proportional amount
                    uint16_t durability = item->GetDurability();
                    item->SetDurability((uint16_t)(durability +
                        (newValue - current) * 1000));
                }

                // Always update when changing max durability
                update = true;
            }
        }
        else
        {
            uint16_t current = item->GetDurability();

            if(points < 0 && current == 0)
            {
                // Cannot reduce further
                return false;
            }

            int32_t newValue = isAdjust ? (current + points) : points;
            if(newValue < 0)
            {
                newValue = 0;
            }
            else if(newValue > item->GetMaxDurability() * 1000)
            {
                newValue = (item->GetMaxDurability() * 1000);
            }

            if(newValue != (int32_t)current)
            {
                item->SetDurability((uint16_t)newValue);

                //Only update if the visible durability changes
                update = ceil(newValue * 0.001) != ceil(current * 0.001);

                // If changing to/from 0, reclaculate stats and tokusei
                recalc = (newValue == 0) != (current == 0);
            }
        }

        if(update)
        {
            if(sendPacket)
            {
                auto itemBox = std::dynamic_pointer_cast<objects::ItemBox>(
                    libcomp::PersistentObject::GetObjectByUUID(item->GetItemBox()));
                if(itemBox)
                {
                    SendItemBoxData(client, itemBox,
                        { (uint16_t)item->GetBoxSlot() });
                }
            }

            server->GetWorldDatabase()->QueueUpdate(item, state->GetAccountUID());

            updated = true;
        }
    }

    if(recalc)
    {
        // Enable/disable tokusei on equipment including set bonuses
        cState->RecalcEquipState(server->GetDefinitionManager());
        RecalculateTokuseiAndStats(cState, client);
    }

    return updated;
}

bool CharacterManager::IsCPItem(const std::shared_ptr<objects::MiItemData>& itemData) const
{
    return itemData && (itemData->GetBasic()->GetFlags() & 0x40) != 0;
}

void CharacterManager::EndExchange(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int32_t outcome)
{
    auto state = client->GetClientState();
    auto exchange = state->GetExchangeSession();

    if(exchange)
    {
        switch(exchange->GetType())
        {
        case objects::PlayerExchangeSession::Type_t::TRADE:
            {
                libcomp::Packet p;
                p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_TRADE_ENDED);
                p.WriteS32Little(outcome);

                client->QueuePacket(p);
            }
            break;
        case objects::PlayerExchangeSession::Type_t::CRYSTALLIZE:
        case objects::PlayerExchangeSession::Type_t::ENCHANT_SOUL:
        case objects::PlayerExchangeSession::Type_t::ENCHANT_TAROT:
        case objects::PlayerExchangeSession::Type_t::SYNTH_MELEE:
        case objects::PlayerExchangeSession::Type_t::SYNTH_GUN:
            {
                libcomp::Packet p;
                p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ENTRUST_FINISH);
                p.WriteS32Little(outcome);

                client->QueuePacket(p);
            }
            break;
        case objects::PlayerExchangeSession::Type_t::TRIFUSION_GUEST:
        case objects::PlayerExchangeSession::Type_t::TRIFUSION_HOST:
            mServer.lock()->GetFusionManager()->EndExchange(client);
            return;
        default:
            break;
        }

        state->SetExchangeSession(nullptr);
        SetStatusIcon(client, 0);

        client->FlushOutgoing();
    }
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

    server->GetTokuseiManager()->Recalculate(cState,
        std::set<TokuseiConditionType> { TokuseiConditionType::LNC });
}

std::shared_ptr<objects::Demon> CharacterManager::ContractDemon(
    const std::shared_ptr<channel::ChannelClientConnection>& client,
    const std::shared_ptr<objects::MiDevilData>& demonData,
    int32_t sourceEntityID, uint16_t familiarity)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    auto demon = ContractDemon(character, demonData, familiarity);

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
    const std::shared_ptr<objects::MiDevilData>& demonData,
    uint16_t familiarity)
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

    auto d = GenerateDemon(demonData, familiarity);
    if(!d)
    {
        LOG_ERROR("Failed to generate demon.\n");

        return nullptr;
    }

    auto ds = d->GetCoreStats().Get();

    d->SetDemonBox(comp->GetUUID());
    d->SetBoxSlot(compSlot);

    comp->SetDemons((size_t)compSlot, d);

    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        character->GetAccount());
    dbChanges->Insert(d);
    dbChanges->Insert(ds);
    dbChanges->Update(comp);

    auto server = mServer.lock();
    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    return d;
}

std::shared_ptr<objects::Demon> CharacterManager::GenerateDemon(
    const std::shared_ptr<objects::MiDevilData>& demonData,
    uint16_t familiarity)
{
    if(nullptr == demonData)
    {
        return nullptr;
    }

    //Create a new demon from it's defaults
    auto growth = demonData->GetGrowth();

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto d = libcomp::PersistentObject::New<
        objects::Demon>(true);
    d->SetType(demonData->GetBasic()->GetID());
    d->SetGrowthType(demonData->GetGrowth()->GetGrowthType());
    d->SetFamiliarity(familiarity);

    int8_t level = (int8_t)growth->GetBaseLevel();

    // Don't create over max level, even if base is higher
    int8_t levelCap = server->GetWorldSharedConfig()->GetLevelCap();
    if(level > levelCap)
    {
        level = levelCap;
    }

    auto ds = libcomp::PersistentObject::New<
        objects::EntityStats>(true);
    ds->SetLevel(level);
    d->SetCoreStats(ds);

    CalculateDemonBaseStats(d);

    // Add learned skills
    for(size_t i = 0; i < 8; i++)
    {
        uint32_t skillID = growth->GetSkills(i);
        auto skillData = skillID
            ? definitionManager->GetSkillData(skillID) : nullptr;
        if(skillData &&
            skillData->GetCommon()->GetCategory()->GetMainCategory() != 2)
        {
            // Switch skills were never supported by the client when sent
            // from partner demons so only add if it is not a switch skill
            d->SetLearnedSkills(i, skillID);
        }
    }

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
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        return;
    }

    uint16_t current = demon->GetFamiliarity();
    int32_t newFamiliarity = isAdjust ? 0 : (int32_t)familiarity;
    if(isAdjust && familiarity != 0)
    {
        auto tokuseiManager = mServer.lock()->GetTokuseiManager();

        // Since familiarity rate adjustments cannot be bound to
        // skills, scale all incoming adjustments here
        if(familiarity > 0)
        {
            // Familiarity up adjustments are on the character
            familiarity = (int32_t)((double)familiarity *
                (1.0 + tokuseiManager->GetAspectSum(cState,
                    TokuseiAspectType::FAMILIARITY_UP_RATE) * 0.01));
        }
        else
        {
            // Familiarity down adjustments are on the demon
            familiarity = (int32_t)((double)familiarity *
                (1.0 + tokuseiManager->GetAspectSum(dState,
                    TokuseiAspectType::FAMILIARITY_DOWN_RATE) * 0.01));
        }

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

        server->GetTokuseiManager()->Recalculate(cState,
            std::set<TokuseiConditionType> { TokuseiConditionType::PARTNER_FAMILIARITY });

        // Rank adjustments will change base stats
        if(oldRank != newRank)
        {
            CalculateDemonBaseStats(dState->GetEntity());
            RecalculateStats(dState, client);

            // Only update the DB and clients if the rank changed
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
}

int32_t CharacterManager::UpdateSoulPoints(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int32_t points,
    bool isAdjust, bool applyRate)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        return 0;
    }

    int32_t current = demon->GetSoulPoints();
    int32_t newPoints = isAdjust ? 0 : (int32_t)points;
    if(isAdjust && points != 0)
    {
        if(applyRate && points > 0)
        {
            auto tokuseiManager = mServer.lock()->GetTokuseiManager();

            // Soul point rate is on the character
            points = (int32_t)((double)points *
                (1.0 + tokuseiManager->GetAspectSum(cState,
                    TokuseiAspectType::SOUL_POINT_RATE) * 0.01));
        }

        newPoints = current + points;
    }

    if(newPoints > MAX_SOUL_POINTS)
    {
        newPoints = MAX_SOUL_POINTS;
    }

    if(newPoints < 0)
    {
        newPoints = 0;
    }

    if(current != newPoints)
    {
        auto server = mServer.lock();

        demon->SetSoulPoints(newPoints);

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SOUL_POINT_UPDATE);
        p.WriteS32Little(dState->GetEntityID());
        p.WriteS32Little(newPoints);

        client->SendPacket(p);

        server->GetWorldDatabase()->QueueUpdate(demon, state->GetAccountUID());
    }

    return points;
}

void CharacterManager::UpdateFusionGauge(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int32_t points, bool isAdjust)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    int32_t current = (int32_t)character->GetFusionGauge();
    int32_t newPoints = isAdjust ? current + points : points;
    uint8_t maxStocks = cState->GetMaxFusionGaugeStocks();

    if(newPoints > (int32_t)(maxStocks * 10000))
    {
        newPoints = (int32_t)(maxStocks * 10000);
    }

    if(newPoints < 0 || !HasValuable(character,
        SVR_CONST.VALUABLE_FUSION_GAUGE))
    {
        newPoints = 0;
    }

    if(current != newPoints)
    {
        character->SetFusionGauge((uint32_t)newPoints);

        // If the visible percentage changed, send to the client
        if((current / 100) != (newPoints / 100))
        {
            SendFusionGauge(client);
        }
    }
}

void CharacterManager::SendFusionGauge(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    libcomp::Packet notify;
    notify.WritePacketCode(ChannelToClientPacketCode_t::PACKET_FUSION_GAUGE);
    notify.WriteS32Little((int32_t)cState->GetEntity()->GetFusionGauge());
    notify.WriteU8(cState->GetMaxFusionGaugeStocks());

    client->SendPacket(notify);
}

void CharacterManager::ExperienceGain(const std::shared_ptr<
    channel::ChannelClientConnection>& client, uint64_t xpGain, int32_t entityID)
{
    auto server = mServer.lock();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    auto eState = state->GetEntityState(entityID);
    auto stats = eState->GetCoreStats();
    if(!eState->Ready() || !stats)
    {
        return;
    }

    int8_t levelCap = server->GetWorldSharedConfig()->GetLevelCap();

    auto level = stats->GetLevel();
    if(level >= levelCap)
    {
        return;
    }

    bool isDemon = false;
    auto demonData = eState->GetDevilData();
    int32_t fType = 0;

    std::set<uint32_t> demonSkills;
    if(eState == dState)
    {
        isDemon = true;

        // Demons cannot level when dead
        if(!dState->IsAlive() || !demonData)
        {
            return;
        }

        fType = demonData->GetFamiliarity()->GetFamiliarityType();

        // Gather all skills so nothing is "re-acquired"
        for(uint32_t skillID : demon->GetLearnedSkills())
        {
            if(skillID)
            {
                demonSkills.insert(skillID);
            }
        }

        for(uint32_t skillID : demon->GetAcquiredSkills())
        {
            if(skillID)
            {
                demonSkills.insert(skillID);
            }
        }

        for(auto iSkill : demon->GetInheritedSkills())
        {
            if(iSkill)
            {
                demonSkills.insert(iSkill->GetSkill());
            }
        }
    }

    int64_t xpDelta = stats->GetXP() + (int64_t)xpGain;
    while(level < levelCap &&
        xpDelta >= (int64_t)libcomp::LEVEL_XP_REQUIREMENTS[level])
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
                if(acSkill->GetLevel() == (uint32_t)level &&
                    demonSkills.find(acSkill->GetID()) == demonSkills.end())
                {
                    demon->AppendAcquiredSkills(acSkill->GetID());
                    newSkills.push_back(acSkill->GetID());
                    demonSkills.insert(acSkill->GetID());
                }
            }

            CalculateDemonBaseStats(dState->GetEntity());
            server->GetTokuseiManager()->Recalculate(cState, true,
                std::set<int32_t>{ dState->GetEntityID() });
            RecalculateStats(dState, client, false);
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
            server->GetTokuseiManager()->Recalculate(cState, true,
                std::set<int32_t>{ cState->GetEntityID() });
            RecalculateStats(cState, client, false);
            if(cState->IsAlive())
            {
                stats->SetHP(cState->GetMaxHP());
                stats->SetMP(cState->GetMaxMP());
            }

            int32_t points = (int32_t)(floorl((float)level / 5) + 2);
            character->SetPoints(character->GetPoints() + points);

            reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_CHARACTER_LEVEL_UP);
            reply.WriteS32Little(entityID);
            reply.WriteS32(0);  //Unknown
            reply.WriteS8(level);
            reply.WriteS64(xpDelta);
            reply.WriteS16Little((int16_t)cState->GetMaxHP());
            reply.WriteS16Little((int16_t)cState->GetMaxMP());
            reply.WriteS32Little(points);
        }

        server->GetZoneManager()->BroadcastPacket(client, reply, true);
    }

    if(level == levelCap)
    {
        xpDelta = 0;
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
    channel::ChannelClientConnection>& client, uint32_t skillID,
    uint16_t rateBoost, float multiplier)
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
    else if(skill->GetCommon()->GetCategory()->GetMainCategory() == 2)
    {
        // Switch skills should never grant expertise
        return;
    }

    if(multiplier <= 0.f)
    {
        // Value not overridden, use 100% + adjustments
        multiplier = (float)cState->GetCorrectValue(CorrectTbl::RATE_EXPERTISE) * 0.01f;
    }

    std::list<std::pair<uint8_t, int32_t>> pointMap;
    for(auto expertGrowth : skill->GetExpertGrowth())
    {
        auto expertise = character->GetExpertises(expertGrowth->GetExpertiseID()).Get();

        // If it hasn't been created, it is disabled
        if(nullptr == expertise || expertise->GetDisabled()) continue;

        auto expDef = definitionManager->GetExpertClassData(expertGrowth->GetExpertiseID());
        if(expDef)
        {
            // Calculate the point gain
            /// @todo: validate
            int32_t gain = (int32_t)((float)(3954.482803f /
                (((float)expertise->GetPoints() * 0.01f) + 158.1808409f)
                * (expertGrowth->GetGrowthRate() + (float)rateBoost)) * 100.f * multiplier);
            if(gain > 0)
            {
                pointMap.push_back(std::pair<uint8_t, int32_t>(
                    expertGrowth->GetExpertiseID(), gain));
            }
        }
    }

    if(pointMap.size() > 0)
    {
        UpdateExpertisePoints(client, pointMap);
    }
}

void CharacterManager::UpdateExpertisePoints(const std::shared_ptr<
    channel::ChannelClientConnection>& client,
    const std::list<std::pair<uint8_t, int32_t>>& pointMap, bool force)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    int32_t maxTotalPoints = GetMaxExpertisePoints(character);

    int32_t currentPoints = 0;
    for(auto expertise : character->GetExpertises())
    {
        if(!expertise.IsNull())
        {
            currentPoints = currentPoints + expertise->GetPoints();
        }
    }

    bool rankChanged = false;

    std::list<std::pair<uint8_t, int32_t>> raised;
    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        state->GetAccountUID());
    for(auto pointPair : pointMap)
    {
        auto expDef = definitionManager->GetExpertClassData(pointPair.first);

        if(!expDef) continue;

        auto expertise = character->GetExpertises(pointPair.first).Get();
        if(!expertise)
        {
            if(force)
            {
                // Create it
                expertise = libcomp::PersistentObject::New<objects::Expertise>(
                    true);
                expertise->SetExpertiseID(pointPair.first);
                expertise->SetCharacter(character->GetUUID());
                expertise->SetDisabled(true);

                character->SetExpertises((size_t)pointPair.first, expertise);

                dbChanges->Update(character);
                dbChanges->Insert(expertise);

                server->GetWorldDatabase()->QueueChangeSet(dbChanges);
            }
            else
            {
                continue;
            }
        }
        else if(expertise->GetDisabled() && !force)
        {
            continue;
        }

        int32_t maxPoints = (expDef->GetMaxClass() * 100 * 1000)
            + (expDef->GetMaxRank() * 100 * 100);

        int32_t expPoints = expertise->GetPoints();
        int8_t currentRank = (int8_t)floorl((float)expPoints * 0.0001f);

        int32_t adjust = pointPair.second;
        if(adjust > 0)
        {
            if(expPoints == maxPoints) continue;

            // Don't exceed the max total points
            if((currentPoints + adjust) > maxTotalPoints)
            {
                adjust = maxTotalPoints - currentPoints;
            }

            // Don't exceed max expertise points
            if((expPoints + adjust) > maxPoints)
            {
                adjust = maxPoints - expPoints;
            }
        }
        else if(adjust < 0)
        {
            // Do not decrease below 0
            if((expPoints - adjust) < 0)
            {
                adjust = expPoints;
            }
        }

        if(adjust == 0) continue;

        currentPoints = currentPoints + adjust;
        expPoints += adjust;

        expertise->SetPoints(expPoints);

        int8_t newRank = (int8_t)((float)expPoints * 0.0001f);

        rankChanged |= currentRank != newRank;
        if(adjust > 0)
        {
            // Points up
            raised.push_back(std::make_pair(pointPair.first, expPoints));

            if(currentRank != newRank)
            {
                libcomp::Packet reply;
                reply.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_EXPERTISE_RANK_UP);
                reply.WriteS32Little(cState->GetEntityID());
                reply.WriteS8((int8_t)expDef->GetID());
                reply.WriteS8(newRank);

                server->GetZoneManager()->BroadcastPacket(client, reply);
            }
        }
        else
        {
            // Points down
            libcomp::Packet notify;
            notify.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_EXPERTISE_DOWN);
            notify.WriteS32Little(cState->GetEntityID());
            notify.WriteS8(1);  // Success
            notify.WriteS8((int8_t)expDef->GetID());
            notify.WriteS32Little(expPoints);

            server->GetZoneManager()->BroadcastPacket(client, notify);
        }

        dbChanges->Update(expertise);
    }

    if(raised.size() > 0)
    {
        libcomp::Packet reply;
        reply.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_EXPERTISE_POINT_UPDATE);
        reply.WriteS32Little(cState->GetEntityID());
        reply.WriteS32Little((int32_t)raised.size());
        for(auto update : raised)
        {
            reply.WriteS8((int8_t)update.first);
            reply.WriteS32Little(update.second);
        }

        client->SendPacket(reply);
    }

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    if(rankChanged)
    {
        //Expertises can be used as multipliers and conditions, always recalc
        cState->RecalcDisabledSkills(definitionManager);
        RecalculateTokuseiAndStats(cState, client);
    }
}

int32_t CharacterManager::GetMaxExpertisePoints(const std::shared_ptr<
    objects::Character>& character)
{
    auto stats = character->GetCoreStats();

    int32_t maxPoints = 1700000 + (int32_t)(floorl((float)stats->GetLevel() * 0.1) *
        1000 * 100) + ((int32_t)character->GetExpertiseExtension() * 1000 * 100);

    if(stats->GetLevel() == 99)
    {
        // Level 99 awards a bonus 1000.00 points available
        maxPoints = maxPoints + 100000;
    }

    return maxPoints;
}

void CharacterManager::SendExertiseExtension(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(character)
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EXPERTISE_EXTENSION);
        p.WriteS8(character->GetExpertiseExtension());

        client->SendPacket(p);
    }
}

void CharacterManager::UpdateSkillPoints(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int32_t points)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    if(character)
    {
        character->SetPoints(character->GetPoints() + points);

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SKILL_POINT_UPDATE);
        p.WriteS32Little(character->GetPoints());

        client->SendPacket(p);

        mServer.lock()->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());
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
        if(def->GetCommon()->GetCategory()->GetMainCategory() == 2)
        {
            // Switch skills are not supported on partner demons
            return false;
        }

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

        auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());

        // Demon skills are learned as 100% progress inherited skills
        auto iSkill = libcomp::PersistentObject::New<
            objects::InheritedSkill>(true);
        iSkill->SetSkill(skillID);
        iSkill->SetProgress(MAX_INHERIT_SKILL);
        iSkill->SetDemon(demon->GetUUID());

        demon->AppendInheritedSkills(iSkill);

        dbChanges->Insert(iSkill);
        dbChanges->Update(demon);

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_INHERIT_SKILL_UPDATED);
        p.WriteS32Little(eState->GetEntityID());
        p.WriteS32Little(1);
        p.WriteU32Little(skillID);
        p.WriteS32Little(MAX_INHERIT_SKILL);

        client->SendPacket(p);

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);
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

        RecalculateTokuseiAndStats(eState, client);
    }

    return true;
}

bool CharacterManager::GetSynthOutcome(ClientState* synthState,
    const std::shared_ptr<objects::PlayerExchangeSession>& exchangeSession,
    uint32_t& outcomeItemType, std::list<int32_t>& successRates, int16_t* effectID)
{
    successRates.clear();
    outcomeItemType = static_cast<uint32_t>(-1);

    if(!synthState || !exchangeSession)
    {
        return false;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto cState = synthState->GetCharacterState();
    auto dState = synthState->GetDemonState();

    bool isSoul = exchangeSession->GetType() ==
        objects::PlayerExchangeSession::Type_t::ENCHANT_SOUL;
    bool isTarot = exchangeSession->GetType() ==
        objects::PlayerExchangeSession::Type_t::ENCHANT_TAROT;

    std::list<double> rates;
    if(isSoul || isTarot)
    {
        auto inputItem = exchangeSession->GetItems(0).Get();
        auto crystal = exchangeSession->GetItems(1).Get();
        auto boostItem = exchangeSession->GetItems(2).Get();

        if(crystal && inputItem)
        {
            auto enchantData = definitionManager->GetEnchantDataByItemID(
                crystal->GetType());
            auto itemData = definitionManager->GetItemData(inputItem->GetType());
            if(!enchantData || !itemData || !effectID ||
                inputItem->GetDurability() == 0)
            {
                return false;
            }

            *effectID = enchantData->GetID();

            double expRank = (double)cState->GetExpertiseRank(
                definitionManager, EXPERTISE_CHAIN_SYNTHESIS);

            double boostRate = 0.0;
            if(boostItem)
            {
                auto it = SVR_CONST.SYNTH_ADJUSTMENTS.find(boostItem->GetType());
                if(it != SVR_CONST.SYNTH_ADJUSTMENTS.end() && it->second[0] == 0)
                {
                    boostRate = (double)it->second[1];
                }
            }

            // If the input is a CP item, the rate increases
            double cpBoost = IsCPItem(itemData) ? 20.0 : 0.0;

            double demonBoost = 0.0;
            if(dState->Ready())
            {
                int16_t intel = dState->GetINTEL();
                int16_t luck = dState->GetLUCK();
                for(auto pair : SVR_CONST.SYNTH_ADJUSTMENTS)
                {
                    // Skill adjustments
                    if(pair.second[0] == 1 && dState->CurrentSkillsContains(
                        (uint32_t)pair.first))
                    {
                        demonBoost = demonBoost +
                            (double)(intel + luck) / (double)pair.second[1];
                    }
                }
            }

            double rate = 0.0;
            if(isTarot)
            {
                auto tarotData = enchantData->GetDevilCrystal()->GetTarot();
                double diff = (double)tarotData->GetDifficulty();

                rate = floor((double)cState->GetINTEL() / 5.0 +
                    (double)cState->GetLUCK() / 10.0 + expRank / 2.0 + (30.0 - diff) +
                    cpBoost + demonBoost + boostRate);
            }
            else
            {
                auto soulData = enchantData->GetDevilCrystal()->GetSoul();
                double diff = (double)soulData->GetDifficulty();

                rate = floor((double)cState->GetINTEL() / 10.0 +
                    (double)cState->GetLUCK() / 5.0 + expRank + (20 - diff) +
                    cpBoost + demonBoost + boostRate);
            }

            rates.push_back(rate);
            rate = 0.0;

            // Determine special enchant result
            auto specialEnchants = definitionManager->GetEnchantSpecialDataByInputItem(
                inputItem->GetType());
            for(auto specialEnchant : specialEnchants)
            {
                double diff = (double)specialEnchant->GetDifficulty();

                bool match = false;
                if(isTarot)
                {
                    if(inputItem->GetSoul() == specialEnchant->GetSoul() &&
                        *effectID == specialEnchant->GetTarot())
                    {
                        rate = floor((double)cState->GetINTEL() / 5.0 +
                            (double)cState->GetLUCK() / 10.0 + expRank / 2.0 +
                            (30.0 - diff) + cpBoost + demonBoost + boostRate);

                        match = true;
                    }
                }
                else
                {
                    if(inputItem->GetTarot() == specialEnchant->GetTarot() &&
                        *effectID == specialEnchant->GetSoul())
                    {
                        rate = floor((double)cState->GetINTEL() / 10.0 +
                            (double)cState->GetLUCK() / 5.0 + expRank + (20 - diff) +
                            cpBoost + demonBoost + boostRate);

                        match = true;
                    }
                }

                if(match)
                {
                    outcomeItemType = specialEnchant->GetResultItem();
                    rates.push_back(rate);

                    // There should never be multiple but break just in case
                    break;
                }
            }
        }
    }
    else if(exchangeSession->GetType() ==
        objects::PlayerExchangeSession::Type_t::CRYSTALLIZE)
    {
        auto inputItem = exchangeSession->GetItems(0).Get();

        auto targetCState = std::dynamic_pointer_cast<CharacterState>(
            exchangeSession->GetOtherCharacterState());
        auto targetDemon = targetCState
            ? targetCState->GetEntity()->GetActiveDemon().Get() : nullptr;
        if(targetDemon && inputItem)
        {
            auto demonData = definitionManager->GetDevilData(targetDemon->GetType());
            auto enchantData = demonData? definitionManager->GetEnchantDataByDemonID(
                demonData->GetUnionData()->GetBaseDemonID()) : nullptr;
            if(!enchantData || !demonData)
            {
                return false;
            }

            // Make sure the crystal being used is valid
            auto it = SVR_CONST.DEMON_CRYSTALS.find(inputItem->GetType());
            if(it == SVR_CONST.DEMON_CRYSTALS.end() || it->second.find(
                (uint8_t)demonData->GetCategory()->GetRace()) == it->second.end())
            {
                return false;
            }

            outcomeItemType = enchantData->GetDevilCrystal()->GetItemID();

            double diff = (double)enchantData->GetDevilCrystal()->GetDifficulty();

            double expRank = (double)cState->GetExpertiseRank(
                definitionManager, EXPERTISE_CHAIN_SYNTHESIS);

            double fam = (double)targetDemon->GetFamiliarity();

            rates.push_back(floor((double)cState->GetINTEL() / 10.0 +
                (double)cState->GetLUCK() / 10.0 + expRank / 2.0 + (100.0 - diff) +
                (fam - 10000.0) / 100.0));
        }
    }
    else
    {
        return false;
    }

    auto clock = server->GetWorldClockTime();

    for(double& rate : rates)
    {
        if(clock.MoonPhase == 8)
        {
            // Full moon boosts success rates
            rate = floor(rate * 1.2);
        }

        if(rate < 0.0)
        {
            rate = 0.0;
        }
        else if(rate > 100.0)
        {
            rate = 100.0;
        }

        successRates.push_back((int32_t)rate);
    }

    return true;
}

void CharacterManager::ConvertIDToMaskValues(uint16_t id, size_t& index,
    uint8_t& shiftVal)
{
    index = (size_t)floor(id / 8);
    shiftVal = (uint8_t)(1 << (id % 8));
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

        auto server = mServer.lock();
        server->GetWorldDatabase()->QueueUpdate(progress, state->GetAccountUID());

        // If valuable is the V2 compendium, recalc boosts for demon
        if(valuableID == SVR_CONST.VALUABLE_DEVIL_BOOK_V2)
        {
            auto dState = state->GetDemonState();
            auto definitionManager = server->GetDefinitionManager();
            if(dState->UpdateSharedState(character, definitionManager))
            {
                server->GetTokuseiManager()->Recalculate(cState, true);
            }
        }
    }

    return true;
}

bool CharacterManager::HasValuable(const std::shared_ptr<objects::Character>& character,
    uint16_t valuableID)
{
    auto progress = character ? character->GetProgress().Get() : nullptr;

    size_t index;
    uint8_t shiftVal;
    ConvertIDToMaskValues(valuableID, index, shiftVal);

    uint8_t indexVal = progress ? progress->GetValuables(index) : 0;

    return (indexVal & shiftVal) != 0;
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

void CharacterManager::SendPluginFlags(const std::shared_ptr<
    ChannelClientConnection>& client)
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

bool CharacterManager::AddTitle(const std::shared_ptr<
    ChannelClientConnection>& client, int16_t titleID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    bool updated = false;
    if(titleID < 1024)
    {
        // Special title

        size_t index;
        uint8_t shiftVal;
        ConvertIDToMaskValues((uint16_t)titleID, index, shiftVal);

        auto oldValue = progress->GetSpecialTitles(index);
        uint8_t newValue = static_cast<uint8_t>(oldValue | shiftVal);

        if(oldValue != newValue)
        {
            progress->SetSpecialTitles(index,
                (uint8_t)(shiftVal | progress->GetSpecialTitles(index)));
            updated = true;
        }
    }
    else
    {
        // Normal title

        std::set<int16_t> existingTitles;
        for(int16_t title : progress->GetTitles())
        {
            existingTitles.insert(title);
        }

        // Push the new title to the end of the list and erase the last if
        // at max size
        if(existingTitles.find(titleID) == existingTitles.end())
        {
            progress->RemoveTitles(49);
            progress->PrependTitles(titleID);
            updated = true;
        }
    }

    if(updated)
    {
        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TITLE_LIST_UPDATED);
        notify.WriteS16Little((int16_t)titleID);

        auto titles = progress->GetTitles();

        notify.WriteS32Little((int32_t)titles.size());
        for(int16_t title : titles)
        {
            notify.WriteS16Little(title);
        }

        client->SendPacket(notify);

        mServer.lock()->GetWorldDatabase()->QueueUpdate(progress);

        return true;
    }

    return false;
}

void CharacterManager::SendMaterials(const std::shared_ptr<
    ChannelClientConnection>& client, std::set<uint32_t> updates)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto materials = character->GetMaterials();

    libcomp::Packet p;
    if(updates.size() == 0)
    {
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MATERIAL_BOX);

        // All materials have a dissasembly entry
        auto disassemblyTypes = mServer.lock()->GetDefinitionManager()
            ->GetDisassembledItemIDs();

        int32_t materialCount = (int32_t)disassemblyTypes.size();
        p.WriteS32Little(materialCount);
        for(uint32_t materialType : disassemblyTypes)
        {
            auto it = materials.find(materialType);

            p.WriteU32Little(materialType);
            p.WriteS32Little(it != materials.end() ? it->second : 0);
        }
    }
    else
    {
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_MATERIAL_BOX_UPDATED);

        p.WriteS32Little((int32_t)updates.size());
        for(uint32_t materialType : updates)
        {
            auto it = materials.find(materialType);

            p.WriteU32Little(materialType);
            p.WriteS32Little(it != materials.end() ? it->second : 0);
        }
    }

    client->SendPacket(p);
}

void CharacterManager::SendDevilBook(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto devilBook = character->GetProgress()->GetDevilBook();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_DEMON_COMPENDIUM);
    reply.WriteS8(0);   // Unknown
    reply.WriteU16Little((uint16_t)devilBook.size());
    reply.WriteArray(&devilBook, (uint32_t)devilBook.size());

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

    libobjgen::UUID accountUID;
    if(cState)
    {
        accountUID = cState->GetEntity()->GetAccount();
    }
    else
    {
        auto box = std::dynamic_pointer_cast<objects::DemonBox>(
            libcomp::PersistentObject::GetObjectByUUID(
                dState->GetEntity()->GetDemonBox()));
        if(box)
        {
            accountUID = box->GetAccount();
        }
    }

    auto changes = libcomp::DatabaseChangeSet::Create(accountUID);

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
            auto effectIter = effectMap.find(effectType);
            if(effectIter != effectMap.end() &&
                effectIter->second != p.Get())
            {
                // Delete old, insert new
                changes->Delete(p.Get());
                effectStates[effectType] = true;
            }
            else
            {
                // Update
                effectStates[effectType] = false;
            }
        }
    }

    std::set<uint32_t> removed;
    std::list<libcomp::ObjectReference<objects::StatusEffect>> updated;
    for(auto ePair : effectStates)
    {
        auto effectType = ePair.first;
        auto effect = effectMap[effectType];

        // If the effect can expire but somehow was not removed yet, remove it
        bool expire = !effect->GetIsConstant() && effect->GetExpiration() == 0;
        
        // Do not save constant effects
        if(!effect->GetIsConstant())
        {
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

    auto db = mServer.lock()->GetWorldDatabase();
    if(queueSave)
    {
        return db->QueueChangeSet(changes);
    }
    else
    {
        return db->ProcessChangeSet(changes);
    }
}

bool CharacterManager::AddStatusEffectImmediate(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<ActiveEntityState>& eState,
    const StatusEffectChanges& effects)
{
    auto server = mServer.lock();

    auto removes = eState->AddStatusEffects(effects,
        server->GetDefinitionManager(), 0, false);

    auto allEffects = eState->GetStatusEffects();

    std::list<std::shared_ptr<objects::StatusEffect>> added;
    for(auto& addEffect : effects)
    {
        auto it = allEffects.find(addEffect.first);
        if(it != allEffects.end())
        {
            added.push_back(it->second);
        }
    }

    if(added.size() > 0)
    {
        libcomp::Packet p;
        if(GetActiveStatusesPacket(p, eState->GetEntityID(), added))
        {
            server->GetZoneManager()->BroadcastPacket(client, p);
        }
    }

    if(removes.size() > 0)
    {
        libcomp::Packet p;
        if(GetRemovedStatusesPacket(p, eState->GetEntityID(), removes))
        {
            server->GetZoneManager()->BroadcastPacket(client, p);
        }
    }

    return added.size() > 0;
}

void CharacterManager::CancelStatusEffects(const std::shared_ptr<
    ChannelClientConnection>& client, uint8_t cancelFlags)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto zone = state->GetZone();

    if(!zone)
    {
        return;
    }

    int32_t cEntityID = cState->GetEntityID();
    int32_t dEntityID = dState->GetEntityID();

    std::unordered_map<int32_t, std::set<uint32_t>> cancelMap;
    cancelMap[cEntityID] = cState->CancelStatusEffects(cancelFlags);
    cancelMap[dEntityID] = dState->CancelStatusEffects(cancelFlags);

    if((cancelFlags & EFFECT_CANCEL_ZONEOUT) != 0)
    {
        // Cancel invalid ride effects
        if(zone->GetDefinition()->GetMountDisabled() && CancelMount(state))
        {
            // Cancel all mount status effects
            cancelMap[cEntityID].insert(SVR_CONST.STATUS_MOUNT);
            cancelMap[cEntityID].insert(SVR_CONST.STATUS_MOUNT_SUPER);
            cancelMap[dEntityID].insert(SVR_CONST.STATUS_MOUNT);
            cancelMap[dEntityID].insert(SVR_CONST.STATUS_MOUNT_SUPER);
        }

        if(zone->GetDefinition()->GetBikeDisabled())
        {
            std::set<uint32_t> rideEffects;
            rideEffects.insert(SVR_CONST.STATUS_BIKE);

            bool cancelled = false;
            for(uint32_t effectType : cState->ExpireStatusEffects(rideEffects))
            {
                cancelMap[cEntityID].insert(effectType);
                cancelled = true;
            }

            if(cancelled)
            {
                RecalculateTokuseiAndStats(cState, client);
            }
        }
    }

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

    if(cancelMap.size() > 0)
    {
        auto zoneManager = mServer.lock()->GetZoneManager();

        for(auto& pair : cancelMap)
        {
            if(pair.second.size() > 0)
            {
                libcomp::Packet p;
                if(GetRemovedStatusesPacket(p, pair.first, pair.second))
                {
                    if(zone)
                    {
                        zoneManager->BroadcastPacket(zone, p);
                    }
                    else
                    {
                        client->QueuePacket(p);
                    }
                }
            }
        }

        client->FlushOutgoing();
    }
}

bool CharacterManager::CancelMount(ClientState* state)
{
    if(state)
    {
        auto server = mServer.lock();
        auto definitionManager = server->GetDefinitionManager();

        auto cState = state->GetCharacterState();
        auto dState = state->GetDemonState();

        if(!cState->IsMounted() && !dState->IsMounted())
        {
            return false;
        }

        for(uint32_t skillID : definitionManager->GetFunctionIDSkills(
            SVR_CONST.SKILL_MOUNT))
        {
            cState->RemoveActiveSwitchSkills(skillID);
            dState->RemoveActiveSwitchSkills(skillID);
        }

        server->GetTokuseiManager()->Recalculate(cState, true);

        // In case the entities have the status from somewhere else,
        // double check that the status is removed
        const std::set<uint32_t> mountEffects =
            {
                SVR_CONST.STATUS_MOUNT,
                SVR_CONST.STATUS_MOUNT_SUPER
            };

        cState->ExpireStatusEffects(mountEffects);
        dState->ExpireStatusEffects(mountEffects);

        return true;
    }

    return false;
}

bool CharacterManager::GetActiveStatusesPacket(libcomp::Packet& p, int32_t entityID,
    const std::list<std::shared_ptr<objects::StatusEffect>>& active)
{
    if(active.size() == 0)
    {
        return false;
    }

    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ADD_STATUS_EFFECT);
    p.WriteS32Little(entityID);
    p.WriteU32Little((uint32_t)active.size());

    for(auto& effect : active)
    {
        p.WriteU32Little(effect->GetEffect());
        p.WriteS32Little((int32_t)effect->GetExpiration());
        p.WriteU8(effect->GetStack());
    }

    return true;
}

bool CharacterManager::GetRemovedStatusesPacket(libcomp::Packet& p,
    int32_t entityID, const std::set<uint32_t>& removed)
{
    if(removed.size() == 0)
    {
        return false;
    }

    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_STATUS_EFFECT);
    p.WriteS32Little(entityID);
    p.WriteU32Little((uint32_t)removed.size());
    for(uint32_t effectType : removed)
    {
        p.WriteU32Little(effectType);
    }

    return true;
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
                if(!e2->Ready()) continue;

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
            p.WriteFloat(entity->GetMovementSpeed());
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
            p.WriteFloat(entity->GetMovementSpeed());
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
            if(entity->GetEntityType() == EntityType_t::PARTNER_DEMON)
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

    uint8_t growthType = 0;
    if(demon)
    {
        ds = demon->GetCoreStats().Get();
        demonData = definitionManager->GetDevilData(demon->GetType());
        growthType = demon->GetGrowthType();
    }

    auto basicData = demonData->GetBasic();
    auto battleData = demonData->GetBattleData();

    if(!growthType)
    {
        growthType = demonData->GetGrowth()->GetGrowthType();
    }

    auto baseLevelRate = definitionManager->GetDevilLVUpRateData(growthType);

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
        // Familiarity boost is applied from the base growth type, not the
        // current growth type
        auto famLevelRate = definitionManager->GetDevilLVUpRateData(
            demonData->GetGrowth()->GetGrowthType());
        int8_t familiarityRank = GetFamiliarityRank(demon->GetFamiliarity());
        if(familiarityRank < 0)
        {
            // Ranks below zero have boost data 0-2 subtracted
            for(int8_t i = familiarityRank; i < 0; i++)
            {
                size_t famBoost = (size_t)(abs(i) - 1);
                BoostStats(stats, famLevelRate->GetLevelUpData(famBoost), -1);
            }
        }
        else if(familiarityRank > 0)
        {
            // Ranks above zero have boost data 0-3 added
            for(int8_t i = 0; i < familiarityRank; i++)
            {
                size_t famBoost = (size_t)i;
                BoostStats(stats, famLevelRate->GetLevelUpData(famBoost), 1);
            }
        }

        AdjustDemonBaseStats(demon, stats, true);
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

    AdjustStatBounds(stats, demon != nullptr);

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

void CharacterManager::AdjustDemonBaseStats(const std::shared_ptr<
    objects::Demon>& demon, libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    bool baseCalc)
{
    if(!demon)
    {
        return;
    }
    else if(baseCalc)
    {
        // Reset base values initialized here
        demon->SetMagReduction(0);
    }

    // Apply reunion boosts
    auto reunionBonuses = demon->GetReunion();
    for(size_t i = 0; i < 12; i++)
    {
        int8_t reunionRank = reunionBonuses[i];
        if(reunionRank > 1)
        {
            int8_t rBoost = (int8_t)(reunionRank - 1);
            switch(i)
            {
            case 0:     // Tiwaz (テイワズ)
                stats[CorrectTbl::CLSR] = (int16_t)(
                    stats[CorrectTbl::CLSR] + 4 * rBoost);
                break;
            case 1:     // Peorth (ペオース)
                stats[CorrectTbl::SPELL] = (int16_t)(
                    stats[CorrectTbl::SPELL] + 4 * rBoost);
                break;
            case 2:     // Eoh (エオー)
                stats[CorrectTbl::LNGR] = (int16_t)(
                    stats[CorrectTbl::LNGR] + 4 * rBoost);
                break;
            case 3:     // Eihwaz (エイワズ)
                stats[CorrectTbl::SUPPORT] = (int16_t)(
                    stats[CorrectTbl::SUPPORT] + 4 * rBoost);
                break;
            case 4:     // Uruz (ウルズ)
                stats[CorrectTbl::CLSR] = (int16_t)(
                    stats[CorrectTbl::CLSR] + 2 * rBoost);
                stats[CorrectTbl::LNGR] = (int16_t)(
                    stats[CorrectTbl::LNGR] + 2 * rBoost);
                break;
            case 5:     // Hagalaz (ハガラズ)
                stats[CorrectTbl::CLSR] = (int16_t)(
                    stats[CorrectTbl::CLSR] + 2 * rBoost);
                stats[CorrectTbl::SPELL] = (int16_t)(
                    stats[CorrectTbl::SPELL] + 2 * rBoost);
                break;
            case 6:     // Laguz (ラグズ)
                stats[CorrectTbl::LNGR] = (int16_t)(
                    stats[CorrectTbl::LNGR] + 2 * rBoost);
                stats[CorrectTbl::SPELL] = (int16_t)(
                    stats[CorrectTbl::SPELL] + 2 * rBoost);
                break;
            case 7:     // Ansuz (アンサズ)
                stats[CorrectTbl::MDEF] = (int16_t)(
                    stats[CorrectTbl::MDEF] + 5 * rBoost);
                break;
            case 8:     // Nauthiz (ナウシズ)
                stats[CorrectTbl::PDEF] = (int16_t)(
                    stats[CorrectTbl::PDEF] + 5 * rBoost);
                break;
            case 9:     // Ingwaz (イング)
                if(baseCalc)
                {
                    int32_t reduction = (5 * rBoost);

                    // Cap at 100%
                    if(reduction > 100)
                    {
                        reduction = 100;
                    }

                    demon->SetMagReduction((int8_t)reduction);
                }
                break;
            case 10:    // Sigel (シーグル)
                if(baseCalc)
                {
                    stats[CorrectTbl::STR] = (int16_t)(
                        stats[CorrectTbl::STR] + 3 * rBoost);
                    stats[CorrectTbl::MAGIC] = (int16_t)(
                        stats[CorrectTbl::MAGIC] + 3 * rBoost);
                    stats[CorrectTbl::VIT] = (int16_t)(
                        stats[CorrectTbl::VIT] + 3 * rBoost);
                    stats[CorrectTbl::INT] = (int16_t)(
                        stats[CorrectTbl::INT] + 3 * rBoost);
                    stats[CorrectTbl::SPEED] = (int16_t)(
                        stats[CorrectTbl::SPEED] + 3 * rBoost);
                    stats[CorrectTbl::LUCK] = (int16_t)(
                        stats[CorrectTbl::LUCK] + 3 * rBoost);
                }
                break;
            case 11:    // Wyrd (ウィアド)
                stats[CorrectTbl::HP_MAX] = (int16_t)(
                    stats[CorrectTbl::HP_MAX] + 40 * rBoost);
                stats[CorrectTbl::MP_MAX] = (int16_t)(
                    stats[CorrectTbl::MP_MAX] + 10 * rBoost);
                break;
            default:
                break;
            }
        }
    }

    // Add demon force if not performing a base calculation
    if(!baseCalc)
    {
        for(size_t i = 0; i < 20; i++)
        {
            int32_t fVal = demon->GetForceValues(i);

            // Only apply if at least one point has been achieved
            if(fVal >= 100000)
            {
                CorrectTbl tblID = (CorrectTbl)libcomp::DEMON_FORCE_CONVERSION[i];
                stats[tblID] = (int16_t)(stats[tblID] + fVal / 100000);
            }
        }
    }
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
    stats[CorrectTbl::MOVE1] = (int16_t)(STAT_DEFAULT_SPEED / 2);
    stats[CorrectTbl::MOVE2] = STAT_DEFAULT_SPEED;
    stats[CorrectTbl::SUMMON_SPEED] = 100;
    stats[CorrectTbl::KNOCKBACK_RESIST] = 100;
    stats[CorrectTbl::COOLDOWN_TIME] = 100;
    stats[CorrectTbl::LB_DAMAGE] = 100;
    stats[CorrectTbl::CHANT_TIME] = 100;

    // Default all the rates to 100%
    for(uint8_t i = (uint8_t)CorrectTbl::RATE_XP;
        i <= (uint8_t)CorrectTbl::RATE_HEAL_TAKEN; i++)
    {
        stats[(CorrectTbl)i] = 100;
    }

    stats[CorrectTbl::RATE_PC] = 100;
    stats[CorrectTbl::RATE_DEMON] = 100;
    stats[CorrectTbl::RATE_PC_TAKEN] = 100;
    stats[CorrectTbl::RATE_DEMON_TAKEN] = 100;

    return stats;
}

void CharacterManager::CalculateDependentStats(
    libcomp::EnumMap<CorrectTbl, int16_t>& stats, int8_t level, bool isDemon)
{
    /// @todo: fix: close but not quite right
    libcomp::EnumMap<CorrectTbl, int16_t> adjusted;
    if(isDemon)
    {
        // Round up each part
        adjusted[CorrectTbl::HP_MAX] = (int16_t)(stats[CorrectTbl::HP_MAX] +
            (int16_t)ceill(stats[CorrectTbl::HP_MAX] * 0.03 * level) +
            (int16_t)ceill(stats[CorrectTbl::STR] * 0.3) +
            (int16_t)ceill(((stats[CorrectTbl::HP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::VIT]));
        adjusted[CorrectTbl::MP_MAX] = (int16_t)(stats[CorrectTbl::MP_MAX] +
            (int16_t)ceill(stats[CorrectTbl::MP_MAX] * 0.03 * level) +
            (int16_t)ceill(stats[CorrectTbl::MAGIC] * 0.3) +
            (int16_t)ceill(((stats[CorrectTbl::MP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::INT]));

        // Round the result, adjusting by 0.5
        adjusted[CorrectTbl::CLSR] = (int16_t)(stats[CorrectTbl::CLSR] +
            (int16_t)roundl((stats[CorrectTbl::STR] * 0.5) + 0.5 + (level * 0.1)));
        adjusted[CorrectTbl::LNGR] = (int16_t)(stats[CorrectTbl::LNGR] +
            (int16_t)roundl((stats[CorrectTbl::SPEED] * 0.5) + 0.5 + (level * 0.1)));
        adjusted[CorrectTbl::SPELL] = (int16_t)(stats[CorrectTbl::SPELL] +
            (int16_t)roundl((stats[CorrectTbl::MAGIC] * 0.5) + 0.5 + (level * 0.1)));
        adjusted[CorrectTbl::SUPPORT] = (int16_t)(stats[CorrectTbl::SUPPORT] +
            (int16_t)roundl((stats[CorrectTbl::INT] * 0.5) + 0.5 + (level * 0.1)));
        adjusted[CorrectTbl::PDEF] = (int16_t)(stats[CorrectTbl::PDEF] +
            (int16_t)roundl((stats[CorrectTbl::VIT] * 0.1) + 0.5 + (level * 0.1)));
        adjusted[CorrectTbl::MDEF] = (int16_t)(stats[CorrectTbl::MDEF] +
            (int16_t)roundl((stats[CorrectTbl::INT] * 0.1) + 0.5 + (level * 0.1)));
    }
    else
    {
        // Round each part
        adjusted[CorrectTbl::HP_MAX] = (int16_t)(stats[CorrectTbl::HP_MAX] +
            (int16_t)roundl(stats[CorrectTbl::HP_MAX] * 0.03 * level) +
            (int16_t)roundl(stats[CorrectTbl::STR] * 0.3) +
            (int16_t)roundl(((stats[CorrectTbl::HP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::VIT]));
        adjusted[CorrectTbl::MP_MAX] = (int16_t)(stats[CorrectTbl::MP_MAX] +
            (int16_t)roundl(stats[CorrectTbl::MP_MAX] * 0.03 * level) +
            (int16_t)roundl(stats[CorrectTbl::MAGIC] * 0.3) +
            (int16_t)roundl(((stats[CorrectTbl::MP_MAX] * 0.01) + 0.5) * stats[CorrectTbl::INT]));

        // Round the results down
        adjusted[CorrectTbl::CLSR] = (int16_t)(stats[CorrectTbl::CLSR] +
            (int16_t)floorl((stats[CorrectTbl::STR] * 0.5) + (level * 0.1)));
        adjusted[CorrectTbl::LNGR] = (int16_t)(stats[CorrectTbl::LNGR] +
            (int16_t)floorl((stats[CorrectTbl::SPEED] * 0.5) + (level * 0.1)));
        adjusted[CorrectTbl::SPELL] = (int16_t)(stats[CorrectTbl::SPELL] +
            (int16_t)floorl((stats[CorrectTbl::MAGIC] * 0.5) + (level * 0.1)));
        adjusted[CorrectTbl::SUPPORT] = (int16_t)(stats[CorrectTbl::SUPPORT] +
            (int16_t)floorl((stats[CorrectTbl::INT] * 0.5) + (level * 0.1)));
        adjusted[CorrectTbl::PDEF] = (int16_t)(stats[CorrectTbl::PDEF] +
            (int16_t)floorl((stats[CorrectTbl::VIT] * 0.1) + (level * 0.1)));
        adjusted[CorrectTbl::MDEF] = (int16_t)(stats[CorrectTbl::MDEF] +
            (int16_t)floorl((stats[CorrectTbl::INT] * 0.1) + (level * 0.1)));
    }

    // Calculate HP/MP regen
    adjusted[CorrectTbl::HP_REGEN] = (int16_t)(stats[CorrectTbl::HP_REGEN] +
        (int16_t)floor(((stats[CorrectTbl::VIT] * 3) + stats[CorrectTbl::HP_MAX]) * 0.01));
    adjusted[CorrectTbl::MP_REGEN] = (int16_t)(stats[CorrectTbl::MP_REGEN] +
        (int16_t)floor(((stats[CorrectTbl::INT] * 3) + stats[CorrectTbl::MP_MAX]) * 0.01));

    for(auto pair : adjusted)
    {
        // Since any negative value used for a calculation here is not valid, any result
        // in a negative value should be treated as an overflow and be set to max
        if(pair.second < 0)
        {
            stats[pair.first] = std::numeric_limits<int16_t>::max();
        }
        else
        {
            stats[pair.first] = pair.second;
        }
    }

    // Calculate incant/cooldown time decrease adjustments
    int32_t chantAdjust = (int32_t)(stats[CorrectTbl::CHANT_TIME] -
        (int16_t)floor(2.5 * floor(stats[CorrectTbl::INT] * 0.1) +
            1.5 * floor(stats[CorrectTbl::SPEED] * 0.1)));
    int32_t coolAdjust = (int32_t)(stats[CorrectTbl::COOLDOWN_TIME] -
        (int16_t)floor(2.5 * floor(stats[CorrectTbl::VIT] * 0.1) +
            1.5 * floor(stats[CorrectTbl::SPEED] * 0.1)));
    stats[CorrectTbl::CHANT_TIME] = (int16_t)(chantAdjust < 0 ? 0 : chantAdjust);
    stats[CorrectTbl::COOLDOWN_TIME] = (int16_t)(coolAdjust < 5 ? 5 : coolAdjust);
}

void CharacterManager::AdjustStatBounds(libcomp::EnumMap<CorrectTbl, int16_t>& stats,
    bool limitMax)
{
    static libcomp::EnumMap<CorrectTbl, int16_t> minStats =
        {
            { CorrectTbl::HP_MAX, 1 },
            { CorrectTbl::MP_MAX, 1 },
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
            { CorrectTbl::MP_REGEN, 0 },
            { CorrectTbl::COOLDOWN_TIME, 5 },
            { CorrectTbl::CHANT_TIME, 0 }
        };
    
    static libcomp::EnumMap<CorrectTbl, int16_t> maxStats =
        {
            { CorrectTbl::HP_MAX, MAX_PLAYER_HP_MP },
            { CorrectTbl::MP_MAX, MAX_PLAYER_HP_MP },
            { CorrectTbl::STR, 999 },
            { CorrectTbl::MAGIC, 999 },
            { CorrectTbl::VIT, 999 },
            { CorrectTbl::INT, 999 },
            { CorrectTbl::SPEED, 999 },
            { CorrectTbl::LUCK, 999 }
        };

    for(auto& pair : minStats)
    {
        auto it = stats.find(pair.first);
        if(it != stats.end() && it->second < pair.second)
        {
            stats[pair.first] = pair.second;
        }
    }

    if(limitMax)
    {
        for(auto& pair : maxStats)
        {
            auto it = stats.find(pair.first);
            if(it != stats.end() && it->second > pair.second)
            {
                stats[pair.first] = pair.second;
            }
        }
    }
}

uint32_t CharacterManager::GetDemonPresent(uint32_t demonType, int8_t level,
    uint16_t familiarity, int8_t& rarity) const
{
    // Presents are only given for the top 2 ranks
    if(GetFamiliarityRank(familiarity) < 3)
    {
        return 0;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto serverDataManager = server->GetServerDataManager();

    auto demonDef = definitionManager->GetDevilData(demonType);
    uint32_t baseType = demonDef
        ? demonDef->GetUnionData()->GetBaseDemonID() : 0;

    auto baseDef = baseType
        ? definitionManager->GetDevilData(baseType) : nullptr;

    auto presentDef = baseType
        ? serverDataManager->GetDemonPresentData(baseType) : nullptr;
    if(baseDef && presentDef)
    {
        // Attempt to pull presents from rares then uncommons, then commons
        std::array<std::list<uint32_t>, 3> presents;
        presents[0] = presentDef->GetRareItems();
        presents[1] = presentDef->GetUncommonItems();
        presents[2] = presentDef->GetCommonItems();

        uint8_t baseLevel = baseDef->GetGrowth()->GetBaseLevel();

        // Rates for uncommons and rares start at 0% at base level and increase
        // to a maximum of 25% and 15% respectively up to max level
        double rng = 0.0;
        double rateSum = 0.0;
        for(size_t i = 0; i < 3; i++)
        {
            bool useSet = false;
            if(i == 2)
            {
                // If we get to the common set, use by default
                useSet = true;
            }
            else if(level - baseLevel > 0)
            {
                uint8_t minLevel = 0;
                double maxRate = 0.0;
                if(i == 0)
                {
                    // Rare set
                    minLevel = (uint8_t)(baseLevel + ceil((100.0 -
                        (double)baseLevel) / 5.0));
                    maxRate = 15.0;
                }
                else
                {
                    // Uncommon set
                    minLevel = (uint8_t)(baseLevel + ceil((100.0 -
                        (double)baseLevel) / 10.0));
                    maxRate = 25.0;
                }

                if(minLevel <= (uint8_t)level)
                {
                    double rate = (((double)(level - minLevel) + 1.0) /
                        (100.0 - (double)minLevel) * maxRate) + rateSum;

                    if(rate > 0.0)
                    {
                        if(rng == 0.0)
                        {
                            rng = (double)RNG(uint16_t, 1, 10000) * 0.01;
                        }

                        if(rng <= rate)
                        {
                            useSet = true;
                        }
                    }

                    // Don't run RNG multiple times (not getting a rare
                    // technically increases your odds of getting an uncommon)
                    rateSum += rate;
                }
            }

            // Use an even distribution between all items in the same set
            if(useSet && presents[i].size() > 0)
            {
                rarity = (int8_t)(2 - i);
                if(presents[i].size() == 1)
                {
                    return presents[i].front();
                }
                else
                {
                    return libcomp::Randomizer::GetEntry(presents[i]);
                }
            }
        }
    }

    return 0;
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
        p.WriteS16Little((int16_t)cs->GetMaxHP());
        p.WriteS16Little((int16_t)cs->GetMaxMP());
        p.WriteS16Little((int16_t)cs->GetHP());
        p.WriteS16Little((int16_t)cs->GetMP());
        p.WriteS8(cs->GetLevel());
        p.WriteU8(demon->GetLocked() ? 1 : 0);

        size_t statusEffectCount = demon->StatusEffectsCount();
        p.WriteS32Little(static_cast<int32_t>(statusEffectCount));
        for(auto effect : demon->GetStatusEffects())
        {
            p.WriteU32Little(effect->GetEffect());
        }

        p.WriteS8(0);   //Unknown

        bool equipped = false;
        for(auto equip : demon->GetEquippedItems())
        {
            if(!equip.IsNull())
            {
                equipped = true;
                break;
            }
        }

        p.WriteS8(equipped ? 1 : 0);

        //Effect length in seconds
        p.WriteS32Little(0);
    }
}

void CharacterManager::GetItemDetailPacketData(libcomp::Packet& p,
    const std::shared_ptr<objects::Item>& item, uint8_t detailLevel)
{
    if(item)
    {   
        if(detailLevel >= 1)
        {
            if(detailLevel >= 2)
            {
                p.WriteU32Little(item->GetType());
                p.WriteU16Little(item->GetStackSize());
            }

            p.WriteU16Little(item->GetDurability());
            p.WriteS8(item->GetMaxDurability());
        }

        p.WriteS16Little(item->GetTarot());
        p.WriteS16Little(item->GetSoul());

        for(auto modSlot : item->GetModSlots())
        {
            p.WriteU16Little(modSlot);
        }

        if(detailLevel >= 1)
        {
            p.WriteS32Little((int32_t)item->GetRentalExpiration());
        }

        auto basicEffect = item->GetBasicEffect();
        p.WriteU32Little(basicEffect ? basicEffect
            : static_cast<uint32_t>(-1));

        auto specialEffect = item->GetSpecialEffect();
        p.WriteU32Little(specialEffect ? specialEffect
            : static_cast<uint32_t>(-1));

        for(auto bonus : item->GetFuseBonuses())
        {
            p.WriteS8(bonus);
        }
    }
    else
    {
        switch(detailLevel)
        {
        case 2:
            p.WriteBlank(27);
            break;
        case 1:
            p.WriteBlank(21);
            break;
        case 0:
        default:
            p.WriteBlank(14);
            break;
        }

        p.WriteU32Little(static_cast<uint32_t>(-1));
        p.WriteU32Little(static_cast<uint32_t>(-1));
        p.WriteBlank(3);
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
            p.WriteS16Little((int16_t)state->GetMaxHP());
            p.WriteS16Little((int16_t)state->GetMaxMP());
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
    auto box = std::dynamic_pointer_cast<objects::DemonBox>(
        libcomp::PersistentObject::GetObjectByUUID(demon->GetDemonBox()));
    if(box && box->GetDemons((size_t)demon->GetBoxSlot()).Get() == demon)
    {
        box->SetDemons((size_t)demon->GetBoxSlot(), NULLUUID);
        changes->Update(box);

        if(demon->GetHasQuest())
        {
            // End the demon quest if it belongs to the demon
            auto account = std::dynamic_pointer_cast<objects::Account>(
                libcomp::PersistentObject::GetObjectByUUID(box->GetAccount()));
            auto character = std::dynamic_pointer_cast<objects::Character>(
                libcomp::PersistentObject::GetObjectByUUID(box->GetCharacter()));
            auto dQuest = character ? character->GetDemonQuest().Get() : nullptr;
            if(account && dQuest && dQuest->GetDemon() == demon->GetUUID())
            {
                auto server = mServer.lock();
                auto client = server->GetManagerConnection()
                    ->GetClientConnection(account->GetUsername());
                if(client)
                {
                    server->GetEventManager()->EndDemonQuest(client);
                }
            }
        }
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
