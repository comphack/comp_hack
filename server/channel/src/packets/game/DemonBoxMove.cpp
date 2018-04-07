/**
 * @file server/channel/src/packets/game/DemonBoxMove.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to move a demon in a box.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>

// objects Includes
#include <Character.h>
#include <CharacterProgress.h>
#include <Demon.h>
#include <DemonBox.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterManager.h"

using namespace channel;

bool Parsers::DemonBoxMove::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 11)
    {
        return false;
    }

    int8_t srcBoxID = p.ReadS8();
    int64_t demonID = p.ReadS64Little();
    int8_t destBoxID = p.ReadS8();
    int8_t destSlot = p.ReadS8();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(connection);
    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager->GetServer());
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress();
    auto characterManager = server->GetCharacterManager();

    auto destBox = characterManager->GetDemonBox(state, destBoxID);

    auto srcDemon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(demonID)));

    int8_t srcSlot = srcDemon->GetBoxSlot();
    auto srcBox = std::dynamic_pointer_cast<objects::DemonBox>(
        libcomp::PersistentObject::GetObjectByUUID(srcDemon->GetDemonBox()));

    uint8_t maxDestSlots = destBoxID == 0 ? progress->GetMaxCOMPSlots() : 50;
    if(!srcBox || srcBoxID != srcBox->GetBoxID() ||
        srcDemon != srcBox->GetDemons((size_t)srcSlot).Get() || destSlot >= maxDestSlots)
    {
        LOG_ERROR(libcomp::String("Invalid arguments supplied for demon move request"
            " %1, %2, %3, %4\n").Arg(srcBoxID).Arg(demonID).Arg(destBoxID).Arg(destSlot));
        return false;
    }

    if(nullptr == destBox)
    {
        // Box has not been rented/obtained yet, the move is invalid so
        // just re-send the source box
        characterManager->SendDemonBoxData(client, srcBoxID);
        return true;
    }
    
    // If the active demon is being moved to a non-COMP box, store it first
    if(srcDemon != nullptr && srcDemon == character->GetActiveDemon().Get() && destBoxID != 0)
    {
        characterManager->StoreDemon(client);
    }

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    dbChanges->Update(srcDemon);
    dbChanges->Update(srcBox);
    dbChanges->Update(destBox);

    auto destDemon = destBox->GetDemons((size_t)destSlot);

    srcDemon->SetBoxSlot(destSlot);
    srcDemon->SetDemonBox(destBox->GetUUID());
    if(!destDemon.IsNull())
    {
        destDemon->SetBoxSlot(srcSlot);
        destDemon->SetDemonBox(srcBox->GetUUID());
        dbChanges->Update(destDemon.Get());
    }

    srcBox->SetDemons((size_t)srcSlot, destDemon);
    destBox->SetDemons((size_t)destSlot, srcDemon);

    if(srcBox != destBox)
    {
        characterManager->SendDemonBoxData(client, destBoxID, { destSlot });
        characterManager->SendDemonBoxData(client, srcBoxID, { srcSlot });
    }
    else
    {
        characterManager->SendDemonBoxData(client, srcBoxID, { srcSlot, destSlot });
    }

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    return true;
}
