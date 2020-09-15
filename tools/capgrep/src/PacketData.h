/**
 * @file tools/capgrep/src/PacketData.h
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Data for a single packet in a capture.
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

#ifndef TOOLS_CAPGREP_SRC_PACKETDATA_H
#define TOOLS_CAPGREP_SRC_PACKETDATA_H

// libcomp Includes
#include <Packet.h>

// Standard C Incldues
#include <stdint.h>

// Ignore warnings
#include <PushIgnore.h>

#include <QByteArray>
#include <QMetaType>
#include <QString>

// Stop ignoring warnings
#include <PopIgnore.h>

class PacketData;

typedef void (*CopyFunc)(PacketData* data, libcomp::Packet& packet,
                         libcomp::Packet& packetBefore);

class PacketData {
 public:
  uint16_t seq;
  uint16_t cmd;
  uint8_t source;
  uint32_t servTime;
  uint64_t micro;
  float servRate;
  QString text;
  QString desc;
  QString shortName;
  QByteArray data;
  CopyFunc copyAction;
  int client;  // -1 = default, 0 = A, 1 = B, etc.
};

Q_DECLARE_METATYPE(PacketData*)

#endif  // TOOLS_CAPGREP_SRC_PACKETDATA_H
