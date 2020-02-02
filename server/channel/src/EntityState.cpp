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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <DiasporaBase.h>
#include <LootBox.h>
#include <PlasmaSpawn.h>
#include <PvPBase.h>
#include <ServerBazaar.h>
#include <ServerCultureMachineSet.h>
#include <ServerNPC.h>
#include <ServerObject.h>

namespace channel
{

template<>
EntityState<objects::DiasporaBase>::EntityState(
    const std::shared_ptr<objects::DiasporaBase>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::DIASPORA_BASE);
}

template<>
EntityState<objects::ServerObject>::EntityState(
    const std::shared_ptr<objects::ServerObject>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::OBJECT);
}

template<>
EntityState<objects::ServerNPC>::EntityState(
    const std::shared_ptr<objects::ServerNPC>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::NPC);
}

template<>
EntityState<objects::ServerBazaar>::EntityState(
    const std::shared_ptr<objects::ServerBazaar>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::BAZAAR);
}

template<>
EntityState<objects::ServerCultureMachineSet>::EntityState(
    const std::shared_ptr<objects::ServerCultureMachineSet>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::CULTURE_MACHINE);
}

template<>
EntityState<objects::LootBox>::EntityState(
    const std::shared_ptr<objects::LootBox>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::LOOT_BOX);
}

template<>
EntityState<objects::PlasmaSpawn>::EntityState(
    const std::shared_ptr<objects::PlasmaSpawn>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::PLASMA);
}

template<>
EntityState<objects::PvPBase>::EntityState(
    const std::shared_ptr<objects::PvPBase>& entity)
    : mEntity(entity)
{
    SetEntityType(EntityType_t::PVP_BASE);
}

}
