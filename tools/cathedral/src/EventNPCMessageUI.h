/**
 * @file tools/cathedral/src/EventNPCMessageUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for an NPC message event.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTNPCMESSAGEUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTNPCMESSAGEUI_H

// Cathedral Includes
#include "EventUI.h"

// objects Includes
#include <EventNPCMessage.h>

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QWidget>

// Stop ignoring warnings
#include <PopIgnore.h>

namespace Ui {

class EventNPCMessage;

}  // namespace Ui

class MainWindow;

class EventNPCMessage : public Event {
  Q_OBJECT

 public:
  explicit EventNPCMessage(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~EventNPCMessage();

  void Load(const std::shared_ptr<objects::Event> &e) override;
  std::shared_ptr<objects::Event> Save() const override;

 protected:
  Ui::EventNPCMessage *prop;

  std::shared_ptr<objects::EventNPCMessage> mEvent;
};

#endif  // TOOLS_CATHEDRAL_SRC_EVENTNPCMESSAGEUI_H
