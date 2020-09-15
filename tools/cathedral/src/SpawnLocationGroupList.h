/**
 * @file tools/cathedral/src/SpawnLocationGroupList.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a control that holds a list of spawn location groups.
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

#ifndef TOOLS_CATHEDRAL_SRC_SPAWNLOCATIONGROUPLIST_H
#define TOOLS_CATHEDRAL_SRC_SPAWNLOCATIONGROUPLIST_H

#include "ObjectList.h"

namespace Ui {

class SpawnLocationGroup;

}  // namespace Ui

class SpawnLocationGroupList : public ObjectList {
  Q_OBJECT

 public:
  explicit SpawnLocationGroupList(QWidget* pParent = 0);
  virtual ~SpawnLocationGroupList();

  virtual void SetMainWindow(MainWindow* pMainWindow);

  QString GetObjectID(
      const std::shared_ptr<libcomp::Object>& obj) const override;
  QString GetObjectName(
      const std::shared_ptr<libcomp::Object>& obj) const override;

  void LoadProperties(const std::shared_ptr<libcomp::Object>& obj) override;
  void SaveProperties(const std::shared_ptr<libcomp::Object>& obj) override;

 protected:
  Ui::SpawnLocationGroup* prop;
};

#endif  // TOOLS_CATHEDRAL_SRC_SPAWNLOCATIONGROUPLIST_H
