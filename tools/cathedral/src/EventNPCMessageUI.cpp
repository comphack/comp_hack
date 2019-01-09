/**
 * @file tools/cathedral/src/EventNPCMessageUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an NPC message event.
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

#include "EventNPCMessageUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Event.h"
#include "ui_EventNPCMessage.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventNPCMessage::EventNPCMessage(MainWindow *pMainWindow, QWidget *pParent)
    : Event(pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::EventNPCMessage;
    prop->setupUi(pWidget);

    prop->messages->Setup(DynamicItemType_t::COMPLEX_EVENT_MESSAGE,
        pMainWindow);
    prop->messages->SetAddText("Add Message");

    ui->eventTitle->setText(tr("<b>NPC Message</b>"));
    ui->layoutMain->addWidget(pWidget);
}

EventNPCMessage::~EventNPCMessage()
{
    delete prop;
}

void EventNPCMessage::Load(const std::shared_ptr<objects::Event>& e)
{
    Event::Load(e);

    mEvent = std::dynamic_pointer_cast<objects::EventNPCMessage>(e);

    if(!mEvent)
    {
        return;
    }

    for(int32_t messageID : mEvent->GetMessageIDs())
    {
        prop->messages->AddInteger(messageID);
    }
}

std::shared_ptr<objects::Event> EventNPCMessage::Save() const
{
    if(!mEvent)
    {
        return nullptr;
    }

    Event::Save();

    auto messageIDs = prop->messages->GetIntegerList();
    mEvent->SetMessageIDs(messageIDs);

    return mEvent;
}
