/**
 * @file tools/cathedral/src/EventRef.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event being referenced from all known events.
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

#include "EventRef.h"

// Cathedral Includes
#include "EventWindow.h"
#include "MainWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QLineEdit>

#include "ui_EventRef.h"

std::list<libcomp::String> EventRef::sAllEventIDs;

EventRef::EventRef(QWidget* pParent) : QWidget(pParent) {
  ui = new Ui::EventRef;
  ui->setupUi(this);

  for (auto& eventID : sAllEventIDs) {
    ui->eventID->addItem(qs(eventID));
  }

  // Make sure nothing is selected by default
  ui->eventID->setCurrentText("");

  connect(ui->go, SIGNAL(clicked(bool)), this, SLOT(Go()));
}

EventRef::~EventRef() { delete ui; }

void EventRef::SetMainWindow(MainWindow* pMainWindow) {
  mMainWindow = pMainWindow;
}

void EventRef::SetEvent(const libcomp::String& event) {
  ui->eventID->lineEdit()->setText(qs(event));
}

libcomp::String EventRef::GetEvent() const {
  return libcomp::String(ui->eventID->currentText().toStdString());
}

void EventRef::RefreshAllEventIDs(MainWindow* pMainWindow) {
  sAllEventIDs.clear();
  for (auto& eventID : pMainWindow->GetEvents()->GetCurrentEventIDs()) {
    sAllEventIDs.push_back(eventID);
  }

  sAllEventIDs.sort();
}

void EventRef::Go() {
  auto event = GetEvent();
  if (mMainWindow && !event.IsEmpty()) {
    auto eventWindow = mMainWindow->GetEvents();
    eventWindow->GoToEvent(event);
  }
}
