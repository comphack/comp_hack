/**
 * @file tools/cathedral/src/EventExNPCMessageUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an EX-NPC message event.
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

#include "EventExNPCMessageUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QLineEdit>

#include "ui_Event.h"
#include "ui_EventExNPCMessage.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventExNPCMessage::EventExNPCMessage(MainWindow *pMainWindow, QWidget *pParent)
    : Event(pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::EventExNPCMessage;
  prop->setupUi(pWidget);

  ui->eventTitle->setText(tr("<b>EX-NPC Message</b>"));
  ui->layoutMain->addWidget(pWidget);

  prop->message->Setup(pMainWindow);
}

EventExNPCMessage::~EventExNPCMessage() { delete prop; }

void EventExNPCMessage::Load(const std::shared_ptr<objects::Event> &e) {
  Event::Load(e);

  mEvent = std::dynamic_pointer_cast<objects::EventExNPCMessage>(e);

  if (!mEvent) {
    return;
  }

  prop->message->SetValue((uint32_t)mEvent->GetMessageID());
  prop->messageValue->setValue(mEvent->GetMessageValue());
}

std::shared_ptr<objects::Event> EventExNPCMessage::Save() const {
  if (!mEvent) {
    return nullptr;
  }

  Event::Save();

  mEvent->SetMessageID((int32_t)prop->message->GetValue());
  mEvent->SetMessageValue(prop->messageValue->value());

  return mEvent;
}
