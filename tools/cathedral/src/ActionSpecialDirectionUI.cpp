/**
 * @file tools/cathedral/src/ActionSpecialDirectionUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a special direction action.
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

#include "ActionSpecialDirectionUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionSpecialDirection.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionSpecialDirection::ActionSpecialDirection(ActionList *pList,
                                               MainWindow *pMainWindow,
                                               QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionSpecialDirection;
  prop->setupUi(pWidget);

  ui->actionTitle->setText(tr("<b>Special Direction</b>"));
  ui->layoutMain->addWidget(pWidget);
}

ActionSpecialDirection::~ActionSpecialDirection() { delete prop; }

void ActionSpecialDirection::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionSpecialDirection>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->direction->lineEdit()->setText(
      QString::number(mAction->GetDirection()));
  prop->special1->setValue(mAction->GetSpecial1());
  prop->special2->setValue(mAction->GetSpecial2());
}

std::shared_ptr<objects::Action> ActionSpecialDirection::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetDirection(prop->direction->currentText().toInt());
  mAction->SetSpecial1((uint8_t)prop->special1->value());
  mAction->SetSpecial2((uint8_t)prop->special2->value());

  return mAction;
}
