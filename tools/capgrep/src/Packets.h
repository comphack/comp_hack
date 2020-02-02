/**
 * @file tools/capgrep/src/Packets.h
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Clipboard actions for specific packets.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_CAPGREP_SRC_PACKETS_H
#define TOOLS_CAPGREP_SRC_PACKETS_H

#include "PacketData.h"

void action0014(PacketData *data, libcomp::Packet& packet,
    libcomp::Packet& packetBefore);
void action0015(PacketData *data, libcomp::Packet& packet,
    libcomp::Packet& packetBefore);
void action0023(PacketData *data, libcomp::Packet& packet,
    libcomp::Packet& packetBefore);
void action00A7(PacketData *data, libcomp::Packet& packet,
    libcomp::Packet& packetBefore);
void action00AC(PacketData *data, libcomp::Packet& packet,
    libcomp::Packet& packetBefore);
void action00B9(PacketData *data, libcomp::Packet& packet,
    libcomp::Packet& packetBefore);

#endif // TOOLS_CAPGREP_SRC_PACKETS_H
