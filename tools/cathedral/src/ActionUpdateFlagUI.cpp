/**
 * @file tools/cathedral/src/ActionUpdateFlagUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an update flag action.
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

#include "ActionUpdateFlagUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionUpdateFlag.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionUpdateFlag::ActionUpdateFlag(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionUpdateFlag;
    prop->setupUi(pWidget);

    ui->actionTitle->setText(tr("<b>Update Flag</b>"));
    ui->layoutMain->addWidget(pWidget);
}

ActionUpdateFlag::~ActionUpdateFlag()
{
    delete prop;
}

void ActionUpdateFlag::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionUpdateFlag>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->flagType->setCurrentIndex(to_underlying(
        mAction->GetFlagType()));
    prop->id->setValue(mAction->GetID());
    prop->remove->setChecked(mAction->GetRemove());
}

std::shared_ptr<objects::Action> ActionUpdateFlag::Save() const
{
    SaveBaseProperties(mAction);

    return mAction;
}
