/**
 * @file server/channel/src/packets/Sync.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to sync with the server time.
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

#include "Packets.h"

// libcomp Includes
#include <Decrypt.h>
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// object Includes
#include <Character.h>

// channel Includes
#include "ChannelClientConnection.h"

using namespace channel;

void SendCharacterData(const std::shared_ptr<channel::ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto c = state->GetCharacter().Get();

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

    /// @todo: status
    //Character status
	reply.WriteU16Little(50);   //Max HP
	reply.WriteU16Little(50);   //Max MP
	reply.WriteU16Little(50);   //HP
	reply.WriteU16Little(50);   //MP
	reply.WriteU64Little(0);    //XP
	reply.WriteU32Little(0);    //Points
	reply.WriteU8(c->GetLevel());
	reply.WriteS16Little(1);    //lnc?
	reply.WriteU16Little(1);    //str
	reply.WriteU16Little(1);    //str2
	reply.WriteU16Little(1);    //magic
	reply.WriteU16Little(1);    //magic2
	reply.WriteU16Little(1);    //vit
	reply.WriteU16Little(1);    //vit2
	reply.WriteU16Little(1);    //intel
	reply.WriteU16Little(1);    //intel2
	reply.WriteU16Little(1);    //speed
	reply.WriteU16Little(1);    //speed2
	reply.WriteU16Little(1);    //luck
	reply.WriteU16Little(1);    //luck2
	reply.WriteU16Little(1);    //cls?
	reply.WriteU16Little(1);    //cls2?
	reply.WriteU16Little(1);    //lng?
	reply.WriteU16Little(1);    //lng2?
	reply.WriteU16Little(1);    //spell
	reply.WriteU16Little(1);    //spell2
	reply.WriteU16Little(1);    //supp?
	reply.WriteU16Little(1);    //supp2?
	reply.WriteU16Little(1);    //pdef
	reply.WriteU16Little(1);    //pdef2
	reply.WriteU16Little(1);    //mdef
	reply.WriteU16Little(1);    //mdef2

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

void SendStatusIcon(const std::shared_ptr<channel::ChannelClientConnection>& client)
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

bool Parsers::State::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    (void)pPacketManager;
    (void)p;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    if(nullptr == client->GetClientState() ||
        client->GetClientState()->GetCharacter().IsNull())
    {
        return false;
    }

    SendCharacterData(client);
    SendStatusIcon(client);

    return true;
}
