/**
 * @file tools/cathedral/src/ActionList.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a list of actions.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONLIST_H
#define TOOLS_CATHEDRAL_SRC_ACTIONLIST_H

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QWidget>

// objects Includes
#include <Action.h>

// Standard C++11 Includes
#include <list>

namespace Ui {

class ActionList;

}  // namespace Ui

class Action;
class MainWindow;

class ActionList : public QWidget {
  Q_OBJECT

 public:
  explicit ActionList(QWidget *pParent = 0);
  virtual ~ActionList();

  void SetMainWindow(MainWindow *pMainWindow);

  void Load(const std::list<std::shared_ptr<objects::Action>> &actions);
  std::list<std::shared_ptr<objects::Action>> Save() const;

  void RemoveAction(Action *pAction);
  void MoveUp(Action *pAction);
  void MoveDown(Action *pAction);

  static std::list<std::pair<libcomp::String, int32_t>> GetActions();

 protected slots:
  void AddNewAction();
  void RefreshPositions();

 signals:
  void rowEdit();

 protected:
  void AddAction(const std::shared_ptr<objects::Action> &act, Action *pAction);
  void ClearActions();

  Ui::ActionList *ui;

  MainWindow *mMainWindow;
  std::list<Action *> mActions;
};

#endif  // TOOLS_CATHEDRAL_SRC_ACTIONLIST_H
