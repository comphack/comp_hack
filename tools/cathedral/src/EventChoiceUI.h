/**
 * @file tools/cathedral/src/EventChoiceUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for an event choice.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTCHOICEUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTCHOICEUI_H

// Cathedral Includes
#include "EventBaseUI.h"

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QGroupBox>
#include <QWidget>

// Stop ignoring warnings
#include <PopIgnore.h>

// objects Includes
#include <EventChoice.h>

class DynamicList;
class EventMessageRef;
class ServerScript;

class EventChoice : public EventBase {
  Q_OBJECT

 public:
  explicit EventChoice(MainWindow *pMainWindow, bool isITime,
                       QWidget *pParent = 0);
  virtual ~EventChoice();

  void Load(const std::shared_ptr<objects::EventChoice> &e);
  std::shared_ptr<objects::EventChoice> Save() const;

 protected:
  EventMessageRef *mMessage;
  QGroupBox *mBranchGroup;
  DynamicList *mBranches;
  ServerScript *mBranchScript;
};

#endif  // TOOLS_CATHEDRAL_SRC_EVENTCHOICEUI_H
