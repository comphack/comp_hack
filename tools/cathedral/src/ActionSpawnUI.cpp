/**
 * @file tools/cathedral/src/ActionSpawnUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a spawn action.
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

#include "ActionSpawnUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionSpawn.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionSpawn::ActionSpawn(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionSpawn;
    prop->setupUi(pWidget);

    prop->spawnGroups->SetValueName(tr("Spot ID:"));
    prop->spawnGroups->BindSelector(pMainWindow, "SpawnGroup");

    prop->spawnLocationGroups->Setup(
        DynamicItemType_t::COMPLEX_OBJECT_SELECTOR, pMainWindow,
        "SpawnLocationGroup");
    prop->spot->SetMainWindow(pMainWindow);
    prop->defeatActions->SetMainWindow(pMainWindow);

    ui->actionTitle->setText(tr("<b>Spawn</b>"));
    ui->layoutMain->addWidget(pWidget);
}

ActionSpawn::~ActionSpawn()
{
    delete prop;
}

void ActionSpawn::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionSpawn>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);
    
    for(uint32_t slgID : mAction->GetSpawnLocationGroupIDs())
    {
        prop->spawnLocationGroups->AddUnsignedInteger(slgID);
    }

    prop->spot->SetValue(mAction->GetSpotID());

    std::unordered_map<uint32_t, int32_t> spawnGroups;

    for(auto id : mAction->GetSpawnGroupIDs())
    {
        spawnGroups[id.first] = (int32_t)id.second;
    }

    prop->spawnGroups->Load(spawnGroups);

    prop->mode->setCurrentIndex(to_underlying(
        mAction->GetMode()));

    auto defeatActions = mAction->GetDefeatActions();
    prop->defeatActions->Load(defeatActions);
    prop->noStagger->setChecked(mAction->GetNoStagger());
}

std::shared_ptr<objects::Action> ActionSpawn::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    auto slgIDs = prop->spawnLocationGroups->GetUnsignedIntegerList();
    mAction->SetSpawnLocationGroupIDs(slgIDs);

    mAction->SetSpotID(prop->spot->GetValue());

    mAction->ClearSpawnGroupIDs();
    for(auto pair : prop->spawnGroups->SaveUnsigned())
    {
        mAction->SetSpawnGroupIDs(pair.first, (uint32_t)pair.second);
    }

    mAction->SetMode((objects::ActionSpawn::Mode_t)prop->mode->currentIndex());

    auto defeatActions = prop->defeatActions->Save();
    mAction->SetDefeatActions(defeatActions);
    mAction->SetNoStagger(prop->noStagger->isChecked());

    return mAction;
}
