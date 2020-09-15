/**
 * @file tools/cathedral/src/EventDirectionUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a direction event.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTDIRECTIONUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTDIRECTIONUI_H

// Cathedral Includes
#include "EventUI.h"

// objects Includes
#include <EventDirection.h>

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QWidget>

// Stop ignoring warnings
#include <PopIgnore.h>

namespace Ui {

class EventDirection;

}  // namespace Ui

class MainWindow;

class EventDirection : public Event {
  Q_OBJECT

 public:
  explicit EventDirection(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~EventDirection();

  void Load(const std::shared_ptr<objects::Event> &e) override;
  std::shared_ptr<objects::Event> Save() const override;

 protected:
  Ui::EventDirection *prop;

  std::shared_ptr<objects::EventDirection> mEvent;
};

#endif  // TOOLS_CATHEDRAL_SRC_EVENTDIRECTIONUI_H
