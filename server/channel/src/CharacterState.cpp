/**
 * @file server/channel/src/CharacterState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of a player character on the channel.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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

#include "CharacterState.h"

// libcomp Includes
#include <DefinitionManager.h>

// objects Includes
#include <Item.h>
#include <MiEquipmentSetData.h>

using namespace channel;

CharacterState::CharacterState()
{
}

std::set<uint32_t> CharacterState::GetEquippedSets() const
{
    return mEquippedSets;
}

void CharacterState::SetEquippedSets(libcomp::DefinitionManager* definitionManager)
{
    auto character = GetEntity();
    if(!character)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mLock);
    mEquippedSets.clear();

    for(size_t i = 0; i < 15; i++)
    {
        auto equip = character->GetEquippedItems(i).Get();
        if(equip)
        {
            auto equipSets = definitionManager->GetEquipmentSetDataByItem(
                equip->GetType());
            for(auto s : equipSets)
            {
                bool invalid = mEquippedSets.find(s->GetID()) != mEquippedSets.end();
                if(!invalid && i > 0)
                {
                    // If the set has not already been added by an
                    // earlier equipment slot and there are earlier
                    // equipment slot pieces needed, ignore the set
                    for(size_t k = i; k > 0;  k--)
                    {
                        if(s->GetEquipment(k - 1))
                        {
                            invalid = true;
                            break;
                        }
                    }
                }

                if(!invalid)
                {
                    for(size_t k = i + 1; k < 15; k++)
                    {
                        uint32_t sEquip = s->GetEquipment(k);
                        if(sEquip && (!character->GetEquippedItems(k) ||
                            character->GetEquippedItems(k)->GetType() != sEquip))
                        {
                            invalid = true;
                        }
                    }

                    if(!invalid)
                    {
                        mEquippedSets.insert(s->GetID());
                    }
                }
            }
        }
    }
}
