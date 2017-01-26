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
#include <PacketCodes.h>

// object Includes
#include <Character.h>
#include <EntityStats.h>
#include <Item.h>

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

    reply.WriteS16(5600); // Unknown
    reply.WriteS16(-5600); // Unknown

    /// @todo: status effects
    uint32_t statusEffectCount = 0;

    reply.WriteU32Little(statusEffectCount + 1);

    for(uint32_t i = 0; i < statusEffectCount; i++)
    {
        /*BfStatusEffectDataPtr sfx = cs->statusEffect(i);

        reply.WriteU32Little(sfx->effect());
        reply.WriteFloat(state->clientTime(state->serverTicks() +
            sfx->duration())); // thinks this is an int32 but i don't believe it
        reply.WriteU8(sfx->stack());*/
    }

    // This is the COMP experience alpha status effect (hence +1)...
    reply.WriteU32Little(1055);
    // Some skills have game time ticks and other have a fixed real time (this is the latter)
    reply.WriteU32Little(1325025608);
    reply.WriteU8(1);

    /// @todo: skills
    reply.WriteU32Little(0);

    /*for(uint32_t skill : state->tempSkillCopy())
        reply.WriteU32Little(skill);
    for(uint32_t skill : c->learnedSkillCopy())
        reply.WriteU32Little(skill);

    reply.WriteArray(defSkills, 4 * defaultCount);*/

    for(int i = 0; i < 38; i++)
    {
        //const BfExpertise& exp = c->expertise(i);

        reply.WriteS32Little(0);    // Points
        reply.WriteS8(0);   // Unknown
        reply.WriteU8(0); // bool: 0 - Raise | 1 - Capped
    }

    reply.WriteU8(0);   // Unknown bool
    reply.WriteU8(0);   // Unknown bool
    reply.WriteU8(0);   // Unknown bool
    reply.WriteU8(0);   // Unknown bool

    /// @todo: demons
    /*if(nullptr != state->demon() && c->activeDemon() > 0)
        reply.WriteU64Little(c->activeDemon());
    else*/
        reply.WriteS64Little(-1);

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
    reply.WriteU32Little(0x43FA8000); // Homepoint X
    reply.WriteU32Little(0x3F800000); // Homepoint Y
    reply.WriteS8(0);
    reply.WriteS8(0);
    reply.WriteS8(1);
    
    reply.WriteS32(0); // some count

    /*
    // count number of these elements
    reply.WriteS8(0);
    reply.WriteU32Little(0);
    */

    client->SendPacket(reply);

    ShowCharacter(client);
}

void CharacterManager::ShowCharacter(const std::shared_ptr<channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto c = cState->GetCharacter().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_SHOW_CHARACTER);
    reply.WriteS32Little(cState->GetEntityID());

    client->SendPacket(reply);
}

void CharacterManager::SendStatusIcon(const std::shared_ptr<channel::ChannelClientConnection>& client)
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
