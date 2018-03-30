/**
 * @file server/channel/src/packets/game/AppearanceAlter.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to alter the appearance of the currently
 *  logged in character.
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

// object Includes
#include <Item.h>
#include <ItemBox.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::AppearanceAlter::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 16)
    {
        return false;
    }

    int32_t shopID = p.ReadS32Little();
    int32_t cacheID = p.ReadS32Little();
    int64_t itemID = p.ReadS64Little();

    // Not sure when these would actually be used
    (void)shopID;
    (void)cacheID;

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto inventory = character->GetItemBoxes(0).Get();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();

    auto item = std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(
            state->GetObjectUUID(itemID)));

    auto itemData = item
        ? definitionManager->GetItemData(item->GetType()) : nullptr;

    bool success = itemData != nullptr &&
        item->GetItemBox().Get() == inventory;
    if(success)
    {
        auto basicData = itemData->GetBasic();

        int16_t appearanceID = basicData->GetAppearanceID();
        if(appearanceID != 0)
        {
            uint8_t hairType = character->GetHairType();
            uint8_t hairColor = character->GetHairColor();
            uint8_t leftEye = character->GetLeftEyeColor();
            uint8_t rightEye = character->GetRightEyeColor();

            // The following are not natively supported but can be
            // wired up to alteration items just as easily
            uint8_t skinType = character->GetSkinType();
            uint8_t eyeType = character->GetEyeType();
            uint8_t faceType = character->GetFaceType();

            switch(basicData->GetEquipType())
            {
            case objects::MiItemBasicData::EquipType_t::VIS_HAIR_STYLE:
                hairType = (uint8_t)appearanceID;
                break;
            case objects::MiItemBasicData::EquipType_t::VIS_HAIR_COLOR:
                hairColor = (uint8_t)appearanceID;
                break;
            case objects::MiItemBasicData::EquipType_t::VIS_EYE_COLOR_LEFT:
                leftEye = (uint8_t)appearanceID;
                break;
            case objects::MiItemBasicData::EquipType_t::VIS_EYE_COLOR_RIGHT:
                rightEye = (uint8_t)appearanceID;
                break;
            case objects::MiItemBasicData::EquipType_t::VIS_EYE_COLOR_BOTH:
                leftEye = (uint8_t)appearanceID;
                rightEye = (uint8_t)appearanceID;
                break;
            case objects::MiItemBasicData::EquipType_t::VIS_SKIN_TYPE:
                skinType = (uint8_t)appearanceID;
                break;
            case objects::MiItemBasicData::EquipType_t::VIS_EYE_TYPE:
                eyeType = (uint8_t)appearanceID;
                break;
            case objects::MiItemBasicData::EquipType_t::VIS_FACE_TYPE:
                faceType = (uint8_t)appearanceID;
                break;
            default:
                LOG_ERROR(libcomp::String("Request to alter appearance"
                    " received for an invalid appearance item: %1\n")
                    .Arg(item->GetType()));
                success = false;
                break;
            }

            if(success)
            {
                character->SetHairType(hairType);
                character->SetHairColor(hairColor);
                character->SetLeftEyeColor(leftEye);
                character->SetRightEyeColor(rightEye);

                character->SetSkinType(skinType);
                character->SetEyeType(eyeType);
                character->SetFaceType(faceType);
            }
        }
        else
        {
            LOG_ERROR(libcomp::String("Request to alter appearance received"
                " for an item with no appearance alteration value: %1\n")
                .Arg(item->GetType()));
            success = false;
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_APPEARANCE_ALTER);
    reply.WriteS32Little(success ? 0 : -1);

    if(success)
    {
        reply.WriteU8(character->GetSkinType());
        reply.WriteU8(character->GetHairType());
        reply.WriteU8(character->GetEyeType());
        reply.WriteU8(character->GetFaceType());
        reply.WriteU8(character->GetLeftEyeColor());
        reply.WriteU8(0); // Unused
        reply.WriteU8(0); // Unused
        reply.WriteU8(character->GetHairColor());
        reply.WriteU8(character->GetRightEyeColor());

        // Notify other players of the change
        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_APPEARANCE_ALTERED);
        notify.WriteS32Little(cState->GetEntityID());
        notify.WriteU8(character->GetSkinType());
        notify.WriteU8(character->GetHairType());
        notify.WriteU8(character->GetEyeType());
        notify.WriteU8(character->GetFaceType());
        notify.WriteU8(character->GetLeftEyeColor());
        notify.WriteU8(0); // Unused
        notify.WriteU8(0); // Unused
        notify.WriteU8(character->GetHairColor());
        notify.WriteU8(character->GetRightEyeColor());

        server->GetZoneManager()->BroadcastPacket(client, notify, false);

        client->QueuePacket(reply);

        // For some weird reason the item being removed can only be reported
        // AFTER the reply has been received or the inventory bugs out.
        // Removing earlier and reporting here doesn't seem worth the effort
        // so just remove now.
        std::unordered_map<uint32_t, uint32_t> items;
        items[item->GetType()] = 1;

        characterManager->AddRemoveItems(client, items,
            false, itemID);

        server->GetWorldDatabase()->QueueUpdate(character,
            state->GetAccountUID());
    }
    else
    {
        client->QueuePacket(reply);
    }

    client->FlushOutgoing();

    return true;
}
