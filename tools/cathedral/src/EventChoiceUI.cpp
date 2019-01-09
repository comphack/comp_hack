/**
 * @file tools/cathedral/src/EventChoiceUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event choice.
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

#include "EventChoiceUI.h"

// Cathedral Includes
#include "DynamicList.h"
#include "EventMessageRef.h"
#include "MainWindow.h"
#include "ServerScript.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_EventBase.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventChoice::EventChoice(MainWindow *pMainWindow, bool isITime,
    QWidget *pParent) : EventBase(pMainWindow, pParent), mMessage(0),
    mBranches(0), mBranchScript(0)
{
    mMessage = new EventMessageRef;
    mBranchGroup = new QGroupBox;
    mBranches = new DynamicList;
    mBranchScript = new ServerScript;

    mMessage->Setup(pMainWindow, isITime
        ? "CHouraiMessageData" : "CEventMessageData");

    mBranches->Setup(DynamicItemType_t::OBJ_EVENT_BASE, pMainWindow);
    mBranches->SetAddText("Add Branch");

    ui->formCore->insertRow(0, "Message:", mMessage);

    QVBoxLayout* branchGroupLayout = new QVBoxLayout;

    branchGroupLayout->addWidget(mBranchScript);
    branchGroupLayout->addWidget(mBranches);

    mBranchGroup->setLayout(branchGroupLayout);

    mBranchGroup->setTitle("Branches");

    ui->layoutBranch->addWidget(mBranchGroup);
}

EventChoice::~EventChoice()
{
    mMessage->deleteLater();
    mBranchGroup->deleteLater();
    mBranches->deleteLater();
    mBranchScript->deleteLater();
}

void EventChoice::Load(const std::shared_ptr<objects::EventChoice>& e)
{
    EventBase::Load(e);

    if(!mEventBase)
    {
        return;
    }

    mMessage->SetValue((uint32_t)e->GetMessageID());

    for(auto branch : e->GetBranches())
    {
        mBranches->AddObject(branch);
    }

    mBranchScript->SetScriptID(e->GetBranchScriptID());

    auto params = e->GetBranchScriptParams();
    mBranchScript->SetParams(params);
}

std::shared_ptr<objects::EventChoice> EventChoice::Save() const
{
    if(!mEventBase)
    {
        return nullptr;
    }

    EventBase::Save();

    auto choice = std::dynamic_pointer_cast<objects::EventChoice>(mEventBase);
    choice->SetMessageID((int32_t)mMessage->GetValue());

    auto branches = mBranches->GetObjectList<objects::EventBase>();
    choice->SetBranches(branches);
    
    choice->SetBranchScriptID(mBranchScript->GetScriptID());
    choice->ClearBranchScriptParams();
    if(!choice->GetBranchScriptID().IsEmpty())
    {
        // Ignore params if no script is set
        auto params = mBranchScript->GetParams();
        choice->SetBranchScriptParams(params);
    }

    return choice;
}
