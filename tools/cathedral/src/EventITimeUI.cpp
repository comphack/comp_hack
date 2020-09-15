/**
 * @file tools/cathedral/src/EventITimeUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an I-Time event.
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

#include "EventITimeUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Event.h"
#include "ui_EventITime.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventITime::EventITime(MainWindow *pMainWindow, QWidget *pParent)
    : Event(pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::EventITime;
  prop->setupUi(pWidget);

  prop->giftIDs->Setup(DynamicItemType_t::PRIMITIVE_UINT, pMainWindow);
  prop->giftIDs->SetAddText("Add Gift");

  ui->eventTitle->setText(tr("<b>I-Time</b>"));
  ui->layoutMain->addWidget(pWidget);

  prop->choices->Setup(DynamicItemType_t::OBJ_EVENT_ITIME_CHOICE, pMainWindow);
  prop->choices->SetAddText("Add Choice");

  prop->iTimeID->BindSelector(pMainWindow, "CHouraiData");
  prop->message->Setup(pMainWindow, "CHouraiMessageData");

  prop->startActions->SetMainWindow(pMainWindow);
}

EventITime::~EventITime() { delete prop; }

void EventITime::Load(const std::shared_ptr<objects::Event> &e) {
  Event::Load(e);

  mEvent = std::dynamic_pointer_cast<objects::EventITime>(e);

  if (!mEvent) {
    return;
  }

  prop->message->SetValue((uint32_t)mEvent->GetMessageID());

  for (auto choice : mEvent->GetChoices()) {
    prop->choices->AddObject(choice);
  }

  prop->iTimeID->SetValue((uint32_t)mEvent->GetITimeID());
  prop->reactionID->setValue(mEvent->GetReactionID());
  prop->timeLimit->setValue(mEvent->GetTimeLimit());

  for (uint32_t giftID : mEvent->GetGiftIDs()) {
    prop->giftIDs->AddUnsignedInteger(giftID);
  }

  prop->startActions->SetEvent(mEvent->GetStartActions());
}

std::shared_ptr<objects::Event> EventITime::Save() const {
  if (!mEvent) {
    return nullptr;
  }

  Event::Save();

  mEvent->SetMessageID((int32_t)prop->message->GetValue());

  auto choices = prop->choices->GetObjectList<objects::EventChoice>();
  mEvent->SetChoices(choices);

  mEvent->SetITimeID((int8_t)prop->iTimeID->GetValue());
  mEvent->SetReactionID(prop->reactionID->value());
  mEvent->SetTimeLimit((int16_t)prop->timeLimit->value());

  auto giftIDs = prop->giftIDs->GetUnsignedIntegerList();
  mEvent->SetGiftIDs(giftIDs);
  mEvent->SetStartActions(prop->startActions->GetEvent());

  return mEvent;
}
