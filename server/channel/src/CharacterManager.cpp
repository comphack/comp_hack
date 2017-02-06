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
#include <ItemBox.h>
#include <MiItemBasicData.h>
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
            reply.WriteU32Little(static_cast<uint32_t>(-1));
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

    GetEntityStatsPacketData(reply, cs, cState);

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
            reply.WriteBlank(5);
            reply.WriteU8(1);
        }
        else
        {
            reply.WriteS32Little(expertise->GetPoints());
            reply.WriteS8(0);   // Unknown
            reply.WriteU8(expertise->GetCapped() ? 1 : 0);
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

    //BfZonePtr zone = state->zoneInst()->zone();

    /// @todo: zone position
    reply.WriteS32Little(1);    //set
    reply.WriteS32Little(0x00004E85); // Zone ID (not UID)
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

void CharacterManager::ShowEntity(const std::shared_ptr<
    ChannelClientConnection>& client, int32_t entityID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_ENTITY);
    reply.WriteS32Little(entityID);

    client->SendPacket(reply);
}

void CharacterManager::SendPartnerData(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetCharacter();
    auto comp = character->GetCOMP();

    auto d = dState->GetDemon().Get();
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
                character.GetUUID().ToString()));
        return;
    }

    dState->RecalculateStats();

    auto ds = d->GetCoreStats().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_DATA);
    reply.WriteS32Little(dState->GetEntityID());
    reply.WriteS8(slot);
    reply.WriteS64Little(state->GetObjectID(d->GetUUID()));
    reply.WriteU32Little(d->GetType());
    reply.WriteS16Little(ds->GetMaxHP());
    reply.WriteS16Little(ds->GetMaxMP());
    reply.WriteS16Little(ds->GetHP());
    reply.WriteS16Little(ds->GetMP());
    reply.WriteS64Little(ds->GetXP());
    reply.WriteS8(ds->GetLevel());
    reply.WriteS16Little(0x22C7); //Unknown

    GetEntityStatsPacketData(reply, ds, dState);

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

    /// @todo: zone position
    reply.WriteS32Little(1);    //set
    reply.WriteS32Little(0x00004E85); // Zone ID (not UID)
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

void CharacterManager::SendCOMPDemonData(const std::shared_ptr<
    ChannelClientConnection>& client,
    int8_t box, int8_t slot, int64_t id)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto comp = cState->GetCharacter()->GetCOMP();

    auto d = comp[(size_t)slot].Get();
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

    GetEntityStatsPacketData(reply, cs, isSummoned ? dState : nullptr);

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
    auto character = cState->GetCharacter().Get();

    std::shared_ptr<objects::Demon> demon;
    for(auto d : character->GetCOMP())
    {
        if(d.IsNull()) continue;

        int64_t demonObjectID = state->GetObjectID(d.GetUUID());
        if(demonID == demonObjectID)
        {
            demon = d.Get();
            break;
        }
    }

    if(nullptr == demon)
    {
        return;
    }

    character->SetActiveDemon(demon);
    dState->SetDemon(demon);
    dState->SetDestinationX(cState->GetDestinationX());
    dState->SetDestinationY(cState->GetDestinationY());

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_PARTNER_SUMMONED);
    reply.WriteS64Little(demonID);

    client->SendPacket(reply);
}

void CharacterManager::StoreDemon(const std::shared_ptr<
    channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();
    auto character = cState->GetCharacter().Get();

    auto dRef = dState->GetDemon();
    if(dRef.IsNull())
    {
        return;
    }

    dRef.SetReference(nullptr);
    character->SetActiveDemon(dRef);
    dState->SetDemon(dRef);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_REMOVE_OBJECT);
    reply.WriteS32Little(dState->GetEntityID());

    client->SendPacket(reply);
}

void CharacterManager::SendItemBoxData(const std::shared_ptr<
    ChannelClientConnection>& client, int64_t boxID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetCharacter();
    auto box = character->GetItemBoxes((size_t)boxID);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_BOX);
    reply.WriteS8(box->GetType());
    reply.WriteS64(state->GetObjectID(box->GetUUID()));
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

    for(uint16_t i = 0; i < 50; i++)
    {
        auto item = box->GetItems(i);

        if(item.IsNull()) continue;

        reply.WriteU16Little(i);    //Slot
        reply.WriteS64Little(
            state->GetObjectID(item->GetUUID()));
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

void CharacterManager::EquipItem(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int64_t itemID)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetCharacter().Get();
    auto itemBox = character->GetItemBoxes(0).Get();

    std::shared_ptr<objects::Item> equip;
    for(auto item : itemBox->GetItems())
    {
        if(item.IsNull()) continue;

        int64_t itemObjectID = state->GetObjectID(item.GetUUID());
        if(itemID == itemObjectID)
        {
            equip = item.Get();
            break;
        }
    }

    if(nullptr == equip)
    {
        return;
    }

    auto slot = objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_NONE;

    /// @todo: Pull from actual item data
    slot = objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_TOP;

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

    cState->RecalculateStats();

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

    client->SendPacket(reply);
}

void CharacterManager::UpdateLNC(const std::shared_ptr<
    channel::ChannelClientConnection>& client, int16_t lnc)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetCharacter().Get();

    character->SetLNC(lnc);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_LNC_POINTS);
    reply.WriteS32Little(cState->GetEntityID());
    reply.WriteS16Little(character->GetLNC());

    client->SendPacket(reply);
}

void CharacterManager::GetEntityStatsPacketData(libcomp::Packet& p,
    const std::shared_ptr<objects::EntityStats>& coreStats,
    const std::shared_ptr<objects::EntityStateObject>& state)
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
