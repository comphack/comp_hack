/**
 * @file tools/cathedral/src/ActionUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an action.
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

#include "ActionUI.h"

// Cathedral Includes
#include "ActionList.h"
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_Action.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

Action::Action(ActionList *pList, MainWindow *pMainWindow, QWidget *pParent) :
    QWidget(pParent), mList(pList), mMainWindow(pMainWindow)
{
    ui = new Ui::Action;
    ui->setupUi(this);

    ui->layoutBaseBody->setVisible(false);

    ui->failureEvent->SetMainWindow(pMainWindow);

    connect(ui->remove, SIGNAL(clicked(bool)), this, SLOT(Remove()));
    connect(ui->up, SIGNAL(clicked(bool)), this, SLOT(MoveUp()));
    connect(ui->down, SIGNAL(clicked(bool)), this, SLOT(MoveDown()));
    connect(ui->toggleBaseDisplay, SIGNAL(clicked(bool)), this,
        SLOT(ToggleBaseDisplay()));
}

Action::~Action()
{
    delete ui;
}

void Action::UpdatePosition(bool isFirst, bool isLast)
{
    ui->up->setEnabled(!isFirst);
    ui->down->setEnabled(!isLast);
    ui->actionListDiv->setVisible(!isLast);
}

void Action::Remove()
{
    if(mList)
    {
        mList->RemoveAction(this);
    }
}

void Action::MoveUp()
{
    if(mList)
    {
        mList->MoveUp(this);
    }
}

void Action::MoveDown()
{
    if(mList)
    {
        mList->MoveDown(this);
    }
}

void Action::ToggleBaseDisplay()
{
    if(ui->layoutBaseBody->isVisible())
    {
        ui->layoutBaseBody->setVisible(false);
        ui->toggleBaseDisplay->setText(u8"►");
    }
    else
    {
        ui->layoutBaseBody->setVisible(true);
        ui->toggleBaseDisplay->setText(u8"▼");
    }
}

void Action::LoadBaseProperties(const std::shared_ptr<objects::Action>& action)
{
    ui->sourceContext->setCurrentIndex(to_underlying(
        action->GetSourceContext()));
    ui->location->setCurrentIndex(to_underlying(
        action->GetLocation()));
    ui->stopOnFailure->setChecked(action->GetStopOnFailure());
    ui->failureEvent->SetEvent(action->GetOnFailureEvent());
    ui->transformScript->SetScriptID(action->GetTransformScriptID());
    ui->transformScript->SetParams(action->GetTransformScriptParams());

    // If any non-base values are set, display the base values section
    if(!ui->layoutBaseBody->isVisible() &&
        (action->GetSourceContext() != objects::Action::SourceContext_t::SOURCE ||
            action->GetLocation() != objects::Action::Location_t::ZONE ||
            !action->GetStopOnFailure() ||
            !action->GetOnFailureEvent().IsEmpty() ||
            !action->GetTransformScriptID().IsEmpty()))
    {
        ToggleBaseDisplay();
    }
}

void Action::SaveBaseProperties(const std::shared_ptr<
    objects::Action>& action) const
{
    action->SetSourceContext((objects::Action::SourceContext_t)ui
        ->sourceContext->currentIndex());
    action->SetLocation((objects::Action::Location_t)ui->location
        ->currentIndex());
    action->SetStopOnFailure(ui->stopOnFailure->isChecked());
    action->SetOnFailureEvent(ui->failureEvent->GetEvent());
    action->SetTransformScriptID(ui->transformScript->GetScriptID());

    action->ClearTransformScriptParams();
    if(!action->GetTransformScriptID().IsEmpty())
    {
        // Ignore params if no script is set
        action->SetTransformScriptParams(ui->transformScript->GetParams());
    }
}
