/**
 * @file tools/cathedral/src/ActionMap.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a map of IDs to a value (for use in actions).
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONMAP_H
#define TOOLS_CATHEDRAL_SRC_ACTIONMAP_H

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QWidget>

// Stop ignoring warnings
#include <PopIgnore.h>

// objects Includes
#include <Action.h>

// Standard C++11 Includes
#include <unordered_map>

namespace Ui {

class ActionMap;

}  // namespace Ui

class ActionMapItem;
class MainWindow;

class ActionMap : public QWidget {
  Q_OBJECT

 public:
  explicit ActionMap(QWidget *pParent = 0);
  virtual ~ActionMap();

  void BindSelector(MainWindow *pMainWindow,
                    const libcomp::String &objectSelectorType,
                    bool serverData = false);

  void SetAddText(const libcomp::String &text);

  void Load(const std::unordered_map<int32_t, int32_t> &values);
  void Load(const std::unordered_map<uint32_t, int32_t> &values);
  std::unordered_map<int32_t, int32_t> SaveSigned() const;
  std::unordered_map<uint32_t, int32_t> SaveUnsigned() const;

  void RemoveValue(ActionMapItem *pValue);
  void SetValueName(const QString &name);
  void SetMinMax(int32_t min, int32_t max);

 protected slots:
  void AddNewValue();

 signals:
  void rowEdit();

 protected:
  void AddValue(ActionMapItem *pValue);
  void ClearValues();

  QString mValueName;
  int32_t mMin, mMax;

  libcomp::String mObjectSelectorType;

  bool mServerData;

  Ui::ActionMap *ui;

  MainWindow *mMainWindow;
  std::list<ActionMapItem *> mValues;
};

#endif  // TOOLS_CATHEDRAL_SRC_ACTIONMAP_H
