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
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_CHARACTER_DATA);

    reply.WriteU32Little((uint32_t)c->GetCID());    /// @todo: replace with unique entity ID
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
    reply.WriteU8(0x01); // Unknown

    for(size_t i = 0; i < 15; i++)
    {
        uint32_t equip = c->GetEquippedItems(i);

        if(equip != 0)
        {
            reply.WriteU32Little(equip);
        }
        else
        {
            reply.WriteU32Little(0xFFFFFFFF);
        }
    }

    //Character status
    reply.WriteU16Little(cs->GetMaxHP());
    reply.WriteU16Little(cs->GetMaxMP());
    reply.WriteU16Little(cs->GetHP());
    reply.WriteU16Little(cs->GetMP());
    reply.WriteU64Little(cs->GetXP());
    reply.WriteU32Little(c->GetPoints());
    reply.WriteU8(cs->GetLevel());
    reply.WriteS16Little(c->GetLNC());
    reply.WriteU16Little(cs->GetSTR());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSTR() - cs->GetSTR()));
    reply.WriteU16Little(cs->GetMAGIC());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetMAGIC() - cs->GetMAGIC()));
    reply.WriteU16Little(cs->GetVIT());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetVIT() - cs->GetVIT()));
    reply.WriteU16Little(cs->GetINTEL());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetINTEL() - cs->GetINTEL()));
    reply.WriteU16Little(cs->GetSPEED());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSPEED() - cs->GetSPEED()));
    reply.WriteU16Little(cs->GetLUCK());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetLUCK() - cs->GetLUCK()));
    reply.WriteU16Little(cs->GetCLSR());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetCLSR() - cs->GetCLSR()));
    reply.WriteU16Little(cs->GetLNGR());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetLNGR() - cs->GetLNGR()));
    reply.WriteU16Little(cs->GetSPELL());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSPELL() - cs->GetSPELL()));
    reply.WriteU16Little(cs->GetSUPPORT());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSUPPORT() - cs->GetSUPPORT()));
    reply.WriteU16Little(cs->GetPDEF());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetPDEF() - cs->GetPDEF()));
    reply.WriteU16Little(cs->GetMDEF());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetMDEF() - cs->GetMDEF()));

    reply.WriteU32Little(367061536); // Unknown

    /// @todo: status effects
    uint32_t statusEffectCount = 0;

    reply.WriteU32Little(statusEffectCount + 1);

    for(uint32_t i = 0; i < statusEffectCount; i++)
    {
        /*BfStatusEffectDataPtr sfx = cs->statusEffect(i);

        reply.WriteU32Little(sfx->effect());
        reply.WriteFloat(state->clientTime(state->serverTicks() +
            sfx->duration()));
        reply.WriteU8(sfx->stack());*/
    }

    reply.WriteU32Little(1055); //Unknown
    reply.WriteU32Little(1325025608);   //Unknown
    reply.WriteU8(1);   //Unknown

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

        reply.WriteU32Little(0);
        reply.WriteU8(0);
        reply.WriteU8(0); // 0 - Raise | 1 - Capped
    }

    reply.WriteU32Little(0);

    /// @todo: demons
    /*if(nullptr != state->demon() && c->activeDemon() > 0)
        reply.WriteU64Little(c->activeDemon());
    else*/
        reply.WriteS64Little(-1);

    reply.WriteU32Little(0xFFFFFFFF);
    reply.WriteU32Little(0xFFFFFFFF);
    reply.WriteU32Little(0xFFFFFFFF);
    reply.WriteU32Little(0xFFFFFFFF);

    //BfZonePtr zone = state->zoneInst()->zone();

    /// @todo: zone position
    reply.WriteU32Little(1);    //set
    reply.WriteU32Little(0x00004E85); // Zone ID
    reply.WriteFloat(0);    //x
    reply.WriteFloat(0);    //y
    reply.WriteFloat(0);    //rotation

    reply.WriteU8(0);
    reply.WriteU32Little(0); // Homepoint zone
    reply.WriteU32Little(0x43FA8000); // Homepoint X
    reply.WriteU32Little(0x3F800000); // Homepoint Y
    reply.WriteU16Little(0);
    reply.WriteU8(1);

    client->SendPacket(reply);

    ShowCharacter(client);
}

void CharacterManager::ShowCharacter(const std::shared_ptr<channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto c = cState->GetCharacter().Get();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_SHOW_CHARACTER);
    reply.WriteU32Little((uint32_t)c->GetCID());

    client->SendPacket(reply);
}

void CharacterManager::SendStatusIcon(const std::shared_ptr<channel::ChannelClientConnection>& client)
{
    /// @todo: implement icons
    uint8_t icon = 0;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_STATUS_ICON);
    reply.WriteU8(0);
    reply.WriteU8(icon);

    client->SendPacket(reply);

    /// @todo: broadcast to other players
}
