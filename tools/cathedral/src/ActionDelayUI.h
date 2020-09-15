/**
 * @file tools/cathedral/src/ActionDelayUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a delay action.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONDELAYUI_H
#define TOOLS_CATHEDRAL_SRC_ACTIONDELAYUI_H

// Cathedral Includes
#include "ActionUI.h"

// objects Includes
#include <ActionDelay.h>

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QWidget>

// Stop ignoring warnings
#include <PopIgnore.h>

namespace Ui {

class ActionDelay;

}  // namespace Ui

class MainWindow;

class ActionDelay : public Action {
  Q_OBJECT

 public:
  explicit ActionDelay(ActionList *pList, MainWindow *pMainWindow,
                       QWidget *pParent = 0);
  virtual ~ActionDelay();

  void Load(const std::shared_ptr<objects::Action> &act) override;
  std::shared_ptr<objects::Action> Save() const override;

 protected:
  Ui::ActionDelay *prop;

  std::shared_ptr<objects::ActionDelay> mAction;
};

#endif  // TOOLS_CATHEDRAL_SRC_ACTIONDELAYUI_H
