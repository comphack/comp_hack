/**
 * @file tools/cathedral/src/ActionUpdateQuestUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an update quest action.
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

#include "ActionUpdateQuestUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionUpdateQuest.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionUpdateQuest::ActionUpdateQuest(ActionList *pList, MainWindow *pMainWindow,
                                     QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionUpdateQuest;
  prop->setupUi(pWidget);

  prop->flagStates->SetValueName(tr("State:"));
  prop->quest->BindSelector(pMainWindow, "CQuestData");

  ui->actionTitle->setText(tr("<b>Update Quest</b>"));
  ui->layoutMain->addWidget(pWidget);
}

ActionUpdateQuest::~ActionUpdateQuest() { delete prop; }

void ActionUpdateQuest::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionUpdateQuest>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->quest->SetValue((uint32_t)mAction->GetQuestID());
  prop->phase->setValue(mAction->GetPhase());
  prop->forceUpdate->setChecked(mAction->GetForceUpdate());
  prop->flagSetMode->setCurrentIndex(to_underlying(mAction->GetFlagSetMode()));

  auto states = mAction->GetFlagStates();
  prop->flagStates->Load(states);
}

std::shared_ptr<objects::Action> ActionUpdateQuest::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetQuestID((int16_t)prop->quest->GetValue());
  mAction->SetPhase((int8_t)prop->phase->value());
  mAction->SetForceUpdate(prop->forceUpdate->isChecked());
  mAction->SetFlagSetMode((objects::ActionUpdateQuest::FlagSetMode_t)
                              prop->flagSetMode->currentIndex());

  auto states = prop->flagStates->SaveSigned();
  mAction->SetFlagStates(states);

  return mAction;
}
