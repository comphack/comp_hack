/**
 * @file server/channel/src/Packets.h
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Classes used to parse client channel packets.
 *
 * This file is part of the channel Server (channel).
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

#ifndef LIBCOMP_SRC_PACKETS_H
#define LIBCOMP_SRC_PACKETS_H

// libcomp Includes
#include <PacketParser.h>

namespace channel
{

namespace Parsers
{

PACKET_PARSER_DECL(Login);               // 0x0000
PACKET_PARSER_DECL(Auth);                // 0x0002
PACKET_PARSER_DECL(SendData);            // 0x0004
PACKET_PARSER_DECL(Logout);              // 0x0005
PACKET_PARSER_DECL(PopulateZone);        // 0x0019
PACKET_PARSER_DECL(Move);                // 0x001C
PACKET_PARSER_DECL(Chat);                // 0x0026
PACKET_PARSER_DECL(ActivateSkill);       // 0x0030
PACKET_PARSER_DECL(AllocateSkillPoint);  // 0x0049
PACKET_PARSER_DECL(ToggleExpertise);     // 0x004F
PACKET_PARSER_DECL(LearnSkill);          // 0x0051
PACKET_PARSER_DECL(KeepAlive);           // 0x0056
PACKET_PARSER_DECL(FixObjectPosition);   // 0x0058
PACKET_PARSER_DECL(State);               // 0x005A
PACKET_PARSER_DECL(PartnerDemonData);    // 0x005B
PACKET_PARSER_DECL(COMPList);            // 0x005C
PACKET_PARSER_DECL(COMPDemonData);       // 0x005E
PACKET_PARSER_DECL(StopMovement);        // 0x006F
PACKET_PARSER_DECL(ItemBox);             // 0x0074
PACKET_PARSER_DECL(ItemMove);            // 0x0076
PACKET_PARSER_DECL(ItemDrop);            // 0x0077
PACKET_PARSER_DECL(ItemStack);           // 0x0078
PACKET_PARSER_DECL(EquipmentList);       // 0x007B
PACKET_PARSER_DECL(COMPSlotUpdate);      // 0x009A
PACKET_PARSER_DECL(DismissDemon);        // 0x009B
PACKET_PARSER_DECL(HotbarData);          // 0x00A2
PACKET_PARSER_DECL(HotbarSave);          // 0x00A4
PACKET_PARSER_DECL(ValuableList);        // 0x00B8
PACKET_PARSER_DECL(Sync);                // 0x00F3
PACKET_PARSER_DECL(Rotate);              // 0x00F8
PACKET_PARSER_DECL(UnionFlag);           // 0x0100
PACKET_PARSER_DECL(LockDemon);           // 0x0233

PACKET_PARSER_DECL(SetWorldInfo);        // 0x1002
PACKET_PARSER_DECL(AccountLogin);        // 0x1004

} // namespace Parsers

} // namespace channel

#endif // LIBCOMP_SRC_PACKETS_H
