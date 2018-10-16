/**
 * @file server/channel/src/packets/game/ItemPromo.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Request from the client to submit an item promo code.
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
#include <PostItem.h>
#include <Promo.h>
#include <PromoExchange.h>

// channel Includes
#include "ChannelServer.h"

using namespace channel;

bool Parsers::ItemPromo::Parse(libcomp::ManagerPacket *pPacketManager,
    const std::shared_ptr<libcomp::TcpConnection>& connection,
    libcomp::ReadOnlyPacket& p) const
{
    if(p.Left() != (uint16_t)(2 + p.PeekU16Little()))
    {
        return false;
    }

    auto server = std::dynamic_pointer_cast<ChannelServer>(pPacketManager
        ->GetServer());
    auto db = server->GetLobbyDatabase();

    auto client = std::dynamic_pointer_cast<ChannelClientConnection>(
        connection);
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    libcomp::String code = p.ReadString16Little(
        state->GetClientStringEncoding(), true);

    uint32_t now = (uint32_t)std::time(0);
    uint8_t worldID = server->GetRegisteredWorld()->GetID();

    auto existing = objects::PromoExchange::LoadPromoExchangeListByAccount(
        db, state->GetAccountUID());

    bool success = false;

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state
        ->GetAccountUID());
    for(auto promo : objects::Promo::LoadPromoListByCode(db, code))
    {
        if(promo->GetStartTime() <= now && (!promo->GetEndTime() ||
            promo->GetEndTime() >= now))
        {
            bool valid = true;
            if(promo->GetLimit())
            {
                auto exchanges = existing;
                exchanges.remove_if([promo](
                    const std::shared_ptr<objects::PromoExchange>& e)
                    {
                        return e->GetPromo() != promo->GetUUID();
                    });

                switch(promo->GetLimitType())
                {
                case objects::Promo::LimitType_t::PER_CHARACTER:
                    exchanges.remove_if([cState](
                        const std::shared_ptr<objects::PromoExchange>& e)
                        {
                            return e->GetCharacter().GetUUID() !=
                                cState->GetEntityUUID();
                        });
                    break;
                case objects::Promo::LimitType_t::PER_WORLD:
                    exchanges.remove_if([worldID](
                        const std::shared_ptr<objects::PromoExchange>& e)
                        {
                            return e->GetWorldID() != worldID;
                        });
                    break;
                default:
                    break;
                }

                valid = (size_t)promo->GetLimit() > exchanges.size();
            }

            if(valid)
            {
                auto exchange = libcomp::PersistentObject::New<
                    objects::PromoExchange>(true);
                exchange->SetPromo(promo->GetUUID());
                exchange->SetAccount(state->GetAccountUID());
                exchange->SetCharacter(cState->GetEntityUUID());
                exchange->SetTimestamp(now);
                exchange->SetWorldID(worldID);

                dbChanges->Insert(exchange);

                for(uint32_t productID : promo->GetPostItems())
                {
                    auto postItem = libcomp::PersistentObject::New<
                        objects::PostItem>(true);
                    postItem->SetSource(objects::PostItem::Source_t::PROMOTION);
                    postItem->SetType(productID);
                    postItem->SetTimestamp(now);
                    postItem->SetAccount(state->GetAccountUID());

                    dbChanges->Insert(postItem);
                }

                success = true;
            }
        }
    }

    if(success)
    {
        success = db->ProcessChangeSet(dbChanges);
    }

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_ITEM_PROMO);
    reply.WriteString16Little(libcomp::Convert::ENCODING_CP932,
        code, true);
    reply.WriteS32Little(success ? 0 : 1);

    // Apart from success/fail, nothing in this packet changes anything
    reply.WriteS32Little(0);
    reply.WriteS32Little(0);
    reply.WriteS8(0);

    client->SendPacket(reply);

    return true;
}
