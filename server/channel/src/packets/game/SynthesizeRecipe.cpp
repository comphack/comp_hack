/**
 * @file server/channel/src/packets/game/SynthesizeRecipe.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to select the synthesis recipe that
 *  will be peformed via a followup packet.
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
#include <DefinitionManager.h>
#include <Log.h>
#include <ManagerPacket.h>
#include <Packet.h>
#include <PacketCodes.h>
#include <ServerConstants.h>

// object Includes
#include <Item.h>
#include <MiSynthesisData.h>
#include <MiSynthesisItemData.h>
#include <PlayerExchangeSession.h>

// channel Includes
#include "CharacterManager.h"
#include "ChannelServer.h"

using namespace channel;

bool Parsers::SynthesizeRecipe::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Size() != 20)
    {
        return false;
    }

    uint32_t recipeID = p.ReadU32Little();
    int64_t catalystID = p.ReadS64Little();

    // Apparently this was never implemented
    int64_t protectionItemID = p.ReadS64Little();

    auto server = std::dynamic_pointer_cast<ChannelServer>(
        pPacketManager->GetServer());
    auto definitionManager = server->GetDefinitionManager();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto exchangeSession = state->GetExchangeSession();

    auto catalyst = catalystID ? std::dynamic_pointer_cast<objects::Item>(
        libcomp::PersistentObject::GetObjectByUUID(state->GetObjectUUID(catalystID)))
        : nullptr;

    auto synthData = definitionManager->GetSynthesisData(recipeID);
    int16_t successRate = 0;

    bool success = false;
    if((!catalystID || catalyst) && exchangeSession && synthData)
    {
        exchangeSession->SetSelectionID(recipeID);
        
        if(catalyst)
        {
            exchangeSession->SetItems(0, catalyst);

            int8_t catalystIdx = -1;

            auto it = SVR_CONST.RATE_SCALING_ITEMS[3].begin();
            for(size_t i = 0; i < SVR_CONST.RATE_SCALING_ITEMS[3].size(); i++)
            {
                if(*it == catalyst->GetType())
                {
                    catalystIdx = (int8_t)(i + 1);
                    break;
                }

                it++;
            }

            if(catalystIdx != -1)
            {
                successRate = synthData->GetRateScaling((size_t)catalystIdx);
                success = successRate != 0;
            }
        }
        else
        {
            successRate = synthData->GetRateScaling(0);
            success = true;
        }

        if(success)
        {
            // Verify materials
            auto character = cState->GetEntity();
            for(auto mat : synthData->GetMaterials())
            {
                uint32_t materialID = mat->GetItemID();

                if(materialID &&
                    character->GetMaterials(materialID) < mat->GetAmount())
                {
                    LOG_ERROR(libcomp::String("SynthesizeRecipe set attampted"
                        " without the necessary materials: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                    success = false;
                    break;
                }
            }
        }
    }

    libcomp::Packet reply;
    reply.WritePacketCode(
        ChannelToClientPacketCode_t::PACKET_SYNTHESIZE_RECIPE);
    reply.WriteU32Little(recipeID);
    reply.WriteS64Little(catalystID);
    reply.WriteU32Little(catalyst ? catalyst->GetType() : 0);
    reply.WriteS64Little(protectionItemID);
    reply.WriteU32Little(0);    // Protection item type?
    reply.WriteS32Little(success ? 0 : 1);
    reply.WriteU32Little(synthData ? synthData->GetItemID() : 0);
    reply.WriteS16Little(successRate);

    client->SendPacket(reply);

    if(!success)
    {
        server->GetCharacterManager()->EndExchange(client);
    }

    return true;
}
