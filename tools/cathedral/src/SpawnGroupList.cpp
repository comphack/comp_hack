/**
 * @file tools/cathedral/src/SpawnGroupList.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a control that holds a list of spawn groups.
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

#include "SpawnGroupList.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "MainWindow.h"
#include "ZoneWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_ObjectList.h"
#include "ui_SpawnGroup.h"
#include <PopIgnore.h>

// objects Includes
#include <SpawnGroup.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

SpawnGroupList::SpawnGroupList(QWidget *pParent) : ObjectList(pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::SpawnGroup;
    prop->setupUi(pWidget);

    prop->spawns->SetValueName(tr("Count:"));
    prop->spawns->SetMinMax(0, 65535);

    ui->splitter->addWidget(pWidget);
}

SpawnGroupList::~SpawnGroupList()
{
    delete prop;
}

void SpawnGroupList::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;

    prop->spawns->BindSelector(pMainWindow, "Spawn");
    prop->spawnActions->SetMainWindow(pMainWindow);
    prop->defeatActions->SetMainWindow(pMainWindow);
}

QString SpawnGroupList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto sg = std::dynamic_pointer_cast<objects::SpawnGroup>(obj);

    if(!sg)
    {
        return {};
    }

    return QString::number(sg->GetID());
}

QString SpawnGroupList::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto sg = std::dynamic_pointer_cast<objects::SpawnGroup>(obj);
    if(mMainWindow && sg)
    {
        auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("SpawnGroup"));
        return dataset ? qs(dataset->GetName(sg)) : "";
    }

    return {};
}

void SpawnGroupList::LoadProperties(const std::shared_ptr<libcomp::Object>& obj)
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

    auto sg = std::dynamic_pointer_cast<objects::SpawnGroup>(obj);
    if(sg)
    {
        prop->groupID->setText(QString::number(sg->GetID()));

        std::unordered_map<uint32_t, int32_t> spawns;
        for(auto& pair : sg->GetSpawns())
        {
            spawns[pair.first] = (int32_t)pair.second;
        }

        prop->spawns->Load(spawns);

        auto restrict = sg->GetRestrictions();
        prop->grpRestrictions->setChecked(restrict != nullptr);
        prop->restrictions->Load(restrict);

        auto actions = sg->GetSpawnActions();
        prop->spawnActions->Load(actions);

        actions = sg->GetDefeatActions();
        prop->defeatActions->Load(actions);
    }
    else
    {
        prop->groupID->setText("");
    }
}

void SpawnGroupList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto sg = std::dynamic_pointer_cast<objects::SpawnGroup>(obj);
    if(sg)
    {
        sg->ClearSpawns();
        for(auto& pair : prop->spawns->SaveUnsigned())
        {
            sg->SetSpawns(pair.first, (uint16_t)pair.second);
        }

        if(prop->grpRestrictions->isChecked())
        {
            sg->SetRestrictions(prop->restrictions->Save());
        }
        else
        {
            sg->SetRestrictions(nullptr);
        }

        auto actions = prop->spawnActions->Save();
        sg->SetSpawnActions(actions);

        actions = prop->defeatActions->Save();
        sg->SetDefeatActions(actions);
    }
}
