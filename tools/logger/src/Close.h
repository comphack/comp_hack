/**
 * @file tools/logger/src/Close.h
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

#ifndef TOOLS_LOGGER_SRC_CLOSE_H
#define TOOLS_LOGGER_SRC_CLOSE_H

// Ignore warnings
#include <PushIgnore.h>

// UI Includes
#include "ui_Close.h"

// Stop ignoring warnings
#include <PopIgnore.h>

/**
 * This dialog notifies the user that any clients will be disconnected when the
 * application exists.
 */
class Close : public QDialog {
  Q_OBJECT

 public:
  /**
   * Construct the dialog box.
   * @arg parent Parent object that this dialog belongs to. Should remain 0.
   */
  Close(QWidget *parent = 0);

  /**
   * Delete the dialog box.
   */
  ~Close();

 protected slots:
  /**
   * Close the application anyway.
   */
  void fuckEm();

 protected:
  /// Generated UI for the dialog.
  Ui::Close ui;
};

#endif  // TOOLS_LOGGER_SRC_CLOSE_H
