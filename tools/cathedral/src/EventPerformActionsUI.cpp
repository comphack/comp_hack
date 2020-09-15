/**
 * @file tools/cathedral/src/EventPerformActionsUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a perform actions event.
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

#include "EventPerformActionsUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Event.h"
#include "ui_EventPerformActions.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventPerformActions::EventPerformActions(MainWindow *pMainWindow,
                                         QWidget *pParent)
    : Event(pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::EventPerformActions;
  prop->setupUi(pWidget);

  ui->eventTitle->setText(tr("<b>Perform Actions</b>"));
  ui->layoutMain->addWidget(pWidget);

  prop->actions->SetMainWindow(pMainWindow);
}

EventPerformActions::~EventPerformActions() { delete prop; }

void EventPerformActions::Load(const std::shared_ptr<objects::Event> &e) {
  Event::Load(e);

  mEvent = std::dynamic_pointer_cast<objects::EventPerformActions>(e);

  if (!mEvent) {
    return;
  }

  auto actions = mEvent->GetActions();
  prop->actions->Load(actions);
}

std::shared_ptr<objects::Event> EventPerformActions::Save() const {
  if (!mEvent) {
    return nullptr;
  }

  Event::Save();

  auto actions = prop->actions->Save();
  mEvent->SetActions(actions);

  return mEvent;
}
