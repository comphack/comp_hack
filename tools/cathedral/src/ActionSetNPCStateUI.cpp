/**
 * @file tools/cathedral/src/ActionSetNPCStateUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a start event action.
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

#include "ActionSetNPCStateUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionSetNPCState.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionSetNPCState::ActionSetNPCState(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionSetNPCState;
    prop->setupUi(pWidget);

    ui->actionTitle->setText(tr("<b>Set NPC State</b>"));
    ui->layoutMain->addWidget(pWidget);

    prop->actor->BindSelector(pMainWindow, "Actor", true);
}

ActionSetNPCState::~ActionSetNPCState()
{
    delete prop;
}

void ActionSetNPCState::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionSetNPCState>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->state->setValue(mAction->GetState());
    prop->from->setValue(mAction->GetFrom());
    prop->actor->SetValue((uint32_t)mAction->GetActorID());
    prop->sourceClientOnly->setChecked(mAction->GetSourceClientOnly());
}

std::shared_ptr<objects::Action> ActionSetNPCState::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetState((uint8_t)prop->state->value());
    mAction->SetFrom((int16_t)prop->from->value());
    mAction->SetActorID((int32_t)prop->actor->GetValue());
    mAction->SetSourceClientOnly(prop->sourceClientOnly->isChecked());

    return mAction;
}
