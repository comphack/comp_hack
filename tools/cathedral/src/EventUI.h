/**
 * @file tools/cathedral/src/EventUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for an event.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTUI_H

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QWidget>

// objects Includes
#include <Event.h>

namespace Ui {

class Event;

}  // namespace Ui

class MainWindow;

class Event : public QWidget {
  Q_OBJECT

 public:
  explicit Event(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~Event();

  virtual void Load(const std::shared_ptr<objects::Event> &e);
  virtual std::shared_ptr<objects::Event> Save() const;

  void SetComments(const std::list<libcomp::String> &comments);
  std::list<libcomp::String> GetComments();

 public slots:
  void ChangeEventID();
  virtual void ToggleBaseDisplay();

 protected:
  Ui::Event *ui;

  MainWindow *mMainWindow;

  std::shared_ptr<objects::Event> mEventBase;
};

#endif  // TOOLS_CATHEDRAL_SRC_EVENTUI_H
