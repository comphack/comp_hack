/**
 * @file tools/cathedral/src/ActionUpdatePointsUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an update points action.
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

#include "ActionUpdatePointsUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionUpdatePoints.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionUpdatePoints::ActionUpdatePoints(ActionList *pList,
                                       MainWindow *pMainWindow,
                                       QWidget *pParent)
    : Action(pList, pMainWindow, pParent) {
  QWidget *pWidget = new QWidget;
  prop = new Ui::ActionUpdatePoints;
  prop->setupUi(pWidget);

  ui->actionTitle->setText(tr("<b>Update Points</b>"));
  ui->layoutMain->addWidget(pWidget);

  connect(prop->pointType, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(PointTypeChanged()));
}

ActionUpdatePoints::~ActionUpdatePoints() { delete prop; }

void ActionUpdatePoints::Load(const std::shared_ptr<objects::Action> &act) {
  mAction = std::dynamic_pointer_cast<objects::ActionUpdatePoints>(act);

  if (!mAction) {
    return;
  }

  LoadBaseProperties(mAction);

  prop->pointType->setCurrentIndex(to_underlying(mAction->GetPointType()));
  prop->value->setValue((int32_t)mAction->GetValue());
  prop->modifier->setValue(mAction->GetModifier());
  prop->isSet->setChecked(mAction->GetIsSet());
}

std::shared_ptr<objects::Action> ActionUpdatePoints::Save() const {
  if (!mAction) {
    return nullptr;
  }

  SaveBaseProperties(mAction);

  mAction->SetPointType((objects::ActionUpdatePoints::PointType_t)
                            prop->pointType->currentIndex());
  mAction->SetValue(prop->value->value());
  mAction->SetModifier((int8_t)prop->modifier->value());
  mAction->SetIsSet(prop->isSet->isChecked());

  return mAction;
}

void ActionUpdatePoints::PointTypeChanged() {
  prop->value->setEnabled(true);
  prop->modifier->setEnabled(true);
  prop->isSet->setEnabled(true);

  prop->value->setMinimum(0);
  prop->modifier->setMinimum(0);
  prop->value->setMaximum(2147483647);
  prop->modifier->setMaximum(2147483647);

  prop->lblValue->setText("Value:");
  prop->lblModifier->setText("Modifier:");
  prop->lblIsSet->setText("Set:");

  switch ((objects::ActionUpdatePoints::PointType_t)
              prop->pointType->currentIndex()) {
    case objects::ActionUpdatePoints::PointType_t::BETHEL:
      prop->lblModifier->setText("Set Type:");
      prop->value->setMinimum(-2147483647);
      prop->modifier->setMaximum(4);
      break;
    case objects::ActionUpdatePoints::PointType_t::CP:
      prop->isSet->setEnabled(false);
      break;
    case objects::ActionUpdatePoints::PointType_t::ITIME:
      prop->lblModifier->setText("I-Time ID:");
      prop->value->setMinimum(-1);
      break;
    case objects::ActionUpdatePoints::PointType_t::PVP_POINTS:
      prop->value->setMinimum(-2147483647);
      prop->modifier->setEnabled(false);
      prop->isSet->setEnabled(false);
      break;
    case objects::ActionUpdatePoints::PointType_t::ZIOTITE:
      prop->lblValue->setText("Small Ziotite:");
      prop->lblModifier->setText("Large Ziotite:");
      prop->value->setMinimum(-2147483647);
      break;
    case objects::ActionUpdatePoints::PointType_t::BP:
    case objects::ActionUpdatePoints::PointType_t::COINS:
    case objects::ActionUpdatePoints::PointType_t::KILL_VALUE:
    case objects::ActionUpdatePoints::PointType_t::SOUL_POINTS:
      prop->value->setMinimum(-2147483647);
      prop->modifier->setEnabled(false);
      break;
    case objects::ActionUpdatePoints::PointType_t::COWRIE:
    case objects::ActionUpdatePoints::PointType_t::UB_POINTS:
      prop->value->setMinimum(-2147483647);
      prop->modifier->setEnabled(false);
      prop->isSet->setEnabled(false);
      break;
    case objects::ActionUpdatePoints::PointType_t::DIGITALIZE_POINTS:
      prop->modifier->setEnabled(false);
      break;
    case objects::ActionUpdatePoints::PointType_t::REUNION_POINTS:
      prop->lblModifier->setText("Mitama?:");
      prop->value->setMinimum(0);
      prop->modifier->setMinimum(0);
      prop->modifier->setMaximum(1);
      prop->isSet->setEnabled(false);
      break;
    default:
      break;
  }

  if (!prop->value->isEnabled()) {
    prop->value->setValue(0);
  }

  if (!prop->modifier->isEnabled()) {
    prop->modifier->setValue(0);
  }

  if (!prop->isSet->isEnabled()) {
    prop->isSet->setChecked(false);
  }
}
