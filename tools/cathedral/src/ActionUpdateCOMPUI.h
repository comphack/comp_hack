/**
 * @file tools/cathedral/src/ActionUpdateCOMPUI.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for an update COMP action.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONUPDATECOMPUI_H
#define TOOLS_CATHEDRAL_SRC_ACTIONUPDATECOMPUI_H

// Cathedral Includes
#include "ActionUI.h"

// objects Includes
#include <ActionUpdateCOMP.h>

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QWidget>

namespace Ui {

class ActionUpdateCOMP;

}  // namespace Ui

class MainWindow;

class ActionUpdateCOMP : public Action {
  Q_OBJECT

 public:
  explicit ActionUpdateCOMP(ActionList *pList, MainWindow *pMainWindow,
                            QWidget *pParent = 0);
  virtual ~ActionUpdateCOMP();

  void Load(const std::shared_ptr<objects::Action> &act) override;
  std::shared_ptr<objects::Action> Save() const override;

 protected:
  Ui::ActionUpdateCOMP *prop;

  std::shared_ptr<objects::ActionUpdateCOMP> mAction;
};

#endif  // TOOLS_CATHEDRAL_SRC_ACTIONUPDATECOMPUI_H
