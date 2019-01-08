/**
 * @file tools/cathedral/src/SpawnLocationGroupList.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a control that holds a list of spawn location groups.
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

#include "SpawnLocationGroupList.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "MainWindow.h"
#include "ZoneWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_ObjectList.h"
#include "ui_SpawnLocationGroup.h"
#include <PopIgnore.h>

// objects Includes
#include <SpawnLocationGroup.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

SpawnLocationGroupList::SpawnLocationGroupList(QWidget *pParent) : ObjectList(pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::SpawnLocationGroup;
    prop->setupUi(pWidget);

    ui->splitter->addWidget(pWidget);
}

SpawnLocationGroupList::~SpawnLocationGroupList()
{
    delete prop;
}

void SpawnLocationGroupList::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;

    prop->groups->Setup(DynamicItemType_t::COMPLEX_OBJECT_SELECTOR,
        pMainWindow, "SpawnGroup", true);
    prop->spots->Setup(DynamicItemType_t::COMPLEX_SPOT, pMainWindow);
    prop->locations->Setup(DynamicItemType_t::OBJ_SPAWN_LOCATION, pMainWindow);
}

QString SpawnLocationGroupList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto slg = std::dynamic_pointer_cast<objects::SpawnLocationGroup>(obj);

    if(!slg)
    {
        return {};
    }

    return QString::number(slg->GetID());
}

QString SpawnLocationGroupList::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto slg = std::dynamic_pointer_cast<objects::SpawnLocationGroup>(obj);
    if(mMainWindow && slg)
    {
        auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("SpawnLocationGroup"));
        return dataset ? qs(dataset->GetName(slg)) : "";
    }

    return {};
}

void SpawnLocationGroupList::LoadProperties(
    const std::shared_ptr<libcomp::Object>& obj)
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

    prop->groups->Clear();
    prop->locations->Clear();
    prop->spots->Clear();

    auto slg = std::dynamic_pointer_cast<objects::SpawnLocationGroup>(obj);
    if(slg)
    {
        prop->slgID->setText(QString::number(slg->GetID()));

        for(uint32_t groupID : slg->GetGroupIDs())
        {
            prop->groups->AddUnsignedInteger(groupID);
        }

        prop->respawnTime->setValue((double)slg->GetRespawnTime());
        prop->immediateSpawn->setChecked(slg->GetImmediateSpawn());

        for(uint32_t spotID : slg->GetSpotIDs())
        {
            prop->spots->AddUnsignedInteger(spotID);
        }

        prop->spotSelection->setCurrentIndex(to_underlying(slg
            ->GetSpotSelection()));
        
        for(auto loc : slg->GetLocations())
        {
            prop->locations->AddObject(loc);
        }
    }
    else
    {
        prop->slgID->setText("");
    }
}

void SpawnLocationGroupList::SaveProperties(const std::shared_ptr<
    libcomp::Object>& obj)
{
    auto slg = std::dynamic_pointer_cast<objects::SpawnLocationGroup>(obj);
    if(slg)
    {
        auto groupIDs = prop->groups->GetUnsignedIntegerList();
        slg->SetGroupIDs(groupIDs);

        slg->SetRespawnTime((float)prop->respawnTime->value());
        slg->SetImmediateSpawn(prop->immediateSpawn->isChecked());

        slg->ClearSpotIDs();
        for(uint32_t spotID : prop->spots->GetUnsignedIntegerList())
        {
            slg->InsertSpotIDs(spotID);
        }

        slg->SetSpotSelection((objects::SpawnLocationGroup::SpotSelection_t)
            prop->spotSelection->currentIndex());

        auto locs = prop->locations->GetObjectList<objects::SpawnLocation>();
        slg->SetLocations(locs);
    }
}
