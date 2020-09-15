/**
 * @file tools/capgrep/src/Settings.h
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the class used to change settings for the application.
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

#ifndef TOOLS_CAPGREP_SRC_SETTINGS_H
#define TOOLS_CAPGREP_SRC_SETTINGS_H

// Ignore warnings
#include <PushIgnore.h>

#include "ui_Settings.h"

// Stop ignoring warnings
#include <PopIgnore.h>

/**
 * Settings dialog to configure capgrep.
 * @ingroup capgrep
 */
class Settings : public QDialog {
  Q_OBJECT

 public:
  /**
   * Default constructor.
   */
  Settings(QWidget *parent = 0);

 signals:
  /**
   * Signal to indicate the packet limit has changed.
   * @param limit The new packet limit or 0 if there is no limit. Negative
   * values are not valid for this signal.
   */
  void packetLimitChanged(int limit);

 protected slots:
  /**
   * Save the current settings and close the dialog.
   */
  void save();

 protected:
  /// Generated user interface object for the dialog.
  Ui::Settings ui;
};

#endif  // TOOLS_CAPGREP_SRC_SETTINGS_H
