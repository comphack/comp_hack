/**
 * @file tools/cathedral/src/EventBaseUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event base object.
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

#include "EventBaseUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_EventBase.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventBase::EventBase(MainWindow *pMainWindow, QWidget *pParent) :
    QWidget(pParent), mMainWindow(pMainWindow)
{
    ui = new Ui::EventBase;
    ui->setupUi(this);

    ui->conditions->Setup(DynamicItemType_t::OBJ_EVENT_CONDITION, pMainWindow);
    ui->conditions->SetAddText("Add Condition");

    ui->layoutBaseBody->setVisible(false);

    // Hide skip invalid by default
    ui->lblSkipInvalid->setVisible(false);
    ui->skipInvalid->setVisible(false);

    ui->next->SetMainWindow(pMainWindow);
    ui->queueNext->SetMainWindow(pMainWindow);

    connect(ui->toggleBaseDisplay, SIGNAL(clicked(bool)), this,
        SLOT(ToggleBaseDisplay()));
}

EventBase::~EventBase()
{
    delete ui;
}

void EventBase::Load(const std::shared_ptr<objects::EventBase>& e)
{
    mEventBase = e;

    if(!mEventBase)
    {
        return;
    }

    ui->next->SetEvent(e->GetNext());
    ui->queueNext->SetEvent(e->GetQueueNext());
    ui->pop->setChecked(e->GetPop());
    ui->popNext->setChecked(e->GetPopNext());

    for(auto condition : e->GetConditions())
    {
        ui->conditions->AddObject(condition);
    }

    // If any non-base values are set, display the base values section
    // (skip invalid is assumed to have already been set)
    if(!ui->layoutBaseBody->isVisible() &&
        (!e->GetQueueNext().IsEmpty() ||
            e->GetPop() ||
            e->GetPopNext() ||
            ui->skipInvalid->isChecked()))
    {
        ToggleBaseDisplay();
    }
}

std::shared_ptr<objects::EventBase> EventBase::Save() const
{
    if(!mEventBase)
    {
        return nullptr;
    }

    mEventBase->SetNext(ui->next->GetEvent());
    mEventBase->SetQueueNext(ui->queueNext->GetEvent());
    mEventBase->SetPop(ui->pop->isChecked());
    mEventBase->SetPopNext(ui->popNext->isChecked());

    auto conditions = ui->conditions->GetObjectList<objects::EventCondition>();
    mEventBase->SetConditions(conditions);

    return mEventBase;
}

void EventBase::ToggleBaseDisplay()
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

bool EventBase::GetSkipInvalid() const
{
    return ui->skipInvalid->isChecked();
}

void EventBase::SetSkipInvalid(bool skip)
{
    // Since we are using the property, show it
    ui->lblSkipInvalid->setVisible(true);
    ui->skipInvalid->setVisible(true);

    ui->skipInvalid->setChecked(skip);
}
