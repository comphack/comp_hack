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
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <Character.h>
#include <Demon.h>
#include <EntityStats.h>
#include <Expertise.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <StatusEffect.h>

using namespace channel;

CharacterManager::CharacterManager()
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
    auto c = cState->GetCharacter().Get();
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
    reply.WriteU8(c->GetGender() == objects::Character::Gender_t::MALE
        ? 0x03 : 0x65); // One of these is wrong
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
            reply.WriteU32Little(0xFFFFFFFF);
        }
    }

    //Character status
    reply.WriteS16Little(cs->GetMaxHP());
    reply.WriteS16Little(cs->GetMaxMP());
    reply.WriteS16Little(cs->GetHP());
    reply.WriteS16Little(cs->GetMP());
    reply.WriteS64Little(cs->GetXP());
    reply.WriteS32Little(c->GetPoints());
    reply.WriteS8(cs->GetLevel());
    reply.WriteS16Little(c->GetLNC());
    reply.WriteS16Little(cs->GetSTR());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSTR() - cs->GetSTR()));
    reply.WriteS16Little(cs->GetMAGIC());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetMAGIC() - cs->GetMAGIC()));
    reply.WriteS16Little(cs->GetVIT());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetVIT() - cs->GetVIT()));
    reply.WriteS16Little(cs->GetINTEL());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetINTEL() - cs->GetINTEL()));
    reply.WriteS16Little(cs->GetSPEED());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSPEED() - cs->GetSPEED()));
    reply.WriteS16Little(cs->GetLUCK());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetLUCK() - cs->GetLUCK()));
    reply.WriteS16Little(cs->GetCLSR());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetCLSR() - cs->GetCLSR()));
    reply.WriteS16Little(cs->GetLNGR());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetLNGR() - cs->GetLNGR()));
    reply.WriteS16Little(cs->GetSPELL());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSPELL() - cs->GetSPELL()));
    reply.WriteS16Little(cs->GetSUPPORT());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetSUPPORT() - cs->GetSUPPORT()));
    reply.WriteS16Little(cs->GetPDEF());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetPDEF() - cs->GetPDEF()));
    reply.WriteS16Little(cs->GetMDEF());
    reply.WriteS16Little(static_cast<int16_t>(
        cState->GetMDEF() - cs->GetMDEF()));

    reply.WriteS16(0); // Unknown
    reply.WriteS16(0); // Unknown

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
    reply.WriteU32(static_cast<int32_t>(skillCount));
    for(auto skill : c->GetLearnedSkills())
    {
        reply.WriteU32Little(skill);
    }

    for(int i = 0; i < 38; i++)
    {
        auto expertise = c->GetExpertises(i);

        if(expertise.IsNull()) continue;

        reply.WriteS32Little(expertise->GetPoints());
        reply.WriteS8(0);   // Unknown
        reply.WriteU8(expertise->GetCapped() ? 1 : 0);
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

    //BfZonePtr zone = state->zoneInst()->zone();

    /// @todo: zone position
    reply.WriteS32Little(1);    //set
    reply.WriteS32Little(0); // Zone UID
    reply.WriteFloat(cState->GetDestinationX());
    reply.WriteFloat(cState->GetDestinationY());
    reply.WriteFloat(cState->GetDestinationRotation());

    reply.WriteU8(0);   //Unknown bool
    reply.WriteS32Little(0); // Homepoint zone
    reply.WriteFloat(0); // Homepoint X
    reply.WriteFloat(0); // Homepoint Y
    reply.WriteS8(0);
    reply.WriteS8(0);
    reply.WriteS8(1);

    size_t ukCount = 0;
    reply.WriteS32(static_cast<int32_t>(ukCount)); // some count
    for(size_t i = 0; i < ukCount; i++)
    {
        reply.WriteS8(0);   //Unknown
        reply.WriteU32Little(0);    //Unknown
    }

    client->SendPacket(reply);

    ShowCharacter(client);
}

void CharacterManager::ShowCharacter(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto c = cState->GetCharacter().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_CHARACTER);
    reply.WriteS32Little(cState->GetEntityID());

    client->SendPacket(reply);
}

void CharacterManager::SendCOMPDemonData(const std::shared_ptr<
    ChannelClientConnection>& client,
    int8_t box, int8_t slot, int64_t id)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto comp = cState->GetCharacter()->GetCOMP();

    auto d = comp[slot].Get();
    if(d == nullptr || state->GetObjectID(d->GetUUID()) != id)
    {
        return;
    }

    auto cs = d->GetCoreStats().Get();
    bool isSummoned = dState->GetDemon().Get() == d;

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
    reply.WriteS16Little(cs->GetSTR());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetSTR() - cs->GetSTR()));
    reply.WriteS16Little(cs->GetMAGIC());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetMAGIC() - cs->GetMAGIC()));
    reply.WriteS16Little(cs->GetVIT());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetVIT() - cs->GetVIT()));
    reply.WriteS16Little(cs->GetINTEL());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetINTEL() - cs->GetINTEL()));
    reply.WriteS16Little(cs->GetSPEED());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetSPEED() - cs->GetSPEED()));
    reply.WriteS16Little(cs->GetLUCK());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetLUCK() - cs->GetLUCK()));
    reply.WriteS16Little(cs->GetCLSR());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetCLSR() - cs->GetCLSR()));
    reply.WriteS16Little(cs->GetLNGR());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetLNGR() - cs->GetLNGR()));
    reply.WriteS16Little(cs->GetSPELL());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetSPELL() - cs->GetSPELL()));
    reply.WriteS16Little(cs->GetSUPPORT());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetSUPPORT() - cs->GetSUPPORT()));
    reply.WriteS16Little(cs->GetPDEF());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetPDEF() - cs->GetPDEF()));
    reply.WriteS16Little(cs->GetMDEF());
    reply.WriteS16Little(static_cast<int16_t>(
        !isSummoned ? 0 : dState->GetMDEF() - cs->GetMDEF()));

    //Learned skill count will always be the static
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
