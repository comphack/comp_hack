/**
 * @file tools/cathedral/src/EventPromptUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a prompt event.
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

#include "EventPromptUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QLineEdit>

#include "ui_Event.h"
#include "ui_EventPrompt.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventPrompt::EventPrompt(MainWindow *pMainWindow, QWidget *pParent)
    : Event(pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::EventPrompt;
  prop->setupUi(pWidget);

  // Normal next paths do not apply to prompts
  ui->lblNext->hide();
  ui->next->hide();
  ui->lblQueueNext->hide();
  ui->queueNext->hide();
  ui->grpBranches->hide();

  ui->eventTitle->setText(tr("<b>Prompt</b>"));
  ui->layoutMain->addWidget(pWidget);

  prop->choices->Setup(DynamicItemType_t::OBJ_EVENT_CHOICE, pMainWindow);
  prop->choices->SetAddText("Add Choice");

  prop->message->Setup(pMainWindow);
}

EventPrompt::~EventPrompt() { delete prop; }

void EventPrompt::Load(const std::shared_ptr<objects::Event> &e) {
  Event::Load(e);

  mEvent = std::dynamic_pointer_cast<objects::EventPrompt>(e);

  if (!mEvent) {
    return;
  }

  prop->message->SetValue((uint32_t)mEvent->GetMessageID());

  for (auto choice : mEvent->GetChoices()) {
    prop->choices->AddObject(choice);
  }
}

std::shared_ptr<objects::Event> EventPrompt::Save() const {
  if (!mEvent) {
    return nullptr;
  }

  Event::Save();

  mEvent->SetMessageID((int32_t)prop->message->GetValue());

  auto choices = prop->choices->GetObjectList<objects::EventChoice>();
  mEvent->SetChoices(choices);

  return mEvent;
}
