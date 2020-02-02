/**
 * @file tools/cathedral/src/SpawnLocationUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a configured SpawnLocation.
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

#include "SpawnLocationUI.h"

// objects Includes
#include <SpawnLocation.h>

// Qt Includes
#include <PushIgnore.h>
#include "ui_SpawnLocation.h"
#include <PopIgnore.h>

SpawnLocation::SpawnLocation(QWidget *pParent) : QWidget(pParent)
{
    prop = new Ui::SpawnLocation;
    prop->setupUi(this);
}

SpawnLocation::~SpawnLocation()
{
    delete prop;
}

void SpawnLocation::Load(const std::shared_ptr<objects::SpawnLocation>& loc)
{
    if(loc)
    {
        prop->x->setValue((double)loc->GetX());
        prop->y->setValue((double)loc->GetY());
        prop->width->setValue((double)loc->GetWidth());
        prop->height->setValue((double)loc->GetHeight());
    }
    else
    {
        prop->x->setValue(0.0);
        prop->y->setValue(0.0);
        prop->width->setValue(0.0);
        prop->height->setValue(0.0);
    }
}

std::shared_ptr<objects::SpawnLocation> SpawnLocation::Save() const
{
    auto obj = std::make_shared<objects::SpawnLocation>();
    obj->SetX((float)prop->x->value());
    obj->SetY((float)prop->y->value());
    obj->SetWidth((float)prop->width->value());
    obj->SetHeight((float)prop->height->value());
    return obj;
}
