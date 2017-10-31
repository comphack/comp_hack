/**
 * @file server/channel/src/EntityState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief State of a non-active entity on the channel.
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

#include "EntityState.h"

// channel Includes
#include <LootBox.h>
#include <ServerBazaar.h>
#include <ServerNPC.h>
#include <ServerObject.h>

namespace channel
{

template<>
EntityState<objects::ServerObject>::EntityState(
    const std::shared_ptr<objects::ServerObject>& entity)
    : mEntity(entity)
{
    SetEntityType(objects::EntityStateObject::EntityType_t::OBJECT);
}

template<>
EntityState<objects::ServerNPC>::EntityState(
    const std::shared_ptr<objects::ServerNPC>& entity)
    : mEntity(entity)
{
    SetEntityType(objects::EntityStateObject::EntityType_t::NPC);
}

template<>
EntityState<objects::ServerBazaar>::EntityState(
    const std::shared_ptr<objects::ServerBazaar>& entity)
    : mEntity(entity)
{
    SetEntityType(objects::EntityStateObject::EntityType_t::BAZAAR);
}

template<>
EntityState<objects::LootBox>::EntityState(
    const std::shared_ptr<objects::LootBox>& entity)
    : mEntity(entity)
{
    SetEntityType(objects::EntityStateObject::EntityType_t::LOOT_BOX);
}

}
