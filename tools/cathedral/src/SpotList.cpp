/**
 * @file tools/cathedral/src/SpotList.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a control that holds a list of zone spots.
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

#include "SpotList.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_SpotProperties.h"
#include "ui_ObjectList.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

// objects Includes
#include <ServerZoneSpot.h>
#include <SpawnLocation.h>

SpotList::SpotList(QWidget *pParent) : ObjectList(pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::SpotProperties;
    prop->setupUi(pWidget);

    ui->splitter->addWidget(pWidget);
}

SpotList::~SpotList()
{
    delete prop;
}

void SpotList::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;

    prop->actionList->SetMainWindow(pMainWindow);
}

QString SpotList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto spot = std::dynamic_pointer_cast<objects::ServerZoneSpot>(obj);

    if(!spot)
    {
        return {};
    }

    return QString::number(spot->GetID());
}

void SpotList::LoadProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto spot = std::dynamic_pointer_cast<objects::ServerZoneSpot>(obj);

    if(!spot)
    {
        return;
    }

    prop->id->setValue((int)spot->GetID());

    auto area = spot->GetSpawnArea();

    if(area)
    {
        prop->x->setValue(area->GetX());
        prop->y->setValue(area->GetY());
        prop->width->setValue(area->GetWidth());
        prop->height->setValue(area->GetHeight());
    }
    else
    {
        prop->x->setValue(0);
        prop->y->setValue(0);
        prop->width->setValue(0);
        prop->height->setValue(0);
    }

    prop->actionList->Load(spot->GetActions());
    /// @todo: add more
}

void SpotList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    (void)obj;
}
