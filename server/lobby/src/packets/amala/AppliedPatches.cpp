/**
 * @file server/lobby/src/packets/amala/AppliedPatches.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Request from the client to dump the account information.
 *
 * This file is part of the Lobby Server (lobby).
 *
 * Copyright (C) 2012-2021 COMP_hack Team <compomega@tutanota.com>
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
#include <Log.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ReadOnlyPacket.h>
#include <TcpConnection.h>

// lobby Includes
#include "LobbyServer.h"
#include "ManagerConnection.h"

using namespace lobby;

bool Parsers::AmalaAppliedPatches::Parse(
    libcomp::ManagerPacket* pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const {
  if (5 > p.Size()) {
    return false;
  }

  uint8_t majorVersion = p.ReadU8();
  uint8_t minorVersion = p.ReadU8();
  uint8_t patchVersion = p.ReadU8();

  if (p.Left() != (uint32_t)(2 + p.PeekS16())) {
    return false;
  }

  libcomp::String patches =
      p.ReadString16Little(libcomp::Convert::ENCODING_UTF8);

  auto conf = config(pPacketManager);
  auto username = state(connection)->GetUsername();

  LogGeneralInfo([&]() {
    return libcomp::String(
               "Client '%1' is running v%2.%3.%4 of the client patch. Running "
               "patches: %5\n")
        .Arg(username)
        .Arg((int)majorVersion)
        .Arg((int)minorVersion)
        .Arg((int)patchVersion)
        .Arg(patches);
  });

  // If there is no enforcement, don't check anything else.
  if (objects::LobbyConfig::ClientPatchEnforcement_t::NONE ==
      conf->GetClientPatchEnforcement()) {
    return true;
  }

  // Client patch version is always enforced.
  uint32_t actualVersion =
      static_cast<uint32_t>((majorVersion * 1000) + minorVersion);
  uint32_t expectedVersion =
      static_cast<uint32_t>(conf->GetClientPatchVersion() * 1000.0f + 0.5f);

  if (actualVersion != expectedVersion) {
    LogGeneralWarning([&]() {
      return libcomp::String(
                 "Client '%1' is running the wrong client patch version. They "
                 "will be denied login.\n")
          .Arg(username);
    });

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyToClientPacketCode_t::PACKET_AMALA_WRONG_CLIENT_PATCH_VERSION);

    connection->SendPacket(reply);

    return true;
  }

  auto patchesList = patches.Split(",");
  auto requiredPatches = conf->GetClientRequiredPatches();
  auto blockedPatches = conf->GetClientBlockedPatches();

  std::list<libcomp::String> badPatches;

  // Required patches are always required.
  for (auto patchName : requiredPatches) {
    if (patchesList.end() ==
        std::find(patchesList.begin(), patchesList.end(), patchName)) {
      badPatches.push_back(patchName);
    }
  }

  if (!badPatches.empty()) {
    patches = libcomp::String::Join(badPatches, ",");

    LogGeneralWarning([&]() {
      return libcomp::String(
                 "Client '%1' will be denied login because they do not have "
                 "the following required patch(es) applied: %2\n")
          .Arg(username)
          .Arg(patches);
    });

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyToClientPacketCode_t::PACKET_AMALA_CLIENT_PATCH_MISSING);
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8, patches);

    connection->SendPacket(reply);

    return true;
  }

  // Some patches are optional unless ALLOW_ONLY_LISTED is set.
  if (objects::LobbyConfig::ClientPatchEnforcement_t::ALLOW_ONLY_LISTED ==
      conf->GetClientPatchEnforcement()) {
    auto allowedPatches = conf->GetClientAllowedPatches();

    for (auto patchName : patchesList) {
      bool isRequiredPatch =
          requiredPatches.end() !=
          std::find(requiredPatches.begin(), requiredPatches.end(), patchName);
      bool isAllowedPatch =
          allowedPatches.end() !=
          std::find(allowedPatches.begin(), allowedPatches.end(), patchName);
      if (!isRequiredPatch && !isAllowedPatch) {
        badPatches.push_back(patchName);
      }
    }
  }

  // Blocked patches are always blocked.
  for (auto patchName : blockedPatches) {
    if (patchesList.end() !=
        std::find(patchesList.begin(), patchesList.end(), patchName)) {
      badPatches.push_back(patchName);
    }
  }

  if (!badPatches.empty()) {
    patches = libcomp::String::Join(badPatches, ",");

    LogGeneralWarning([&]() {
      return libcomp::String(
                 "Client '%1' will be denied login because they have the "
                 "following blocked or disallowed patch(es) applied: %2\n")
          .Arg(username)
          .Arg(patches);
    });

    libcomp::Packet reply;
    reply.WritePacketCode(
        LobbyToClientPacketCode_t::PACKET_AMALA_CLIENT_PATCH_BLOCKED);
    reply.WriteString16Little(libcomp::Convert::ENCODING_UTF8, patches);

    connection->SendPacket(reply);

    return true;
  }

  // Client patches have been validated; allow login.
  state(connection)->SetHaveValidClientPatches(true);

  return true;
}
