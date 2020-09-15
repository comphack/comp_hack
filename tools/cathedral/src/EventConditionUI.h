/**
 * @file tools/cathedral/src/EventConditionUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for an event condition.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTCONDITIONUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTCONDITIONUI_H

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QWidget>

// objects Includes
#include <EventCondition.h>

namespace Ui {

class EventCondition;

}  // namespace Ui

class MainWindow;

class EventCondition : public QWidget {
  Q_OBJECT

 public:
  explicit EventCondition(MainWindow *pMainWindow, QWidget *pParent = 0);
  virtual ~EventCondition();

  virtual void Load(const std::shared_ptr<objects::EventCondition> &e);
  virtual std::shared_ptr<objects::EventCondition> Save() const;

 protected slots:
  void RadioToggle();
  void CompareModeSelectionChanged();
  void TypeSelectionChanged();

 private:
  objects::EventCondition::Type_t GetCurrentType() const;
  void RefreshAvailableOptions();
  void RefreshTypeContext();
  void ResetCompareMode(const libcomp::String &defaultCompareTxt);

  Ui::EventCondition *ui;
  MainWindow *mMainWindow;
};

#endif  // TOOLS_CATHEDRAL_SRC_EVENTCONDITIONUI_H
