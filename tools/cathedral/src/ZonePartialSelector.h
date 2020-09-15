/**
 * @file tools/cathedral/src/ZonePartialSelector.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a zone partial selection window.
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

#ifndef TOOLS_CATHEDRAL_SRC_ZONEPARTIALSELECTOR_H
#define TOOLS_CATHEDRAL_SRC_ZONEPARTIALSELECTOR_H

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QDialog>

// C++11 Standard Includes
#include <set>

namespace Ui {

class ZonePartialSelector;

}  // namespace Ui

class MainWindow;

class ZonePartialSelector : public QDialog {
  Q_OBJECT

 public:
  explicit ZonePartialSelector(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~ZonePartialSelector();

  std::set<uint32_t> Select();

 private:
  Ui::ZonePartialSelector *ui;

  MainWindow *mMainWindow;
};

#endif  // TOOLS_CATHEDRAL_SRC_ZONEPARTIALSELECTOR_H
