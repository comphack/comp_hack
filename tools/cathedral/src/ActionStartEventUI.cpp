/**
 * @file tools/cathedral/src/ActionStartEventUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a start event action.
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

#include "ActionStartEventUI.h"

// Cathedral Includes
#include "EventRef.h"
#include "MainWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionStartEvent.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionStartEvent::ActionStartEvent(ActionList *pList, MainWindow *pMainWindow,
                                   QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionStartEvent;
  prop->setupUi(pWidget);

  prop->eventTransformScriptParams->Setup(DynamicItemType_t::PRIMITIVE_STRING,
                                          nullptr);
  prop->eventTransformScriptParams->SetAddText("Add Param");

  ui->actionTitle->setText(tr("<b>Start Event</b>"));
  ui->layoutMain->addWidget(pWidget);

  prop->event->SetMainWindow(pMainWindow);
}

ActionStartEvent::~ActionStartEvent() { delete prop; }

void ActionStartEvent::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionStartEvent>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->event->SetEvent(mAction->GetEventID());
  prop->allowInterrupt->setCurrentIndex(
      to_underlying(mAction->GetAllowInterrupt()));
  prop->autoOnly->setChecked(mAction->GetAutoOnly());

  for (auto param : mAction->GetEventTransformScriptParams()) {
    prop->eventTransformScriptParams->AddString(param);
  }
}

std::shared_ptr<objects::Action> ActionStartEvent::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetEventID(prop->event->GetEvent());
  mAction->SetAllowInterrupt((objects::ActionStartEvent::AllowInterrupt_t)
                                 prop->allowInterrupt->currentIndex());
  mAction->SetAutoOnly(prop->autoOnly->isChecked());

  mAction->SetEventTransformScriptParams(
      prop->eventTransformScriptParams->GetStringList());

  return mAction;
}
