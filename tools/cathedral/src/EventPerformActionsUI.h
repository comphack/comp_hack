/**
 * @file tools/cathedral/src/EventPerformActionsUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a perform actions event.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTPERFORMACTIONSUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTPERFORMACTIONSUI_H

// Cathedral Includes
#include "EventUI.h"

// objects Includes
#include <EventPerformActions.h>

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QWidget>

namespace Ui {

class EventPerformActions;

}  // namespace Ui

class MainWindow;

class EventPerformActions : public Event {
  Q_OBJECT

 public:
  explicit EventPerformActions(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~EventPerformActions();

  void Load(const std::shared_ptr<objects::Event> &e) override;
  std::shared_ptr<objects::Event> Save() const override;

 protected:
  Ui::EventPerformActions *prop;

  std::shared_ptr<objects::EventPerformActions> mEvent;
};

#endif  // TOOLS_CATHEDRAL_SRC_EVENTPERFORMACTIONSUI_H
