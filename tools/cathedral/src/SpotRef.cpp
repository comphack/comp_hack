/**
 * @file tools/cathedral/src/SpotRef.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a zone spot being referenced.
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

#include "SpotRef.h"

// Cathedral Includes
#include "MainWindow.h"
#include "ZoneWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include "ui_SpotRef.h"

SpotRef::SpotRef(QWidget *pParent) : QWidget(pParent) {
  ui = new Ui::SpotRef;
  ui->setupUi(this);

  // Hide the show button until the window is bound
  ui->show->hide();

  connect(ui->show, SIGNAL(clicked(bool)), this, SLOT(Show()));
}

SpotRef::~SpotRef() { delete ui; }

void SpotRef::SetMainWindow(MainWindow *pMainWindow) {
  mMainWindow = pMainWindow;
  ui->show->show();
}

void SpotRef::SetValue(uint32_t spotID) {
  ui->spotID->setValue((int32_t)spotID);
}

uint32_t SpotRef::GetValue() const { return (uint32_t)ui->spotID->value(); }

void SpotRef::Show() {
  uint32_t spotID = GetValue();
  if (mMainWindow && spotID) {
    auto zoneWindow = mMainWindow->GetZones();
    zoneWindow->ShowSpot(spotID);
  }
}
