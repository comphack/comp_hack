/**
 * @file tools/cathedral/src/ZoneTriggerUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a configured ZoneTrigger.
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

#ifndef TOOLS_CATHEDRAL_SRC_ZONETRIGGERUI_H
#define TOOLS_CATHEDRAL_SRC_ZONETRIGGERUI_H

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QWidget>

// Standard C++11 Includes
#include <memory>

namespace objects {

class ServerZoneTrigger;

}  // namespace objects

namespace Ui {

class ZoneTrigger;

}  // namespace Ui

class MainWindow;

class ZoneTrigger : public QWidget {
  Q_OBJECT

 public:
  explicit ZoneTrigger(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~ZoneTrigger();

  void Load(const std::shared_ptr<objects::ServerZoneTrigger> &trigger);
  std::shared_ptr<objects::ServerZoneTrigger> Save() const;

 protected:
  Ui::ZoneTrigger *prop;
};

#endif  // TOOLS_CATHEDRAL_SRC_ZONETRIGGERUI_H
