/**
 * @file tools/cathedral/src/EventMessageRef.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event message being referenced.
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

#include "EventMessageRef.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "MainWindow.h"

// objects Includes
#include <MiCEventMessageData.h>

// Ignore warnings
#include <PushIgnore.h>

// UI Includes
#include "ui_EventMessageRef.h"

// Stop ignoring warnings
#include <PopIgnore.h>

EventMessageRef::EventMessageRef(QWidget *pParent)
    : ObjectSelectorBase(pParent) {
  ui = new Ui::EventMessageRef;
  ui->setupUi(this);

  ui->message->setFontPointSize(10);
  ui->message->setText("[Empty]");

  connect(ui->getMessage, SIGNAL(clicked()), this, SLOT(GetItem()));
  connect(ui->messageID, SIGNAL(valueChanged(int)), this,
          SLOT(MessageIDChanged()));
}

EventMessageRef::~EventMessageRef() { delete ui; }

void EventMessageRef::Setup(MainWindow *pMainWindow,
                            const libcomp::String &objType) {
  Bind(pMainWindow, objType);
}

void EventMessageRef::SetValue(uint32_t value) {
  ui->messageID->setValue((int32_t)value);
}

uint32_t EventMessageRef::GetValue() const {
  return (uint32_t)ui->messageID->value();
}

void EventMessageRef::MessageIDChanged() {
  ui->message->setText("[Empty]");

  if (mMainWindow) {
    auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet(GetObjectType()));
    if (dataset) {
      ui->message->setText(
          qs(dataset->GetName(dataset->GetObjectByID(GetValue()))));
    }
  }
}
