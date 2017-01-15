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

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_CHARACTER_DATA_RESPONSE);

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

    /// @todo: equipment
    for(int z = 0; z < 15; z++)
    {
        /*uint32_t equip = c->equip(z);

        if(equip != 0)
            reply.WriteU32Little(equip);
        else*/
            reply.WriteU32Little(0xFFFFFFFF);
    }

    //Character status
    reply.WriteU16Little(c->GetMaxHP());
    reply.WriteU16Little(c->GetMaxMP());
    reply.WriteU16Little(c->GetHP());
    reply.WriteU16Little(c->GetMP());
    reply.WriteU64Little(c->GetXP());
    reply.WriteU32Little(c->GetPoints());
    reply.WriteU8(c->GetLevel());
    reply.WriteS16Little(c->GetLNC());
    reply.WriteU16Little(c->GetSTR());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSTR() - c->GetSTR()));
    reply.WriteU16Little(c->GetMAGIC());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetMAGIC() - c->GetMAGIC()));
    reply.WriteU16Little(c->GetVIT());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetVIT() - c->GetVIT()));
    reply.WriteU16Little(c->GetINTEL());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetINTEL() - c->GetINTEL()));
    reply.WriteU16Little(c->GetSPEED());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSPEED() - c->GetSPEED()));
    reply.WriteU16Little(c->GetLUCK());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetLUCK() - c->GetLUCK()));
    reply.WriteU16Little(c->GetCLSR());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetCLSR() - c->GetCLSR()));
    reply.WriteU16Little(c->GetLNGR());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetLNGR() - c->GetLNGR()));
    reply.WriteU16Little(c->GetSPELL());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSPELL() - c->GetSPELL()));
    reply.WriteU16Little(c->GetSUPPORT());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetSUPPORT() - c->GetSUPPORT()));
    reply.WriteU16Little(c->GetPDEF());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetPDEF() - c->GetPDEF()));
    reply.WriteU16Little(c->GetMDEF());
    reply.WriteU16Little(static_cast<uint16_t>(
        cState->GetMDEF() - c->GetMDEF()));

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
}

void CharacterManager::SendStatusIcon(const std::shared_ptr<channel::ChannelClientConnection>& client)
{
    /// @todo: implement icons
    uint8_t icon = 0;

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelClientPacketCode_t::PACKET_STATUS_ICON_RESPONSE);
    reply.WriteU8(0);
    reply.WriteU8(icon);

    client->SendPacket(reply);

    /// @todo: broadcast to other players
}
