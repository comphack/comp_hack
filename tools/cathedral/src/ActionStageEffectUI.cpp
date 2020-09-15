/**
 * @file tools/cathedral/src/ActionStageEffectUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a stage effect action.
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

#include "ActionStageEffectUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionStageEffect.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionStageEffect::ActionStageEffect(ActionList *pList, MainWindow *pMainWindow,
                                     QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionStageEffect;
  prop->setupUi(pWidget);

  ui->actionTitle->setText(tr("<b>Stage Effect</b>"));
  ui->layoutMain->addWidget(pWidget);

  prop->message->Setup(pMainWindow);
}

ActionStageEffect::~ActionStageEffect() { delete prop; }

void ActionStageEffect::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionStageEffect>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->message->SetValue((uint32_t)mAction->GetMessageID());
  prop->effectType->setValue(mAction->GetEffectType());
  prop->messageValue->setValue(mAction->GetMessageValue());
  prop->includeMessage->setChecked(mAction->GetIncludeMessage());
}

std::shared_ptr<objects::Action> ActionStageEffect::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetMessageID((int32_t)prop->message->GetValue());
  mAction->SetEffectType((int8_t)prop->effectType->value());
  mAction->SetMessageValue(prop->messageValue->value());
  mAction->SetIncludeMessage(prop->includeMessage->isChecked());

  return mAction;
}
