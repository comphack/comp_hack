/**
 * @file tools/cathedral/src/SpawnRestrictionUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a configured SpawnRestriction.
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

#include "SpawnRestrictionUI.h"

// objects Includes
#include <SpawnRestriction.h>

// Qt Includes
#include <PushIgnore.h>
#include "ui_SpawnRestriction.h"
#include <PopIgnore.h>

SpawnRestriction::SpawnRestriction(QWidget *pParent) : QWidget(pParent)
{
    prop = new Ui::SpawnRestriction;
    prop->setupUi(this);

    prop->time->SetValueName(tr("To:"));
    prop->systemTime->SetValueName(tr("To:"));
    prop->date->SetValueName(tr("To:"));
}

SpawnRestriction::~SpawnRestriction()
{
    delete prop;
}

void SpawnRestriction::Load(const std::shared_ptr<objects::SpawnRestriction>& restrict)
{
    if(!restrict)
    {
        return;
    }

    prop->disabled->setChecked(restrict->GetDisabled());

    std::unordered_map<uint32_t, int32_t> time;
    std::unordered_map<uint32_t, int32_t> sysTime;
    std::unordered_map<uint32_t, int32_t> date;

    for(auto& pair : restrict->GetTimeRestriction())
    {
        time[(uint32_t)pair.first] = (int32_t)pair.second;
    }

    for(auto& pair : restrict->GetSystemTimeRestriction())
    {
        sysTime[(uint32_t)pair.first] = (int32_t)pair.second;
    }

    for(auto& pair : restrict->GetDateRestriction())
    {
        date[(uint32_t)pair.first] = (int32_t)pair.second;
    }

    prop->time->Load(time);
    prop->systemTime->Load(sysTime);
    prop->date->Load(date);

    // Set the moon controls
    std::vector<QPushButton*> moonControls;
    for(int i = 0; i < prop->layoutMoonWax->count(); i++)
    {
        moonControls.push_back((QPushButton*)prop->layoutMoonWax
            ->itemAt(i)->widget());
    }
    
    for(int i = 0; i < prop->layoutMoonWane->count(); i++)
    {
        moonControls.push_back((QPushButton*)prop->layoutMoonWane
            ->itemAt(i)->widget());
    }

    uint16_t moonRestrict = restrict->GetMoonRestriction();
    for(size_t i = 0; i < 16; i++)
    {
        moonControls[i]->setChecked((moonRestrict & (1 << i)) != 0);
    }

    // Set the day controls
    std::vector<QPushButton*> dayControls;
    for(int i = 0; i < prop->layoutDay->count(); i++)
    {
        dayControls.push_back((QPushButton*)prop->layoutDay
            ->itemAt(i)->widget());
    }

    uint16_t dayRestrict = restrict->GetDayRestriction();
    for(size_t i = 0; i < 7; i++)
    {
        dayControls[i]->setChecked((dayRestrict & (1 << i)) != 0);
    }
}

std::shared_ptr<objects::SpawnRestriction> SpawnRestriction::Save() const
{
    /// @todo

    return nullptr;
}
