/**
 * @file server/channel/src/ActiveEntityState.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Represents the state of an active entity on the channel.
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

#include "ActiveEntityState.h"

// libcomp Includes
#include <Constants.h>
#include <ScriptEngine.h>

// objects Includes
#include <Character.h>
#include <Demon.h>
#include <Enemy.h>
#include <Item.h>
#include <MiCorrectTbl.h>
#include <MiDevilBattleData.h>
#include <MiDevilData.h>
#include <MiItemData.h>
#include <MiSkillItemStatusCommonData.h>

// channel Includes
#include "CharacterManager.h"
#include "DefinitionManager.h"

using namespace channel;

void ActiveEntityState::Move(float xPos, float yPos, uint64_t now)
{
    SetOriginX(GetCurrentX());
    SetOriginY(GetCurrentY());
    SetOriginRotation(GetCurrentRotation());
    SetOriginTicks(now);

    SetDestinationX(xPos);
    SetDestinationY(yPos);
    SetDestinationTicks(now + 500000);  /// @todo: modify for speed
}

void ActiveEntityState::Rotate(float rot, uint64_t now)
{
    SetOriginX(GetCurrentX());
    SetOriginY(GetCurrentY());
    SetOriginRotation(GetCurrentRotation());
    SetOriginTicks(now);

    SetDestinationRotation(CorrectRotation(rot));
    SetDestinationTicks(now + 500000);  /// @todo: modify for speed
}

void ActiveEntityState::Stop(uint64_t now)
{
    SetDestinationX(GetCurrentX());
    SetDestinationY(GetCurrentY());
    SetDestinationRotation(GetCurrentRotation());
    SetDestinationTicks(now);
    SetOriginX(GetCurrentX());
    SetOriginY(GetCurrentY());
    SetOriginRotation(GetCurrentRotation());
    SetOriginTicks(now);
}

bool ActiveEntityState::IsMoving() const
{
    return GetCurrentX() != GetDestinationX() ||
        GetCurrentY() != GetDestinationY();
}

bool ActiveEntityState::IsRotating() const
{
    return GetCurrentRotation() != GetDestinationRotation();
}

void ActiveEntityState::RefreshCurrentPosition(uint64_t now)
{
    if(now != mLastRefresh)
    {
        mLastRefresh = now;

        float currentX = GetCurrentX();
        float currentY = GetCurrentY();
        float currentRot = GetCurrentRotation();

        float destX = GetDestinationX();
        float destY = GetDestinationY();
        float destRot = GetDestinationRotation();

        bool xDiff = currentX != destX;
        bool yDiff = currentY != destY;
        bool rotDiff = currentRot != destRot;

        if(!xDiff && !yDiff && !rotDiff)
        {
            // Already up to date
            return;
        }

        uint64_t destTicks = GetDestinationTicks();

        if(now >= destTicks)
        {
            SetCurrentX(destX);
            SetCurrentY(destY);
            SetCurrentRotation(destRot);
        }
        else
        {
            float originX = GetOriginX();
            float originY = GetOriginY();
            float originRot = GetOriginRotation();
            uint64_t originTicks = GetOriginTicks();

            uint64_t elapsed = now - originTicks;
            uint64_t total = destTicks - originTicks;

            double prog = (double)((double)elapsed/(double)total);
            if(xDiff || yDiff)
            {
                float newX = (float)(originX + (prog * (destX - originX)));
                float newY = (float)(originY + (prog * (destY - originY)));

                SetCurrentX(newX);
                SetCurrentY(newY);
            }

            if(rotDiff)
            {
                // Bump both origin and destination by 3.14 to range from
                // 0-+6.28 instead of -3.14-+3.14 for simpler math
                originRot += 3.14f;
                destRot += 3.14f;

                float newRot = (float)(originRot + (prog * (destRot - originRot)));

                SetCurrentRotation(CorrectRotation(newRot));
            }
        }
    }
}

float ActiveEntityState::CorrectRotation(float rot) const
{
    if(rot > 3.16f)
    {
        return rot - 6.32f;
    }
    else if(rot < -3.16f)
    {
        return -rot - 3.16f;
    }

    return rot;
}

namespace channel
{

template<>
ActiveEntityStateImp<objects::Character>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::CHARACTER);
}

template<>
ActiveEntityStateImp<objects::Demon>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::PARTNER_DEMON);
}

template<>
ActiveEntityStateImp<objects::Enemy>::ActiveEntityStateImp()
{
    SetEntityType(objects::EntityStateObject::EntityType_t::ENEMY);
}

template<>
bool ActiveEntityStateImp<objects::Character>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager)
{
    auto c = GetEntity();
    auto cs = c->GetCoreStats().Get();

    std::unordered_map<uint8_t, int16_t> correctMap = CharacterManager::GetCharacterBaseStatMap(cs);

    for(auto equip : c->GetEquippedItems())
    {
        if(!equip.IsNull())
        {
            auto itemData = definitionManager->GetItemData(equip->GetType());
            auto itemCommonData = itemData->GetCommon();

            for(auto ct : itemCommonData->GetCorrectTbl())
            {
                auto tblID = ct->GetID();
                switch (ct->GetType())
                {
                    case objects::MiCorrectTbl::Type_t::NUMERIC:
                        correctMap[tblID] = (int16_t)(correctMap[tblID] + ct->GetValue());
                        break;
                    case objects::MiCorrectTbl::Type_t::PERCENT:
                        /// @todo: apply in the right order
                        break;
                    default:
                        break;
                }
            }
        }
    }

    /// @todo: apply effects

    CharacterManager::CalculateDependentStats(correctMap, cs->GetLevel(), false);

    SetMaxHP(correctMap[libcomp::CORRECT_MAXHP]);
    SetMaxMP(correctMap[libcomp::CORRECT_MAXMP]);
    SetSTR(correctMap[libcomp::CORRECT_STR]);
    SetMAGIC(correctMap[libcomp::CORRECT_MAGIC]);
    SetVIT(correctMap[libcomp::CORRECT_VIT]);
    SetINTEL(correctMap[libcomp::CORRECT_INTEL]);
    SetSPEED(correctMap[libcomp::CORRECT_SPEED]);
    SetLUCK(correctMap[libcomp::CORRECT_LUCK]);
    SetCLSR(correctMap[libcomp::CORRECT_CLSR]);
    SetLNGR(correctMap[libcomp::CORRECT_LNGR]);
    SetSPELL(correctMap[libcomp::CORRECT_SPELL]);
    SetSUPPORT(correctMap[libcomp::CORRECT_SUPPORT]);
    SetPDEF(correctMap[libcomp::CORRECT_PDEF]);
    SetMDEF(correctMap[libcomp::CORRECT_MDEF]);

    return true;
}

bool RecalculateDemonStats(libcomp::DefinitionManager* definitionManager,
    objects::ActiveEntityStateObject* entity,
    std::shared_ptr<objects::EntityStats> cs, uint32_t demonID)
{
    auto demonData = definitionManager->GetDevilData(demonID);
    auto battleData = demonData->GetBattleData();

    std::unordered_map<uint8_t, int16_t> correctMap;
    correctMap[libcomp::CORRECT_STR] = cs->GetSTR();
    correctMap[libcomp::CORRECT_MAGIC] = cs->GetMAGIC();
    correctMap[libcomp::CORRECT_VIT] = cs->GetVIT();
    correctMap[libcomp::CORRECT_INTEL] = cs->GetINTEL();
    correctMap[libcomp::CORRECT_SPEED] = cs->GetSPEED();
    correctMap[libcomp::CORRECT_LUCK] = cs->GetLUCK();
    correctMap[libcomp::CORRECT_MAXHP] = battleData->GetCorrect(libcomp::CORRECT_MAXHP);
    correctMap[libcomp::CORRECT_MAXMP] = battleData->GetCorrect(libcomp::CORRECT_MAXMP);
    correctMap[libcomp::CORRECT_CLSR] = battleData->GetCorrect(libcomp::CORRECT_CLSR);
    correctMap[libcomp::CORRECT_LNGR] = battleData->GetCorrect(libcomp::CORRECT_LNGR);
    correctMap[libcomp::CORRECT_SPELL] = battleData->GetCorrect(libcomp::CORRECT_SPELL);
    correctMap[libcomp::CORRECT_SUPPORT] = battleData->GetCorrect(libcomp::CORRECT_SUPPORT);
    correctMap[libcomp::CORRECT_PDEF] = battleData->GetCorrect(libcomp::CORRECT_PDEF);
    correctMap[libcomp::CORRECT_MDEF] = battleData->GetCorrect(libcomp::CORRECT_MDEF);

    /// @todo: apply effects

    CharacterManager::CalculateDependentStats(correctMap, cs->GetLevel(), true);

    entity->SetMaxHP(correctMap[libcomp::CORRECT_MAXHP]);
    entity->SetMaxMP(correctMap[libcomp::CORRECT_MAXMP]);
    entity->SetSTR(correctMap[libcomp::CORRECT_STR]);
    entity->SetMAGIC(correctMap[libcomp::CORRECT_MAGIC]);
    entity->SetVIT(correctMap[libcomp::CORRECT_VIT]);
    entity->SetINTEL(correctMap[libcomp::CORRECT_INTEL]);
    entity->SetSPEED(correctMap[libcomp::CORRECT_SPEED]);
    entity->SetLUCK(correctMap[libcomp::CORRECT_LUCK]);
    entity->SetCLSR(correctMap[libcomp::CORRECT_CLSR]);
    entity->SetLNGR(correctMap[libcomp::CORRECT_LNGR]);
    entity->SetSPELL(correctMap[libcomp::CORRECT_SPELL]);
    entity->SetSUPPORT(correctMap[libcomp::CORRECT_SUPPORT]);
    entity->SetPDEF(correctMap[libcomp::CORRECT_PDEF]);
    entity->SetMDEF(correctMap[libcomp::CORRECT_MDEF]);

    return true;
}

template<>
bool ActiveEntityStateImp<objects::Demon>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager)
{
    auto d = GetEntity();
    if (nullptr == d)
    {
        return true;
    }

    return RecalculateDemonStats(definitionManager, this, d->GetCoreStats().Get(), d->GetType());
}

template<>
bool ActiveEntityStateImp<objects::Enemy>::RecalculateStats(
    libcomp::DefinitionManager* definitionManager)
{
    auto e = GetEntity();
    if (nullptr == e)
    {
        return true;
    }

    return RecalculateDemonStats(definitionManager, this, e->GetCoreStats().Get(), e->GetType());
}

}
