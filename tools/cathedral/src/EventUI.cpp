/**
 * @file tools/cathedral/src/EventUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event.
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

#include "EventUI.h"

// Cathedral Includes
#include "EventWindow.h"
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_Event.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

Event::Event(MainWindow *pMainWindow, QWidget *pParent) :
    QWidget(pParent), mMainWindow(pMainWindow)
{
    ui = new Ui::Event;
    ui->setupUi(this);

    ui->eventTitle->setText(tr("<b>Fork</b>"));

    ui->branches->Setup(DynamicItemType_t::OBJ_EVENT_BASE, pMainWindow);
    ui->branches->SetAddText("Add Branch");

    ui->conditions->Setup(DynamicItemType_t::OBJ_EVENT_CONDITION, pMainWindow);
    ui->conditions->SetAddText("Add Condition");

    ui->comments->Setup(DynamicItemType_t::PRIMITIVE_MULTILINE_STRING,
        pMainWindow);
    ui->comments->SetAddText("Add Comment");

    ui->layoutBaseBody->setVisible(false);

    ui->next->SetMainWindow(pMainWindow);
    ui->queueNext->SetMainWindow(pMainWindow);

    connect(ui->changeEventID, SIGNAL(clicked(bool)), this,
        SLOT(ChangeEventID()));
    connect(ui->toggleBaseDisplay, SIGNAL(clicked(bool)), this,
        SLOT(ToggleBaseDisplay()));
}

Event::~Event()
{
    delete ui;
}

void Event::Load(const std::shared_ptr<objects::Event>& e)
{
    mEventBase = e;

    if(!mEventBase)
    {
        return;
    }

    ui->eventID->setText(qs(e->GetID()));
    ui->next->SetEvent(e->GetNext());
    ui->queueNext->SetEvent(e->GetQueueNext());
    ui->pop->setChecked(e->GetPop());
    ui->popNext->setChecked(e->GetPopNext());
    ui->skipInvalid->setChecked(e->GetSkipInvalid());
    ui->branchScript->SetScriptID(e->GetBranchScriptID());
    ui->transformScript->SetScriptID(e->GetTransformScriptID());

    auto params = e->GetBranchScriptParams();
    ui->branchScript->SetParams(params);

    params = e->GetTransformScriptParams();
    ui->transformScript->SetParams(params);

    for(auto branch : e->GetBranches())
    {
        ui->branches->AddObject(branch);
    }

    for(auto condition : e->GetConditions())
    {
        ui->conditions->AddObject(condition);
    }

    // If any non-base values are set, display the base values section
    if(!ui->layoutBaseBody->isVisible() &&
        (!e->GetQueueNext().IsEmpty() ||
            e->GetPop() ||
            e->GetPopNext() ||
            e->GetSkipInvalid() ||
            !e->GetTransformScriptID().IsEmpty()))
    {
        ToggleBaseDisplay();
    }
}

std::shared_ptr<objects::Event> Event::Save() const
{
    if(!mEventBase)
    {
        return nullptr;
    }

    mEventBase->SetID(cs(ui->eventID->text()));
    mEventBase->SetNext(ui->next->GetEvent());
    mEventBase->SetQueueNext(ui->queueNext->GetEvent());
    mEventBase->SetPop(ui->pop->isChecked());
    mEventBase->SetPopNext(ui->popNext->isChecked());
    mEventBase->SetSkipInvalid(ui->skipInvalid->isChecked());

    mEventBase->SetBranchScriptID(ui->branchScript->GetScriptID());
    mEventBase->SetTransformScriptID(ui->transformScript->GetScriptID());

    // Ignore params if no script is set
    mEventBase->ClearBranchScriptParams();
    if(!mEventBase->GetBranchScriptID().IsEmpty())
    {
        auto params = ui->branchScript->GetParams();
        mEventBase->SetBranchScriptParams(params);
    }

    mEventBase->ClearTransformScriptParams();
    if(!mEventBase->GetTransformScriptID().IsEmpty())
    {
        auto params = ui->transformScript->GetParams();
        mEventBase->SetTransformScriptParams(params);
    }

    auto branches = ui->branches->GetObjectList<objects::EventBase>();
    mEventBase->SetBranches(branches);

    auto conditions = ui->conditions->GetObjectList<objects::EventCondition>();
    mEventBase->SetConditions(conditions);

    return mEventBase;
}

void Event::SetComments(const std::list<libcomp::String>& comments)
{
    ui->comments->Clear();
    for(auto comment : comments)
    {
        ui->comments->AddString(comment);
    }

    if(comments.size() > 0 && !ui->layoutBaseBody->isVisible())
    {
        ToggleBaseDisplay();
    }
}

std::list<libcomp::String> Event::GetComments()
{
    return ui->comments->GetStringList();
}

void Event::ChangeEventID()
{
    if(mMainWindow && mEventBase)
    {
        auto eventWindow = mMainWindow->GetEvents();
        eventWindow->ChangeEventID(mEventBase->GetID());
    }
}

void Event::ToggleBaseDisplay()
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
