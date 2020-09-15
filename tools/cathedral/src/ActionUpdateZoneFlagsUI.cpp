/**
 * @file tools/cathedral/src/ActionUpdateZoneFlagsUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an update zone flags action.
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

#include "ActionUpdateZoneFlagsUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionUpdateZoneFlags.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionUpdateZoneFlags::ActionUpdateZoneFlags(ActionList *pList,
                                             MainWindow *pMainWindow,
                                             QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionUpdateZoneFlags;
  prop->setupUi(pWidget);
  prop->flagStates->SetValueName(tr("State:"));

  ui->actionTitle->setText(tr("<b>Update Zone Flags</b>"));
  ui->layoutMain->addWidget(pWidget);
}

ActionUpdateZoneFlags::~ActionUpdateZoneFlags() { delete prop; }

void ActionUpdateZoneFlags::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionUpdateZoneFlags>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->type->setCurrentIndex(to_underlying(mAction->GetType()));
  prop->setMode->setCurrentIndex(to_underlying(mAction->GetSetMode()));

  auto states = mAction->GetFlagStates();
  prop->flagStates->Load(states);
}

std::shared_ptr<objects::Action> ActionUpdateZoneFlags::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetType(
      (objects::ActionUpdateZoneFlags::Type_t)prop->type->currentIndex());
  mAction->SetSetMode(
      (objects::ActionUpdateZoneFlags::SetMode_t)prop->setMode->currentIndex());

  auto states = prop->flagStates->SaveSigned();
  mAction->SetFlagStates(states);

  return mAction;
}
