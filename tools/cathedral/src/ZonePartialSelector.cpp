/**
 * @file tools/cathedral/src/ZonePartialSelector.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a zone partial selection window.
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

#include "ZonePartialSelector.h"

// Cathedral Includes
#include "MainWindow.h"
#include "ZoneWindow.h"

// Ignore warnings
#include <PushIgnore.h>

// UI Includes
#include "ui_ZonePartialSelector.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// object Includes
#include <ServerZone.h>
#include <ServerZonePartial.h>

ZonePartialSelector::ZonePartialSelector(MainWindow *pMainWindow,
                                         QWidget *pParent)
    : QDialog(pParent), mMainWindow(pMainWindow) {
  ui = new Ui::ZonePartialSelector;
  ui->setupUi(this);

  connect(ui->apply, SIGNAL(clicked()), this, SLOT(close()));
}

ZonePartialSelector::~ZonePartialSelector() { delete ui; }

std::set<uint32_t> ZonePartialSelector::Select() {
  if (mMainWindow) {
    auto zoneWindow = mMainWindow->GetZones();

    auto merged = zoneWindow->GetMergedZone();
    auto partials = zoneWindow->GetLoadedPartials();
    auto selected = zoneWindow->GetSelectedPartials();

    ui->tableWidget->setRowCount((int)partials.size());

    uint32_t dynamicMapID = merged->CurrentZone->GetDynamicMapID();

    int idx = 0;
    for (auto &pair : partials) {
      auto partial = pair.second;

      bool mapped =
          dynamicMapID && partial->DynamicMapIDsContains(dynamicMapID);
      bool hasObjects = partial->NPCsCount() || partial->ObjectsCount();
      bool hasSpawns = partial->SpawnsCount() || partial->SpawnGroupsCount() ||
                       partial->SpawnLocationGroupsCount();
      bool hasSpots = partial->SpotsCount() > 0;
      bool hasTriggers = partial->TriggersCount() > 0;
      bool hasOther = partial->DropSetIDsCount() ||
                      partial->SkillBlacklistCount() ||
                      partial->SkillWhitelistCount();

      ui->tableWidget->setItem(
          idx, 0, new QTableWidgetItem(QString::number(pair.first)));
      ui->tableWidget->setItem(idx, 1,
                               new QTableWidgetItem((mapped ? "Y" : "N")));
      ui->tableWidget->setItem(
          idx, 2, new QTableWidgetItem((partial->GetAutoApply() ? "Y" : "N")));
      ui->tableWidget->setItem(idx, 3,
                               new QTableWidgetItem((hasObjects ? "Y" : "N")));
      ui->tableWidget->setItem(idx, 4,
                               new QTableWidgetItem((hasSpawns ? "Y" : "N")));
      ui->tableWidget->setItem(idx, 5,
                               new QTableWidgetItem((hasSpots ? "Y" : "N")));
      ui->tableWidget->setItem(idx, 6,
                               new QTableWidgetItem((hasTriggers ? "Y" : "N")));
      ui->tableWidget->setItem(idx, 7,
                               new QTableWidgetItem((hasOther ? "Y" : "N")));

      if (selected.find(pair.first) != selected.end()) {
        auto qIdx = ui->tableWidget->model()->index(idx, 0);
        ui->tableWidget->selectionModel()->select(
            qIdx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
      }

      idx++;
    }

    ui->tableWidget->resizeColumnsToContents();
  }

  // Block until selection is done
  exec();

  std::set<uint32_t> selection;

  int rowCount = ui->tableWidget->rowCount();
  for (int i = 0; i < rowCount; i++) {
    auto item = ui->tableWidget->item(i, 0);
    if (ui->tableWidget->isItemSelected(item)) {
      selection.insert(item->text().toUInt());
    }
  }

  return selection;
}
