/**
 * @file tools/cathedral/src/ActionCreateLootUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a create loot action.
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

#include "ActionCreateLootUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionCreateLoot.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionCreateLoot::ActionCreateLoot(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionCreateLoot;
    prop->setupUi(pWidget);

    ui->actionTitle->setText(tr("<b>Create Loot</b>"));
    ui->layoutMain->addWidget(pWidget);
}

ActionCreateLoot::~ActionCreateLoot()
{
    delete prop;
}

void ActionCreateLoot::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionCreateLoot>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->dropSetIDs->Load(mAction->GetDropSetIDs());
    prop->isBossBox->setChecked(mAction->GetIsBossBox());
    prop->expirationTime->setValue(mAction->GetExpirationTime());
    prop->position->setCurrentIndex(to_underlying(
        mAction->GetPosition()));
}

std::shared_ptr<objects::Action> ActionCreateLoot::Save() const
{
    SaveBaseProperties(mAction);

    return mAction;
}
