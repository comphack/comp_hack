/**
 * @file tools/cathedral/src/ItemDropUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a configured ItemDrop.
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

#ifndef TOOLS_CATHEDRAL_SRC_ITEMDROPUI_H
#define TOOLS_CATHEDRAL_SRC_ITEMDROPUI_H

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QWidget>

// Stop ignoring warnings
#include <PopIgnore.h>

// Standard C++11 Includes
#include <memory>

namespace objects {

class ItemDrop;

}  // namespace objects

namespace Ui {

class ItemDrop;

}  // namespace Ui

class MainWindow;

class ItemDrop : public QWidget {
  Q_OBJECT

 public:
  explicit ItemDrop(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~ItemDrop();

  void Load(const std::shared_ptr<objects::ItemDrop> &drop);
  std::shared_ptr<objects::ItemDrop> Save() const;

 protected slots:
  void MinStackChanged();
  void TypeChanged();

 protected:
  Ui::ItemDrop *prop;
};

#endif  // TOOLS_CATHEDRAL_SRC_ITEMDROPUI_H
