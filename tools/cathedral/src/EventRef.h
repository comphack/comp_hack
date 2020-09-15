/**
 * @file tools/cathedral/src/EventRef.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for an event being referenced from all known events.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTREF_H
#define TOOLS_CATHEDRAL_SRC_EVENTREF_H

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QWidget>

// libcomp Includes
#include <CString.h>

namespace Ui {

class EventRef;

}  // namespace Ui

class MainWindow;

class EventRef : public QWidget {
  Q_OBJECT

 public:
  explicit EventRef(QWidget *pParent = 0);
  virtual ~EventRef();

  void SetMainWindow(MainWindow *pMainWindow);

  void SetEvent(const libcomp::String &event);
  libcomp::String GetEvent() const;

  static void RefreshAllEventIDs(MainWindow *pMainWindow);

 public slots:
  void Go();

 protected:
  Ui::EventRef *ui;

  MainWindow *mMainWindow;

  static std::list<libcomp::String> sAllEventIDs;
};

#endif  // TOOLS_CATHEDRAL_SRC_EVENTREF_H
