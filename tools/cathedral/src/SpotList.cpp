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
#include "ZoneWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_SpotProperties.h"
#include "ui_ObjectList.h"
#include <PopIgnore.h>

// objects Includes
#include <MiSpotData.h>
#include <ServerZone.h>
#include <ServerZoneSpot.h>
#include <SpawnLocation.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

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

    prop->actions->SetMainWindow(pMainWindow);
    prop->leaveActions->SetMainWindow(pMainWindow);
}

QString SpotList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto spotDef = std::dynamic_pointer_cast<objects::MiSpotData>(obj);
    auto spot = std::dynamic_pointer_cast<objects::ServerZoneSpot>(obj);

    if(spotDef)
    {
        // Client definition
        return QString::number(spotDef->GetID());
    }
    else if(spot)
    {
        // Server only definition
        return QString::number(spot->GetID());
    }

    return {};
}

QString SpotList::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto spotDef = std::dynamic_pointer_cast<objects::MiSpotData>(obj);
    auto spot = std::dynamic_pointer_cast<objects::ServerZoneSpot>(obj);

    if(spotDef)
    {
        QString typeTxt = prop->type->itemText((int)spotDef->GetType());

        if(mMainWindow)
        {
            auto zoneWindow = mMainWindow->GetZones();
            if(zoneWindow)
            {
                auto merged = zoneWindow->GetMergedZone();
                if(merged && merged->Definition->SpotsKeyExists(spotDef
                    ->GetID()))
                {
                    return QString("%1 [%2] [Defined]").arg(typeTxt)
                        .arg(spotDef->GetType());
                }
            }
        }

        // Client only definition
        return QString("%1 [%2]").arg(typeTxt).arg(spotDef->GetType());
    }
    else if(spot)
    {
        // Server only definition
        return "[INVALID]";
    }

    return {};
}

void SpotList::LoadProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto parentWidget = prop->layoutMain->itemAt(0)->widget();
    if(!obj)
    {
        parentWidget->hide();
    }
    else if(parentWidget->isHidden())
    {
        parentWidget->show();
    }

    auto spotDef = std::dynamic_pointer_cast<objects::MiSpotData>(obj);
    auto spot = std::dynamic_pointer_cast<objects::ServerZoneSpot>(obj);

    if(spotDef)
    {
        // Client definition
        prop->id->setText(QString::number(spotDef->GetID()));
        prop->x->setText(QString::number((double)spotDef->GetCenterX()));
        prop->y->setText(QString::number((double)spotDef->GetCenterY()));
        prop->rotation->setText(QString::number((double)spotDef
            ->GetRotation()));
        prop->width->setText(QString::number((double)spotDef->GetSpanX()));
        prop->height->setText(QString::number((double)spotDef->GetSpanY()));
        prop->type->setCurrentIndex(spotDef->GetType());
        prop->chkEnabled->setChecked(spotDef->GetEnabled());
        prop->lblArguments->setText(QString("Arguments: (%1, %2, %3, %4)")
            .arg(spotDef->GetArgs(0)).arg(spotDef->GetArgs(1))
            .arg(spotDef->GetArgs(2)).arg(spotDef->GetArgs(4)));

        if(mMainWindow)
        {
            auto zoneWindow = mMainWindow->GetZones();
            if(zoneWindow)
            {
                auto merged = zoneWindow->GetMergedZone();
                if(merged)
                {
                    spot = merged->Definition->GetSpots(spotDef->GetID());
                }
            }
        }
    }
    else if(spot)
    {
        // Server only definition
        prop->id->setText(QString::number(spot->GetID()));
        prop->x->setText("N/A");
        prop->y->setText("N/A");
        prop->rotation->setText("N/A");
        prop->width->setText("N/A");
        prop->height->setText("N/A");
        prop->type->setCurrentIndex(0);
        prop->chkEnabled->setChecked(false);
        prop->lblArguments->setText("No client arguments");
    }

    std::shared_ptr<objects::SpawnLocation> spawnArea;
    if(spot)
    {
        prop->grpServerDefinition->setChecked(true);

        auto actions = spot->GetActions();
        prop->actions->Load(actions);

        actions = spot->GetLeaveActions();
        prop->leaveActions->Load(actions);

        spawnArea = spot->GetSpawnArea();

        prop->matchSpawn->setCurrentIndex(to_underlying(spot
            ->GetMatchSpawn()));
        prop->matchBase->setValue((int32_t)spot->GetMatchBase());
        prop->matchZoneInLimit->setValue((int32_t)spot->GetMatchZoneInLimit());
    }
    else
    {
        prop->grpServerDefinition->setChecked(false);

        std::list<std::shared_ptr<objects::Action>> actions;
        prop->actions->Load(actions);
        prop->leaveActions->Load(actions);

        prop->matchSpawn->setCurrentIndex(0);
        prop->matchBase->setValue(0);
        prop->matchZoneInLimit->setValue(0);
    }

    if(spawnArea)
    {
        prop->grpSpawnArea->setChecked(true);
        prop->spawnArea->Load(spawnArea);
    }
    else
    {
        prop->grpSpawnArea->setChecked(false);
        prop->spawnArea->Load(nullptr);
    }
}

void SpotList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    (void)obj;
}
