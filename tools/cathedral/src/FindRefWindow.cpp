/**
 * @file tools/cathedral/src/FindRefWindow.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a window that finds reference object uses.
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

#include "FindRefWindow.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "EventWindow.h"
#include "MainWindow.h"
#include "ZoneWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QCloseEvent>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>

// objects Includes
#include <ActionAddRemoveItems.h>
#include <ActionAddRemoveStatus.h>
#include <ActionCreateLoot.h>
#include <ActionDelay.h>
#include <ActionDisplayMessage.h>
#include <ActionPlayBGM.h>
#include <ActionPlaySoundEffect.h>
#include <ActionSetHomepoint.h>
#include <ActionSpawn.h>
#include <ActionStageEffect.h>
#include <ActionUpdateCOMP.h>
#include <ActionUpdateFlag.h>
#include <ActionUpdatePoints.h>
#include <ActionUpdateQuest.h>
#include <ActionZoneChange.h>
#include <DropSet.h>
#include <EventChoice.h>
#include <EventExNPCMessage.h>
#include <EventITime.h>
#include <EventNPCMessage.h>
#include <EventPerformActions.h>
#include <EventPrompt.h>
#include <ItemDrop.h>
#include <PlasmaSpawn.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>
#include <ServerZoneTrigger.h>
#include <Spawn.h>
#include <SpawnGroup.h>

// Standard C++11 Includes
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>

FindRefWindow::FindRefWindow(MainWindow* pMainWindow, QWidget* pParent)
    : QMainWindow(pParent), mMainWindow(pMainWindow) {
  ui = new Ui::FindRefWindow;
  ui->setupUi(this);

  connect(ui->actionExport, SIGNAL(triggered()), this, SLOT(Export()));

  connect(ui->find, SIGNAL(clicked()), this, SLOT(Find()));
  connect(ui->useZoneDirectory, SIGNAL(toggled(bool)), this,
          SLOT(ToggleZoneDirectory()));
  connect(ui->zoneDirectoryBrowse, SIGNAL(clicked()), this,
          SLOT(SetZoneDirectory()));
}

FindRefWindow::~FindRefWindow() { delete ui; }

bool FindRefWindow::Open(const libcomp::String& objType, uint32_t val) {
  mObjType = objType;

  setWindowTitle(libcomp::String("COMP_hack Cathedral of Content - Find %1")
                     .Arg(objType)
                     .C());

  ui->value->setValue((int32_t)val);
  ui->maxValue->setValue(0);

  ui->results->clear();
  ui->results->setColumnCount(0);
  ui->results->setRowCount(0);
  ui->lblRefs->setText("");
  ui->progressBar->hide();

  // Build the filters for the object type
  mActionFilters.clear();
  mEventFilters.clear();
  mEventConditionFilters.clear();
  mDropSetFilter = nullptr;
  mSpawnFilter = nullptr;
  mZoneFilter = nullptr;
  mZonePartialFilter = nullptr;

  if (mObjType == "DropSet") {
    BuildDropSetFilters();
  } else if (mObjType == "CEventMessageData") {
    BuildCEventMessageDataFilters();
  } else if (mObjType == "CHouraiData") {
    BuildCHouraiDataFilters();
  } else if (mObjType == "CHouraiMessageData") {
    BuildCHouraiMessageDataFilters();
  } else if (mObjType == "CItemData") {
    BuildCItemDataFilters();
  } else if (mObjType == "CKeyItemData") {
    BuildCKeyItemDataFilters();
  } else if (mObjType == "CQuestData") {
    BuildCQuestDataFilters();
  } else if (mObjType == "CSoundData") {
    BuildCSoundDataFilters();
  } else if (mObjType == "CTitleData") {
    BuildCTitleDataFilters();
  } else if (mObjType == "CValuablesData") {
    BuildCValuablesDataFilters();
  } else if (mObjType == "DevilData") {
    BuildDevilDataFilters();
  } else if (mObjType == "hNPCData") {
    BuildHNPCDataFilters();
  } else if (mObjType == "oNPCData") {
    BuildONPCDataFilters();
  } else if (mObjType == "ShopProductData") {
    BuildShopProductDataFilters();
  } else if (mObjType == "StatusData") {
    BuildStatusDataFilters();
  } else if (mObjType == "ZoneData") {
    BuildZoneDataFilters();
  } else {
    // Invalid object type
    return false;
  }

  if (ui->zoneDirectory->text() == "") {
    // Default zone directory to the current zone's if one exists
    auto merged = mMainWindow->GetZones()->GetMergedZone();
    if (merged && !merged->Path.IsEmpty()) {
      QFileInfo f(qs(merged->Path));
      if (f.exists()) {
        ui->zoneDirectory->setText(f.absoluteDir().path());
      }
    }
  }

  show();
  raise();

  return true;
}

void FindRefWindow::closeEvent(QCloseEvent* event) {
  if (!ui->centralwidget->isEnabled()) {
    // If we're currently running the search process, block close events
    event->ignore();
  }
}

void FindRefWindow::BuildDropSetFilters() {
  mActionFilters[objects::Action::ActionType_t::ADD_REMOVE_ITEMS] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionAddRemoveItems>(action);
        if (act->GetFromDropSet()) {
          for (auto& pair : act->GetItems()) {
            ids.insert(pair.first);
          }
        }
      });

  mActionFilters[objects::Action::ActionType_t::CREATE_LOOT] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act = std::dynamic_pointer_cast<objects::ActionCreateLoot>(action);
        for (uint32_t dropSetID : act->GetDropSetIDs()) {
          ids.insert(dropSetID);
        }
      });

  mSpawnFilter = ([](const std::shared_ptr<objects::Spawn>& spawn,
                     std::set<uint32_t>& ids) {
    for (uint32_t dropSetID : spawn->GetDropSetIDs()) {
      ids.insert(dropSetID);
    }

    for (uint32_t dropSetID : spawn->GetGiftSetIDs()) {
      ids.insert(dropSetID);
    }
  });

  mZoneFilter = ([](const std::shared_ptr<objects::ServerZone>& zone,
                    std::set<uint32_t>& ids) {
    for (uint32_t dropSetID : zone->GetDropSetIDs()) {
      ids.insert(dropSetID);
    }

    for (auto pair : zone->GetPlasmaSpawns()) {
      ids.insert(pair.second->GetDropSetID());
    }
  });

  mZonePartialFilter = ([](const std::shared_ptr<objects::ServerZonePartial>& p,
                           std::set<uint32_t>& ids) {
    for (uint32_t dropSetID : p->GetDropSetIDs()) {
      ids.insert(dropSetID);
    }
  });
}

void FindRefWindow::BuildCEventMessageDataFilters() {
  mActionFilters[objects::Action::ActionType_t::DISPLAY_MESSAGE] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionDisplayMessage>(action);
        for (int32_t messageID : act->GetMessageIDs()) {
          ids.insert((uint32_t)messageID);
        }
      });

  mActionFilters[objects::Action::ActionType_t::STAGE_EFFECT] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionStageEffect>(action);
        ids.insert((uint32_t)act->GetMessageID());
      });

  mEventFilters[objects::Event::EventType_t::EX_NPC_MESSAGE] =
      ([](const std::shared_ptr<objects::Event>& event,
          std::set<uint32_t>& ids) {
        auto e = std::dynamic_pointer_cast<objects::EventExNPCMessage>(event);

        ids.insert((uint32_t)e->GetMessageID());
      });

  mEventFilters[objects::Event::EventType_t::NPC_MESSAGE] =
      ([](const std::shared_ptr<objects::Event>& event,
          std::set<uint32_t>& ids) {
        auto e = std::dynamic_pointer_cast<objects::EventNPCMessage>(event);
        for (int32_t messageID : e->GetMessageIDs()) {
          ids.insert((uint32_t)messageID);
        }
      });

  mEventFilters[objects::Event::EventType_t::PROMPT] =
      ([](const std::shared_ptr<objects::Event>& event,
          std::set<uint32_t>& ids) {
        auto e = std::dynamic_pointer_cast<objects::EventPrompt>(event);

        ids.insert((uint32_t)e->GetMessageID());

        for (auto choice : e->GetChoices()) {
          ids.insert((uint32_t)choice->GetMessageID());
        }
      });
}

void FindRefWindow::BuildCHouraiDataFilters() {
  mActionFilters[objects::Action::ActionType_t::UPDATE_POINTS] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionUpdatePoints>(action);
        if (act->GetPointType() ==
            objects::ActionUpdatePoints::PointType_t::ITIME) {
          ids.insert((uint32_t)act->GetModifier());
        }
      });

  mEventFilters[objects::Event::EventType_t::ITIME] =
      ([](const std::shared_ptr<objects::Event>& event,
          std::set<uint32_t>& ids) {
        auto e = std::dynamic_pointer_cast<objects::EventITime>(event);

        ids.insert((uint32_t)e->GetITimeID());
      });
}

void FindRefWindow::BuildCHouraiMessageDataFilters() {
  mEventFilters[objects::Event::EventType_t::ITIME] =
      ([](const std::shared_ptr<objects::Event>& event,
          std::set<uint32_t>& ids) {
        auto e = std::dynamic_pointer_cast<objects::EventITime>(event);

        ids.insert((uint32_t)e->GetMessageID());

        for (auto choice : e->GetChoices()) {
          ids.insert((uint32_t)choice->GetMessageID());
        }
      });
}

void FindRefWindow::BuildCItemDataFilters() {
  mActionFilters[objects::Action::ActionType_t::ADD_REMOVE_ITEMS] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionAddRemoveItems>(action);
        if (act->GetMode() != objects::ActionAddRemoveItems::Mode_t::POST) {
          for (auto& pair : act->GetItems()) {
            ids.insert(pair.first);
          }
        }
      });

  mActionFilters[objects::Action::ActionType_t::CREATE_LOOT] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act = std::dynamic_pointer_cast<objects::ActionCreateLoot>(action);
        for (auto drop : act->GetDrops()) {
          ids.insert(drop->GetItemType());
        }
      });

  mDropSetFilter = ([](const std::shared_ptr<objects::DropSet>& dropset,
                       std::set<uint32_t>& ids) {
    for (auto drop : dropset->GetDrops()) {
      ids.insert(drop->GetItemType());
    }
  });

  mSpawnFilter = ([](const std::shared_ptr<objects::Spawn>& spawn,
                     std::set<uint32_t>& ids) {
    for (auto drop : spawn->GetDrops()) {
      ids.insert(drop->GetItemType());
    }

    for (auto drop : spawn->GetGifts()) {
      ids.insert(drop->GetItemType());
    }
  });

  mEventConditionFilters[objects::EventCondition::Type_t::EQUIPPED] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::ITEM] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::MATERIAL] =
      &FindRefWindow::GetValue1;
}

void FindRefWindow::BuildCKeyItemDataFilters() {
  mActionFilters[objects::Action::ActionType_t::UPDATE_FLAG] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act = std::dynamic_pointer_cast<objects::ActionUpdateFlag>(action);
        if (act->GetFlagType() ==
            objects::ActionUpdateFlag::FlagType_t::PLUGIN) {
          ids.insert((uint32_t)act->GetID());
        }
      });

  mEventConditionFilters[objects::EventCondition::Type_t::PLUGIN] =
      &FindRefWindow::GetValue1;
}

void FindRefWindow::BuildCQuestDataFilters() {
  mActionFilters[objects::Action::ActionType_t::UPDATE_QUEST] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionUpdateQuest>(action);
        ids.insert((uint32_t)act->GetQuestID());
      });

  mEventConditionFilters[objects::EventCondition::Type_t::QUEST_ACTIVE] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::QUEST_AVAILABLE] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::QUEST_COMPLETE] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::QUEST_FLAGS] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::QUEST_PHASE] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters
      [objects::EventCondition::Type_t::QUEST_PHASE_REQUIREMENTS] =
          &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::QUEST_SEQUENCE] =
      &FindRefWindow::GetValue1;
}

void FindRefWindow::BuildCSoundDataFilters() {
  mActionFilters[objects::Action::ActionType_t::PLAY_BGM] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act = std::dynamic_pointer_cast<objects::ActionPlayBGM>(action);
        ids.insert((uint32_t)act->GetMusicID());
      });

  mActionFilters[objects::Action::ActionType_t::PLAY_SOUND_EFFECT] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionPlaySoundEffect>(action);
        ids.insert((uint32_t)act->GetSoundID());
      });
}

void FindRefWindow::BuildCTitleDataFilters() {
  mSpawnFilter =
      ([](const std::shared_ptr<objects::Spawn>& spawn,
          std::set<uint32_t>& ids) { ids.insert(spawn->GetVariantType()); });
}

void FindRefWindow::BuildCValuablesDataFilters() {
  mActionFilters[objects::Action::ActionType_t::UPDATE_FLAG] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act = std::dynamic_pointer_cast<objects::ActionUpdateFlag>(action);
        if (act->GetFlagType() ==
            objects::ActionUpdateFlag::FlagType_t::VALUABLE) {
          ids.insert((uint32_t)act->GetID());
        }
      });

  mEventConditionFilters[objects::EventCondition::Type_t::VALUABLE] =
      &FindRefWindow::GetValue1;
}

void FindRefWindow::BuildDevilDataFilters() {
  mActionFilters[objects::Action::ActionType_t::UPDATE_COMP] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act = std::dynamic_pointer_cast<objects::ActionUpdateCOMP>(action);
        for (auto& pair : act->GetAddDemons()) {
          ids.insert(pair.first);
        }

        for (auto& pair : act->GetRemoveDemons()) {
          ids.insert(pair.first);
        }
      });

  mSpawnFilter =
      ([](const std::shared_ptr<objects::Spawn>& spawn,
          std::set<uint32_t>& ids) { ids.insert(spawn->GetEnemyType()); });

  mEventConditionFilters[objects::EventCondition::Type_t::COMP_DEMON] =
      &FindRefWindow::GetValue1;
  mEventConditionFilters[objects::EventCondition::Type_t::SUMMONED] =
      &FindRefWindow::GetValue1;

  mEventConditionFilters[objects::EventCondition::Type_t::DEMON_BOOK] =
      ([](FindRefWindow& self,
          const std::shared_ptr<objects::EventCondition>& c,
          std::set<uint32_t>& ids) {
        (void)self;

        if (c->GetCompareMode() ==
            objects::EventCondition::CompareMode_t::EXISTS) {
          ids.insert((uint32_t)c->GetValue1());
        }
      });
}

void FindRefWindow::BuildHNPCDataFilters() {
  mZoneFilter = ([](const std::shared_ptr<objects::ServerZone>& zone,
                    std::set<uint32_t>& ids) {
    for (auto npc : zone->GetNPCs()) {
      ids.insert(npc->GetID());
    }
  });

  mZonePartialFilter = ([](const std::shared_ptr<objects::ServerZonePartial>& p,
                           std::set<uint32_t>& ids) {
    for (auto npc : p->GetNPCs()) {
      ids.insert(npc->GetID());
    }
  });
}

void FindRefWindow::BuildONPCDataFilters() {
  mZoneFilter = ([](const std::shared_ptr<objects::ServerZone>& zone,
                    std::set<uint32_t>& ids) {
    for (auto obj : zone->GetObjects()) {
      ids.insert(obj->GetID());
    }
  });

  mZonePartialFilter = ([](const std::shared_ptr<objects::ServerZonePartial>& p,
                           std::set<uint32_t>& ids) {
    for (auto obj : p->GetObjects()) {
      ids.insert(obj->GetID());
    }
  });
}

void FindRefWindow::BuildShopProductDataFilters() {
  mActionFilters[objects::Action::ActionType_t::ADD_REMOVE_ITEMS] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionAddRemoveItems>(action);
        if (act->GetMode() == objects::ActionAddRemoveItems::Mode_t::POST) {
          for (auto& pair : act->GetItems()) {
            ids.insert(pair.first);
          }
        }
      });
}

void FindRefWindow::BuildStatusDataFilters() {
  mActionFilters[objects::Action::ActionType_t::ADD_REMOVE_STATUS] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionAddRemoveStatus>(action);
        for (auto& pair : act->GetStatusStacks()) {
          ids.insert(pair.first);
        }

        for (auto& pair : act->GetStatusTimes()) {
          ids.insert(pair.first);
        }
      });

  mEventConditionFilters[objects::EventCondition::Type_t::STATUS_ACTIVE] =
      &FindRefWindow::GetValue1;
}

void FindRefWindow::BuildZoneDataFilters() {
  mActionFilters[objects::Action::ActionType_t::SET_HOMEPOINT] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act =
            std::dynamic_pointer_cast<objects::ActionSetHomepoint>(action);
        ids.insert(act->GetZoneID());
      });

  mActionFilters[objects::Action::ActionType_t::ZONE_CHANGE] =
      ([](const std::shared_ptr<objects::Action>& action,
          std::set<uint32_t>& ids) {
        auto act = std::dynamic_pointer_cast<objects::ActionZoneChange>(action);
        ids.insert(act->GetZoneID());
      });

  mEventConditionFilters[objects::EventCondition::Type_t::CLAN_HOME] =
      &FindRefWindow::GetValue1;
}

void FindRefWindow::GetValue1(const std::shared_ptr<objects::EventCondition>& c,
                              std::set<uint32_t>& ids) {
  ids.insert((uint32_t)c->GetValue1());
}

void FindRefWindow::Export() {
  QString qPath = QFileDialog::getSaveFileName(
      this, tr("Export tabular file"), mMainWindow->GetDialogDirectory(),
      tr("Text file (*.txt)"));
  if (qPath.isEmpty()) {
    return;
  }

  std::ofstream out;
  out.open(qPath.toUtf8().constData());

  int colCount = ui->results->columnCount();
  for (int colIdx = 0; colIdx < colCount; colIdx++) {
    if (colIdx != 0) {
      out << "\t";
    }

    out << ui->results->horizontalHeaderItem(colIdx)
               ->text()
               .toUtf8()
               .constData();
  }

  for (int rowIdx = 0; rowIdx < ui->results->rowCount(); rowIdx++) {
    for (int colIdx = 0; colIdx < colCount; colIdx++) {
      if (colIdx != 0) {
        out << "\t";
      } else {
        out << std::endl;
      }

      auto item = ui->results->item(rowIdx, colIdx);
      if (item) {
        auto txt = cs(item->text());
        if (!txt.IsEmpty()) {
          out << txt.C();
        }
      }
    }
  }

  if (!out.good()) {
    QMessageBox err;
    err.setText(
        QString("Failed to save file: %1").arg(qPath.toUtf8().constData()));
    err.exec();
  }
}

void FindRefWindow::Find() {
  uint32_t val = (uint32_t)ui->value->value();
  uint32_t maxVal = (uint32_t)ui->maxValue->value();

  if (maxVal && val > maxVal) {
    QMessageBox err;
    err.setText("Max value must be zero or not less than the value.");
    err.exec();

    return;
  }

  ui->progressBar->show();
  ui->centralwidget->setDisabled(true);
  ui->menubar->setDisabled(true);

  std::future<void> f = std::async(&FindRefWindow::FindAsync, this);

  std::chrono::milliseconds span(10);
  while (f.wait_for(span) == std::future_status::timeout) {
    // Actively wait by calling the event processor
    QApplication::processEvents();
  }

  ui->progressBar->hide();
  ui->centralwidget->setDisabled(false);
  ui->menubar->setDisabled(false);
}

void FindRefWindow::SetZoneDirectory() {
  auto defaultDirectory = ui->zoneDirectory->text();
  if (defaultDirectory.isEmpty()) {
    defaultDirectory = mMainWindow->GetDialogDirectory();
  }

  QString qPath = QFileDialog::getExistingDirectory(
      this, tr("Set Zone XML folder"), defaultDirectory);
  if (qPath.isEmpty()) {
    return;
  }

  ui->zoneDirectory->setText(qPath);
}

void FindRefWindow::ToggleZoneDirectory() {
  if (ui->useZoneDirectory->isChecked()) {
    ui->zoneDirectory->setDisabled(false);
    ui->zoneDirectoryBrowse->setDisabled(false);
  } else {
    ui->zoneDirectory->setDisabled(true);
    ui->zoneDirectoryBrowse->setDisabled(true);
  }
}

void FindRefWindow::FindAsync() {
  uint32_t val = (uint32_t)ui->value->value();
  uint32_t maxVal = (uint32_t)ui->maxValue->value();

  ui->results->clear();
  ui->results->setColumnCount(0);
  ui->results->setRowCount(0);
  ui->lblRefs->setText("");

  ui->results->setColumnCount(3);
  ui->results->setHorizontalHeaderItem(0, new QTableWidgetItem("Value"));
  ui->results->setHorizontalHeaderItem(1, new QTableWidgetItem("Location"));
  ui->results->setHorizontalHeaderItem(2, new QTableWidgetItem("Section"));

  auto eventWindow = mMainWindow->GetEvents();
  auto zoneWindow = mMainWindow->GetZones();

  size_t eventFileCount = 0;
  size_t zoneFileCount = 0;
  size_t partialFileCount = 0;

  int row = 0;
  if (mEventFilters.size() > 0 || mEventConditionFilters.size() > 0 ||
      mActionFilters.size() > 0) {
    // Search events (and event actions)
    std::list<libcomp::String> eventFiles;
    if (ui->radModeEventCurrentOnly->isChecked()) {
      auto current = eventWindow->GetCurrentFile();
      if (!current.IsEmpty()) {
        eventFiles.push_back(current);
      }
    } else {
      eventFiles = eventWindow->GetCurrentFiles();
    }

    for (auto path : eventFiles) {
      auto events = eventWindow->GetFileEvents(path);
      for (auto e : events) {
        std::set<uint32_t> ids;

        auto eIter = mEventFilters.find(e->GetEventType());
        if (eIter != mEventFilters.end()) {
          eIter->second(e, ids);
        }

        // Gather all conditions and actions
        auto conditions = e->GetConditions();
        for (auto b : e->GetBranches()) {
          for (auto c : b->GetConditions()) {
            conditions.push_back(c);
          }
        }

        std::list<std::shared_ptr<objects::Action>> actions;
        switch (e->GetEventType()) {
          case objects::Event::EventType_t::PERFORM_ACTIONS:
            actions = std::dynamic_pointer_cast<objects::EventPerformActions>(e)
                          ->GetActions();
            break;
          case objects::Event::EventType_t::PROMPT:
            for (auto choice :
                 std::dynamic_pointer_cast<objects::EventPrompt>(e)
                     ->GetChoices()) {
              for (auto c : choice->GetConditions()) {
                conditions.push_back(c);
              }
            }
            break;
          default:
            break;
        }

        for (auto c : conditions) {
          auto cIter = mEventConditionFilters.find(c->GetType());
          if (cIter != mEventConditionFilters.end()) {
            cIter->second(*this, c, ids);
          }
        }

        FilterActionIDs(actions, ids);

        bool refFound = false;
        for (uint32_t id : GetFilteredIDs(ids, val, maxVal)) {
          AddResult(id, path, libcomp::String("Event %1").Arg(e->GetID()));
          refFound = true;
        }

        if (refFound) {
          eventFileCount++;
        }
      }
    }
  }

  if (!ui->radModeEventCurrentOnly->isChecked()) {
    if (mZoneFilter || mSpawnFilter || mActionFilters.size() > 0) {
      // Search zones
      std::unordered_map<libcomp::String, std::shared_ptr<objects::ServerZone>>
          zoneFiles;
      auto merged = zoneWindow->GetMergedZone();
      if (merged && merged->CurrentZone) {
        zoneFiles[merged->Path] = merged->CurrentZone;
      }

      if (ui->useZoneDirectory->isChecked() &&
          !ui->zoneDirectory->text().isEmpty()) {
        QDirIterator it(ui->zoneDirectory->text(), QStringList() << "*.xml",
                        QDir::Files);
        while (it.hasNext()) {
          libcomp::String path = cs(it.next());
          if (zoneFiles.find(path) == zoneFiles.end()) {
            auto zone = zoneWindow->LoadZoneFromFile(path);
            if (zone) {
              zoneFiles[path] = zone;
            }
          }
        }
      }

      for (auto& zonePair : zoneFiles) {
        auto zone = zonePair.second;

        std::unordered_map<libcomp::String, std::set<uint32_t>> ids;
        if (mZoneFilter) {
          mZoneFilter(zone, ids[""]);
        }

        if (mSpawnFilter) {
          for (auto& spawnPair : zone->GetSpawns()) {
            auto spawnID = libcomp::String("Spawn %1").Arg(spawnPair.first);
            mSpawnFilter(spawnPair.second, ids[spawnID]);
          }
        }

        if (mActionFilters.size() > 0) {
          for (auto npc : zone->GetNPCs()) {
            auto npcID = libcomp::String("NPC %1").Arg(npc->GetID());

            auto actions = npc->GetActions();
            FilterActionIDs(actions, ids[npcID]);
          }

          for (auto obj : zone->GetObjects()) {
            auto objID = libcomp::String("Object %1").Arg(obj->GetID());

            auto actions = obj->GetActions();
            FilterActionIDs(actions, ids[objID]);
          }

          for (auto& sgPair : zone->GetSpawnGroups()) {
            auto sgID = libcomp::String("Spawn Group %1").Arg(sgPair.first);

            auto actions = sgPair.second->GetSpawnActions();
            FilterActionIDs(actions, ids[sgID]);

            actions = sgPair.second->GetDefeatActions();
            FilterActionIDs(actions, ids[sgID]);
          }

          for (auto& pPair : zone->GetPlasmaSpawns()) {
            auto plasmaID = libcomp::String("Plasma %1").Arg(pPair.first);

            auto actions = pPair.second->GetSuccessActions();
            FilterActionIDs(actions, ids[plasmaID]);

            actions = pPair.second->GetFailActions();
            FilterActionIDs(actions, ids[plasmaID]);
          }

          for (auto& spotPair : zone->GetSpots()) {
            auto spotID = libcomp::String("Spot %1").Arg(spotPair.first);

            auto actions = spotPair.second->GetActions();
            FilterActionIDs(actions, ids[spotID]);

            actions = spotPair.second->GetLeaveActions();
            FilterActionIDs(actions, ids[spotID]);
          }

          for (auto trigger : zone->GetTriggers()) {
            auto actions = trigger->GetActions();
            FilterActionIDs(actions, ids["Trigger"]);
          }
        }

        bool refFound = false;
        for (auto& idPair : ids) {
          for (uint32_t id : GetFilteredIDs(idPair.second, val, maxVal)) {
            AddResult(id,
                      libcomp::String("Zone %1 (%2)")
                          .Arg(zone->GetID())
                          .Arg(zone->GetDynamicMapID()),
                      idPair.first);
            refFound = true;
          }
        }

        if (refFound) {
          zoneFileCount++;
        }
      }
    }

    if (mZonePartialFilter || mSpawnFilter || mActionFilters.size() > 0) {
      // Search zone partials
      for (auto& partialPair : zoneWindow->GetLoadedPartials()) {
        auto partial = partialPair.second;

        std::unordered_map<libcomp::String, std::set<uint32_t>> ids;
        if (mZonePartialFilter) {
          mZonePartialFilter(partial, ids[""]);
        }

        if (mSpawnFilter) {
          for (auto& spawnPair : partial->GetSpawns()) {
            auto spawnID = libcomp::String("Spawn %1").Arg(spawnPair.first);
            mSpawnFilter(spawnPair.second, ids[spawnID]);
          }
        }

        if (mActionFilters.size() > 0) {
          for (auto npc : partial->GetNPCs()) {
            auto npcID = libcomp::String("NPC %1").Arg(npc->GetID());

            auto actions = npc->GetActions();
            FilterActionIDs(actions, ids[npcID]);
          }

          for (auto obj : partial->GetObjects()) {
            auto objID = libcomp::String("Object %1").Arg(obj->GetID());

            auto actions = obj->GetActions();
            FilterActionIDs(actions, ids[objID]);
          }

          for (auto& sgPair : partial->GetSpawnGroups()) {
            auto sgID = libcomp::String("Spawn Group %1").Arg(sgPair.first);

            auto actions = sgPair.second->GetSpawnActions();
            FilterActionIDs(actions, ids[sgID]);

            actions = sgPair.second->GetDefeatActions();
            FilterActionIDs(actions, ids[sgID]);
          }

          for (auto& spotPair : partial->GetSpots()) {
            auto spotID = libcomp::String("Spot %1").Arg(spotPair.first);

            auto actions = spotPair.second->GetActions();
            FilterActionIDs(actions, ids[spotID]);

            actions = spotPair.second->GetLeaveActions();
            FilterActionIDs(actions, ids[spotID]);
          }

          for (auto trigger : partial->GetTriggers()) {
            auto actions = trigger->GetActions();
            FilterActionIDs(actions, ids["Trigger"]);
          }
        }

        bool refFound = false;
        for (auto& idPair : ids) {
          for (uint32_t id : GetFilteredIDs(idPair.second, val, maxVal)) {
            AddResult(id,
                      libcomp::String("Zone Partial %1").Arg(partial->GetID()),
                      idPair.first);
            refFound = true;
          }
        }

        if (refFound) {
          partialFileCount++;
        }
      }
    }

    if (mDropSetFilter) {
      // Search drop sets
      auto dataset = mMainWindow->GetBinaryDataSet("DropSet");
      if (dataset) {
        for (auto obj : dataset->GetObjects()) {
          auto ds = std::dynamic_pointer_cast<objects::DropSet>(obj);

          std::set<uint32_t> ids;
          mDropSetFilter(ds, ids);

          for (uint32_t id : GetFilteredIDs(ids, val, maxVal)) {
            AddResult(id, libcomp::String("Drop Set %1").Arg(ds->GetID()), "");
          }
        }
      }
    }
  }

  if (ui->includeText->isChecked()) {
    // Double back and add text
    ui->results->setColumnCount(4);
    ui->results->setHorizontalHeaderItem(3, new QTableWidgetItem("Text"));

    auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet(mObjType));
    if (dataset) {
      for (int rowIdx = 0; rowIdx < ui->results->rowCount(); rowIdx++) {
        uint32_t id = (uint32_t)ui->results->item(rowIdx, 0)->text().toUInt();
        auto name = dataset->GetName(dataset->GetObjectByID(id));

        if (!name.IsEmpty()) {
          ui->results->setItem(rowIdx, 3, new QTableWidgetItem(qs(name)));
        }
      }
    }
  }

  // Hide the first column if only one value is being searched for then
  // resize the columns
  ui->results->setColumnHidden(0, maxVal == 0);
  ui->results->resizeColumnsToContents();

  if (ui->results->rowCount()) {
    ui->lblRefs->setText(
        QString("%1 reference(s) found in %2 file(s)")
            .arg(row)
            .arg(eventFileCount + zoneFileCount + partialFileCount));
  } else {
    ui->lblRefs->setText("No references found");
  }
}

void FindRefWindow::AddResult(uint32_t id, const libcomp::String& location,
                              const libcomp::String& section) {
  int row = ui->results->rowCount();

  ui->results->setRowCount(++row);
  ui->results->setItem(row - 1, 0, new QTableWidgetItem(QString::number(id)));

  if (!location.IsEmpty()) {
    ui->results->setItem(row - 1, 1, new QTableWidgetItem(qs(location)));
  }

  if (!section.IsEmpty()) {
    ui->results->setItem(row - 1, 2, new QTableWidgetItem(qs(section)));
  }
}

void FindRefWindow::FilterActionIDs(
    const std::list<std::shared_ptr<objects::Action>>& actions,
    std::set<uint32_t>& ids) {
  auto currentActions = actions;

  std::list<std::shared_ptr<objects::Action>> newActions;
  while (currentActions.size() > 0) {
    // Actions can't nest forever so loop until we're done
    for (auto action : currentActions) {
      auto aIter = mActionFilters.find(action->GetActionType());
      if (aIter != mActionFilters.end()) {
        aIter->second(action, ids);
      }

      switch (action->GetActionType()) {
        case objects::Action::ActionType_t::DELAY: {
          auto act = std::dynamic_pointer_cast<objects::ActionDelay>(action);
          for (auto act2 : act->GetActions()) {
            newActions.push_back(act2);
          }
        } break;
        case objects::Action::ActionType_t::SPAWN: {
          auto act = std::dynamic_pointer_cast<objects::ActionSpawn>(action);
          for (auto act2 : act->GetDefeatActions()) {
            newActions.push_back(act2);
          }
        } break;
        default:
          break;
      }
    }

    currentActions = newActions;
    newActions.clear();
  }
}

std::set<uint32_t> FindRefWindow::GetFilteredIDs(const std::set<uint32_t>& ids,
                                                 uint32_t value,
                                                 uint32_t maxValue) {
  std::set<uint32_t> result;

  if (!maxValue) {
    // Check if the (only) value is in the set
    if (ids.find(value) != ids.end()) {
      result.insert(value);
    }
  } else {
    // Gather all IDs between min and max in the set
    for (uint32_t val = value; val <= maxValue; val++) {
      if (ids.find(val) != ids.end()) {
        result.insert(val);
      }
    }
  }

  return result;
}
