/**
 * @file tools/cathedral/src/ActionAddRemoveItemsUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an add/remove items action.
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

#include "ActionAddRemoveItemsUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionAddRemoveItems.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionAddRemoveItems::ActionAddRemoveItems(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionAddRemoveItems;
    prop->setupUi(pWidget);
    prop->items->SetValueName(tr("Qty:"));

    ui->actionTitle->setText(tr("<b>Add/Remove Items</b>"));
    ui->actionLayout->insertWidget(2, pWidget);
}

ActionAddRemoveItems::~ActionAddRemoveItems()
{
    delete prop;
}

void ActionAddRemoveItems::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionAddRemoveItems>(act);

    if(!mAction)
    {
        return;
    }

    prop->sourceContext->setCurrentIndex(to_underlying(
        mAction->GetSourceContext()));
    prop->location->setCurrentIndex(to_underlying(
        mAction->GetLocation()));
    prop->notify->setChecked(mAction->GetNotify());
    prop->stopOnFailure->setChecked(mAction->GetStopOnFailure());
    prop->onFailureEvent->lineEdit()->setText(
        qs(mAction->GetOnFailureEvent()));
    prop->fromDropSet->setChecked(mAction->GetFromDropSet());
    prop->items->Load(mAction->GetItems());
    prop->mode->setCurrentIndex(to_underlying(
        mAction->GetMode()));
}

std::shared_ptr<objects::Action> ActionAddRemoveItems::Save() const
{
    return mAction;
}
