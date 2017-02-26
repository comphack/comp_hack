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

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <Character.h>
#include <Demon.h>
#include <EntityStats.h>
#include <Expertise.h>
#include <InheritedSkill.h>
#include <Item.h>
#include <ItemBox.h>
#include <MiAcquisitionSkillData.h>
#include <MiDevilBattleData.h>
#include <MiDevilData.h>
#include <MiDevilLVUpData.h>
#include <MiDevilLVUpRateData.h>
#include <MiGrowthData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiNPCBasicData.h>
#include <MiSkillData.h>
#include <StatusEffect.h>

const uint64_t LevelXPRequirements[] = {
    0ULL, 40ULL, 180ULL, 480ULL, 1100ULL, 2400ULL, 4120ULL, 6220ULL, 9850ULL,   //1-9
    14690ULL, 20080ULL, 25580ULL, 33180ULL, 41830ULL, 50750ULL, 63040ULL, 79130ULL, 99520ULL, 129780ULL,    //10-19
    159920ULL, 189800ULL, 222600ULL, 272800ULL, 354200ULL, 470400ULL, 625000ULL, 821600ULL, 1063800ULL, 1355200ULL, //20-29
    1699400ULL, 840000ULL, 899000ULL, 1024000ULL, 1221000ULL, 1496000ULL, 1855000ULL, 2304000ULL, 2849000ULL, 3496000ULL,   //30-39
    4251000ULL, 2160000ULL, 2255000ULL, 2436000ULL, 2709000ULL, 3080000ULL, 3452000ULL, 4127000ULL, 5072000ULL, 6241000ULL, //40-49
    7640000ULL, 4115000ULL, 4401000ULL, 4803000ULL, 5353000ULL, 6015000ULL, 6892000ULL, 7900000ULL, 9308000ULL, 11220000ULL,    //50-59
    14057000ULL, 8122000ULL, 8538000ULL, 9247000ULL, 10101000ULL, 11203000ULL, 12400000ULL, 14382000ULL, 17194000ULL, 20444000ULL,  //60-69
    25600000ULL, 21400314ULL, 23239696ULL, 24691100ULL, 27213000ULL, 31415926ULL, 37564000ULL, 46490000ULL, 55500000ULL, 66600000ULL,   //70-79
    78783200ULL, 76300000ULL, 78364000ULL, 81310000ULL, 85100000ULL, 89290000ULL, 97400000ULL, 110050000ULL, 162000000ULL, 264000000ULL,    //80-89
    354000000ULL, 696409989ULL, 1392819977ULL, 2089229966ULL, 2100000000ULL, 2110000000ULL, 10477689898ULL, 41910759592ULL, 125732278776ULL, 565795254492ULL,   //90-99
};

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

    dState->RecalculateStats(mServer.lock()->GetDefinitionManager());

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
    reply.WriteS16Little(0x22C7); //Unknown

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

    GetEntityStatsPacketData(reply, character->GetCoreStats().Get(), cState, true);

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
    character->SetCOMP(compSlot, d);

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
    auto character = cState->GetCharacter().Get();
    auto dState = state->GetDemonState();
    auto demon = dState->GetDemon().Get();

    bool isDemon = false;
    std::shared_ptr<objects::EntityStats> stats;
    std::shared_ptr<objects::MiDevilData> demonData;
    if(cState->GetEntityID() == entityID)
    {
        stats = character->GetCoreStats().Get();
    }
    else if(dState->GetEntityID() == entityID && nullptr != demon)
    {
        stats = demon->GetCoreStats().Get();
        isDemon = true;
        demonData = definitionManager->GetDevilData(demon->GetType());
    }
    else
    {
        return;
    }

    auto level = stats->GetLevel();
    if(level == 99)
    {
        return;
    }

    uint64_t xpDelta = stats->GetXP() + xpGain;
    while(level < 99 && xpDelta >= LevelXPRequirements[level])
    {
        xpDelta -= LevelXPRequirements[level];

        level++;

        stats->SetLevel(level);

        libcomp::Packet reply;
        if(isDemon)
        {
            auto growth = demonData->GetGrowth();
            for(auto acSkill : growth->GetAcquisitionSkills())
            {
                if(acSkill->GetLevel() == level)
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

        /// @todo: send to all players in the zone
        client->SendPacket(reply);
    }

    stats->SetXP(xpDelta);

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_XP_UPDATE);
    reply.WriteS32Little(entityID);
    reply.WriteS64(xpDelta);
    reply.WriteS32Little((int32_t)xpGain);
    reply.WriteS32Little(0);    //Unknown

    /// @todo: send to all players in the zone
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
    auto cState = state->GetCharacterState();
    auto dState = state->GetDemonState();

    std::shared_ptr<objects::EntityStats> stats;
    if(cState->GetEntityID() == entityID)
    {
        stats = cState->GetCharacter()->GetCoreStats().Get();
    }
    else if(dState->GetEntityID() == entityID && !dState->GetDemon().IsNull())
    {
        stats = dState->GetDemon()->GetCoreStats().Get();
    }
    else
    {
        return;
    }

    uint64_t xpGain = 0;
    for(int8_t i = stats->GetLevel(); i < level; i++)
    {
        if(xpGain == 0)
        {
            xpGain += LevelXPRequirements[i] - stats->GetXP();
        }
        else
        {
            xpGain += LevelXPRequirements[i];
        }
    }

    ExperienceGain(client, xpGain, entityID);
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
    uint8_t boostLevel = (level + 3) / 4;
    uint8_t boostStage = (boostLevel - 1) / 5;
    
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
        stats[libcomp::CORRECT_MAXHP] += (int16_t)ceill(stats[libcomp::CORRECT_MAXHP] * 0.03 * level) +
            (int16_t)ceill(stats[libcomp::CORRECT_STR] * 0.3) +
            (int16_t)ceill(((stats[libcomp::CORRECT_MAXHP] * 0.01) + 0.5) * stats[libcomp::CORRECT_VIT]);
        stats[libcomp::CORRECT_MAXMP] += (int16_t)ceill(stats[libcomp::CORRECT_MAXMP] * 0.03 * level) +
            (int16_t)ceill(stats[libcomp::CORRECT_MAGIC] * 0.3) +
            (int16_t)ceill(((stats[libcomp::CORRECT_MAXMP] * 0.01) + 0.5) * stats[libcomp::CORRECT_INTEL]);

        // Round the result, adjusting by 0.5
        stats[libcomp::CORRECT_CLSR] += (int16_t)roundl(((stats[libcomp::CORRECT_STR]) * 0.5) + 0.5 + (level * 0.1));
        stats[libcomp::CORRECT_LNGR] += (int16_t)roundl(((stats[libcomp::CORRECT_SPEED]) * 0.5) + 0.5 + (level * 0.1));
        stats[libcomp::CORRECT_SPELL] += (int16_t)roundl(((stats[libcomp::CORRECT_MAGIC]) * 0.5) + 0.5 + (level * 0.1));
        stats[libcomp::CORRECT_SUPPORT] += (int16_t)roundl(((stats[libcomp::CORRECT_INTEL]) * 0.5) + 0.5 + (level * 0.1));
        stats[libcomp::CORRECT_PDEF] += (int16_t)roundl(((stats[libcomp::CORRECT_VIT]) * 0.1) + 0.5 + (level * 0.1));
        stats[libcomp::CORRECT_MDEF] += (int16_t)roundl(((stats[libcomp::CORRECT_INTEL]) * 0.1) + 0.5 + (level * 0.1));
    }
    else
    {
        // Round each part
        stats[libcomp::CORRECT_MAXHP] += (int16_t)roundl(stats[libcomp::CORRECT_MAXHP] * 0.03 * level) +
            (int16_t)roundl(stats[libcomp::CORRECT_STR] * 0.3) +
            (int16_t)roundl(((stats[libcomp::CORRECT_MAXHP] * 0.01) + 0.5) * stats[libcomp::CORRECT_VIT]);
        stats[libcomp::CORRECT_MAXMP] += (int16_t)roundl(stats[libcomp::CORRECT_MAXMP] * 0.03 * level) +
            (int16_t)roundl(stats[libcomp::CORRECT_MAGIC] * 0.3) +
            (int16_t)roundl(((stats[libcomp::CORRECT_MAXMP] * 0.01) + 0.5) * stats[libcomp::CORRECT_INTEL]);

        // Round the results down
        stats[libcomp::CORRECT_CLSR] += (int16_t)floorl(((stats[libcomp::CORRECT_STR]) * 0.5) + (level * 0.1));
        stats[libcomp::CORRECT_LNGR] += (int16_t)floorl(((stats[libcomp::CORRECT_SPEED]) * 0.5) + (level * 0.1));
        stats[libcomp::CORRECT_SPELL] += (int16_t)floorl(((stats[libcomp::CORRECT_MAGIC]) * 0.5) + (level * 0.1));
        stats[libcomp::CORRECT_SUPPORT] += (int16_t)floorl(((stats[libcomp::CORRECT_INTEL]) * 0.5) + (level * 0.1));
        stats[libcomp::CORRECT_PDEF] += (int16_t)floorl(((stats[libcomp::CORRECT_VIT]) * 0.1) + (level * 0.1));
        stats[libcomp::CORRECT_MDEF] += (int16_t)floorl(((stats[libcomp::CORRECT_INTEL]) * 0.1) + (level * 0.1));
    }
}

void CharacterManager::GetCOMPSlotPacketData(libcomp::Packet& p,
    const std::shared_ptr<channel::ChannelClientConnection>& client, size_t slot)
{
    auto state = client->GetClientState();
    auto demon = state->GetCharacterState()->GetCharacter()->GetCOMP(slot).Get();

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
    const std::shared_ptr<objects::EntityStateObject>& state,
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
    stats[libcomp::CORRECT_STR] += data->GetSTR() * boostLevel;
    stats[libcomp::CORRECT_MAGIC] += data->GetMAGIC() * boostLevel;
    stats[libcomp::CORRECT_VIT] += data->GetVIT() * boostLevel;
    stats[libcomp::CORRECT_INTEL] += data->GetINTEL() * boostLevel;
    stats[libcomp::CORRECT_SPEED] += data->GetSPEED() * boostLevel;
    stats[libcomp::CORRECT_LUCK] += data->GetLUCK() * boostLevel;
}
