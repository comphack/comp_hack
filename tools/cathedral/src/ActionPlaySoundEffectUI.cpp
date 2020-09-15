/**
 * @file tools/cathedral/src/ActionPlaySoundEffectUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a play sound effect action.
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

#include "ActionPlaySoundEffectUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionPlaySoundEffect.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionPlaySoundEffect::ActionPlaySoundEffect(ActionList *pList,
                                             MainWindow *pMainWindow,
                                             QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionPlaySoundEffect;
  prop->setupUi(pWidget);

  prop->sound->BindSelector(pMainWindow, "CSoundData");

  ui->actionTitle->setText(tr("<b>Play Sound Effect</b>"));
  ui->layoutMain->addWidget(pWidget);
}

ActionPlaySoundEffect::~ActionPlaySoundEffect() { delete prop; }

void ActionPlaySoundEffect::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionPlaySoundEffect>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->sound->SetValue((uint32_t)mAction->GetSoundID());
  prop->delay->setValue(mAction->GetDelay());
  prop->waitTime->setValue(mAction->GetWaitTime());
}

std::shared_ptr<objects::Action> ActionPlaySoundEffect::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetSoundID((int32_t)prop->sound->GetValue());
  mAction->SetDelay(prop->delay->value());
  mAction->SetWaitTime(prop->waitTime->value());

  return mAction;
}
