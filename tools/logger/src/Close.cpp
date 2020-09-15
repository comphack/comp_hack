/**
 * @file tools/logger/src/Close.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Dialog to warn the user about disconnecting clients on app exit.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "Close.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QApplication>
#include <QSettings>

// Stop ignoring warnings
#include <PopIgnore.h>

Close::Close(QWidget *p) : QDialog(p) {
  // Setup the UI.
  ui.setupUi(this);

  // Connect the buttons to the slots.
  connect(ui.yesButton, SIGNAL(clicked(bool)), this, SLOT(fuckEm()));
  connect(ui.noButton, SIGNAL(clicked(bool)), this, SLOT(deleteLater()));
}

Close::~Close() {}

void Close::fuckEm() {
  // Save the setting.
  QSettings().setValue("noexitwarning", ui.ignoreCheckbox->isChecked());

  // Quit the application.
  qApp->quit();
}
