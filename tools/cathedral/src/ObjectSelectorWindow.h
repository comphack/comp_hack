/**
 * @file tools/cathedral/src/ObjectSelectorWindow.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a value selection window bound to an ObjectSelector.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTSELECTORWINDOW_H
#define TOOLS_CATHEDRAL_SRC_OBJECTSELECTORWINDOW_H

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QMainWindow>

// libcomp Includes
#include <CString.h>
#include <Object.h>

namespace Ui {

class ObjectSelectorWindow;

}  // namespace Ui

class FindRefWindow;
class MainWindow;
class ObjectSelectorBase;
class ObjectSelectorList;

class ObjectSelectorWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit ObjectSelectorWindow(MainWindow* pMainWindow, QWidget* pParent = 0);
  virtual ~ObjectSelectorWindow();

  void Bind(ObjectSelectorList* listControl, bool findRef);

  void Open(ObjectSelectorBase* ctrl = 0);

  bool CloseIfConnected(QWidget* topLevel);

  void closeEvent(QCloseEvent* event) override;

 protected slots:
  void ObjectSelected();
  void SelectedObjectChanged();
  void Find();

 private:
  Ui::ObjectSelectorWindow* ui;

  MainWindow* mMainWindow;

  ObjectSelectorBase* mSelectorControl;

  FindRefWindow* mFindWindow;
};

#endif  // TOOLS_CATHEDRAL_SRC_OBJECTSELECTORWINDOW_H
