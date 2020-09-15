/**
 * @file tools/cathedral/src/SettingsWindow.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for the setting configuration window.
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

#ifndef TOOLS_CATHEDRAL_SRC_SETTINGSWINDOW_H
#define TOOLS_CATHEDRAL_SRC_SETTINGSWINDOW_H

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QDialog>

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <CString.h>

namespace Ui {

class SettingsWindow;

}  // namespace Ui

class MainWindow;

class SettingsWindow : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsWindow(MainWindow* pMainWindow, bool initializing,
                          QWidget* pParent = 0);
  virtual ~SettingsWindow();

 public slots:
  void BrowseCrashDump();
  void BrowseDatastore();

  void Save();

 protected:
  Ui::SettingsWindow* ui;

  MainWindow* mMainWindow;

  bool mInitializing;
};

#endif  // TOOLS_CATHEDRAL_SRC_SETTINGSWINDOW_H
