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
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_EventBase.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventChoice::EventChoice(MainWindow *pMainWindow, QWidget *pParent) :
    EventBase(pMainWindow, pParent), mMessage(0), mBranches(0)
{
    mMessage = new QSpinBox;
    mMessage->setMaximum(2147483647);
    mMessage->setMinimum(-2147483647);

    mBranches = new DynamicList;

    mBranches->Setup(DynamicItemType_t::OBJ_EVENT_BASE, pMainWindow);

    ui->formCore->insertRow(0, "Message", mMessage);
    ui->formBranch->addRow("Branches:", mBranches);
}

EventChoice::~EventChoice()
{
}

void EventChoice::Load(const std::shared_ptr<objects::EventChoice>& e)
{
    EventBase::Load(e);

    mEventBase = e;

    if(!mEventBase)
    {
        return;
    }

    mMessage->setValue(e->GetMessageID());

    for(auto branch : e->GetBranches())
    {
        mBranches->AddObject(branch);
    }
}

std::shared_ptr<objects::EventChoice> EventChoice::Save() const
{
    return nullptr;
}
