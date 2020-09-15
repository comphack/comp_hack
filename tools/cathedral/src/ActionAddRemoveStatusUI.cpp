/**
 * @file tools/cathedral/src/ActionAddRemoveStatusUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an add/remove status action.
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

#include "ActionAddRemoveStatusUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionAddRemoveStatus.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionAddRemoveStatus::ActionAddRemoveStatus(ActionList *pList,
                                             MainWindow *pMainWindow,
                                             QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionAddRemoveStatus;
  prop->setupUi(pWidget);

  prop->statusStacks->BindSelector(pMainWindow, "StatusData");
  prop->statusStacks->SetValueName(tr("Stacks:"));
  prop->statusStacks->SetMinMax(0, 255);
  prop->statusStacks->SetAddText("Add Status Effect");

  prop->statusTimes->BindSelector(pMainWindow, "StatusData");
  prop->statusTimes->SetValueName(tr("Time:"));
  prop->statusTimes->SetAddText("Add Status Effect");

  ui->actionTitle->setText(tr("<b>Add/Remove Status</b>"));
  ui->layoutMain->addWidget(pWidget);
}

ActionAddRemoveStatus::~ActionAddRemoveStatus() { delete prop; }

void ActionAddRemoveStatus::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionAddRemoveStatus>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->targetType->setCurrentIndex(to_underlying(mAction->GetTargetType()));
  prop->isReplace->setChecked(mAction->GetIsReplace());
  prop->allowNull->setChecked(mAction->GetAllowNull());

  std::unordered_map<uint32_t, int32_t> stacks;
  std::unordered_map<uint32_t, int32_t> times;

  for (auto stack : mAction->GetStatusStacks()) {
    stacks[stack.first] = stack.second;
  }

  for (auto time : mAction->GetStatusTimes()) {
    times[time.first] = (int32_t)time.second;
  }

  prop->statusStacks->Load(stacks);
  prop->statusTimes->Load(times);
}

std::shared_ptr<objects::Action> ActionAddRemoveStatus::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetTargetType((objects::ActionAddRemoveStatus::TargetType_t)
                             prop->targetType->currentIndex());
  mAction->SetIsReplace(prop->isReplace->isChecked());
  mAction->SetAllowNull(prop->allowNull->isChecked());

  mAction->ClearStatusStacks();
  for (auto pair : prop->statusStacks->SaveUnsigned()) {
    mAction->SetStatusStacks(pair.first, (int8_t)pair.second);
  }

  mAction->ClearStatusTimes();
  for (auto pair : prop->statusTimes->SaveUnsigned()) {
    mAction->SetStatusTimes(pair.first, (uint32_t)pair.second);
  }

  return mAction;
}
