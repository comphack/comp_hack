/**
 * @file tools/cathedral/src/ZoneWindow.cpp
 * @ingroup map
 *
 * @author HACKfrost
 *
 * @brief Zone window which allows for visualization and modification of zone
 *  map data.
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

#include "ZoneWindow.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "MainWindow.h"
#include "XmlHandler.h"
#include "ZonePartialSelector.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>
#include <ui_SpotProperties.h>

#include <QDirIterator>
#include <QFileDialog>
#include <QIODevice>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPicture>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QToolTip>

// object Includes
#include <MiCTitleData.h>
#include <MiDevilData.h>
#include <MiGrowthData.h>
#include <MiNPCBasicData.h>
#include <MiSpotData.h>
#include <MiZoneData.h>
#include <MiZoneFileData.h>
#include <QmpBoundary.h>
#include <QmpBoundaryLine.h>
#include <QmpElement.h>
#include <QmpFile.h>
#include <QmpNavPoint.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>
#include <ServerZoneTrigger.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>
#include <SpawnRestriction.h>

// C++11 Standard Includes
#include <cmath>
#include <map>

// libcomp Includes
#include <Constants.h>
#include <Log.h>
#include <ServerDataManager.h>

const QColor COLOR_SELECTED = Qt::red;
const QColor COLOR_PLAYER = Qt::magenta;
const QColor COLOR_NPC = Qt::darkRed;
const QColor COLOR_OBJECT = Qt::blue;
const QColor COLOR_SPAWN_LOC = Qt::green;
const QColor COLOR_SPOT = Qt::darkGreen;

const QColor COLOR_BOUNDARY = Qt::darkMagenta;
const QColor COLOR_NAVPOINT = Qt::darkBlue;

// Barrier colors
const QColor COLOR_GENERIC = Qt::black;
const QColor COLOR_1WAY = Qt::darkGray;
const QColor COLOR_TOGGLE1 = Qt::darkYellow;
const QColor COLOR_TOGGLE2 = Qt::darkCyan;

// Custom comparator for QPointF hashing
struct QPointFHash {
  bool operator()(const QPointF& lhs, const QPointF& rhs) const {
    return lhs.x() < rhs.x() || (lhs.x() == rhs.x() && lhs.y() < rhs.y());
  }
};

ZoneWindow::ZoneWindow(MainWindow* pMainWindow, QWidget* p)
    : QMainWindow(p),
      mMainWindow(pMainWindow),
      mOffsetX(0),
      mOffsetY(0),
      mDragging(false),
      mExternalQmpFile(false) {
  ui.setupUi(this);

  mMergedZone = std::make_shared<MergedZone>();

  ui.npcs->Bind(pMainWindow, true);
  ui.objects->Bind(pMainWindow, false);
  ui.spawns->SetMainWindow(pMainWindow);
  ui.spawnGroups->SetMainWindow(pMainWindow);
  ui.spawnLocationGroups->SetMainWindow(pMainWindow);
  ui.spots->SetMainWindow(pMainWindow);

  ui.zoneID->BindSelector(pMainWindow, "ZoneData");

  ui.validTeamTypes->Setup(DynamicItemType_t::PRIMITIVE_INT, pMainWindow);

  ui.dropSetIDs->Setup(DynamicItemType_t::COMPLEX_OBJECT_SELECTOR, pMainWindow,
                       "DropSet", true);
  ui.dropSetIDs->SetAddText("Add Drop Set");

  ui.skillBlacklist->Setup(DynamicItemType_t::PRIMITIVE_UINT, pMainWindow);
  ui.skillBlacklist->SetAddText("Add Skill");

  ui.skillWhitelist->Setup(DynamicItemType_t::PRIMITIVE_UINT, pMainWindow);
  ui.skillWhitelist->SetAddText("Add Skill");

  ui.triggers->Setup(DynamicItemType_t::OBJ_ZONE_TRIGGER, pMainWindow);
  ui.triggers->SetAddText("Add Trigger");

  ui.partialDynamicMapIDs->Setup(DynamicItemType_t::PRIMITIVE_UINT,
                                 pMainWindow);

  connect(ui.actionRefresh, SIGNAL(triggered()), this, SLOT(Refresh()));

  connect(ui.actionShowNPCs, SIGNAL(toggled(bool)), this,
          SLOT(ShowToggled(bool)));
  connect(ui.actionShowObjects, SIGNAL(toggled(bool)), this,
          SLOT(ShowToggled(bool)));
  connect(ui.actionShowBoundaries, SIGNAL(toggled(bool)), this,
          SLOT(ShowToggled(bool)));
  connect(ui.actionShowNavPoints, SIGNAL(toggled(bool)), this,
          SLOT(ShowToggled(bool)));

  connect(ui.addNPC, SIGNAL(clicked()), this, SLOT(AddNPC()));
  connect(ui.addObject, SIGNAL(clicked()), this, SLOT(AddObject()));
  connect(ui.addSpawn, SIGNAL(clicked()), this, SLOT(AddSpawn()));
  connect(ui.cloneSpawn, SIGNAL(clicked()), this, SLOT(CloneSpawn()));
  connect(ui.removeNPC, SIGNAL(clicked()), this, SLOT(RemoveNPC()));
  connect(ui.removeObject, SIGNAL(clicked()), this, SLOT(RemoveObject()));
  connect(ui.removeSpawn, SIGNAL(clicked()), this, SLOT(RemoveSpawn()));
  connect(ui.btnQmpReset, SIGNAL(clicked()), this, SLOT(ResetQmpFile()));
  connect(ui.btnQmpResetBoundaries, SIGNAL(clicked()), this,
          SLOT(ResetBoundaries()));
  connect(ui.btnQmpResetNavPoints, SIGNAL(clicked()), this,
          SLOT(ResetNavPoints()));

  connect(ui.actionLoad, SIGNAL(triggered()), this, SLOT(LoadZoneFile()));
  connect(ui.actionLoadQmp, SIGNAL(triggered()), this, SLOT(LoadQmpFile()));
  connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(SaveFile()));
  connect(ui.actionSaveAll, SIGNAL(triggered()), this, SLOT(SaveAllFiles()));
  connect(ui.actionSaveQmp, SIGNAL(triggered()), this, SLOT(SaveQmpFile()));

  connect(ui.actionPartialsLoadFile, SIGNAL(triggered()), this,
          SLOT(LoadPartialFile()));
  connect(ui.actionPartialsLoadDirectory, SIGNAL(triggered()), this,
          SLOT(LoadPartialDirectory()));
  connect(ui.actionPartialsApply, SIGNAL(triggered()), this,
          SLOT(ApplyPartials()));

  connect(ui.tabs, SIGNAL(currentChanged(int)), this, SLOT(MainTabChanged()));
  connect(ui.npcs, SIGNAL(selectedObjectChanged()), this,
          SLOT(SelectListObject()));
  connect(ui.objects, SIGNAL(selectedObjectChanged()), this,
          SLOT(SelectListObject()));
  connect(ui.spawns, SIGNAL(selectedObjectChanged()), this,
          SLOT(SelectListObject()));
  connect(ui.spawnGroups, SIGNAL(selectedObjectChanged()), this,
          SLOT(SelectListObject()));
  connect(ui.spawnLocationGroups, SIGNAL(selectedObjectChanged()), this,
          SLOT(SelectListObject()));
  connect(ui.spots, SIGNAL(selectedObjectChanged()), this,
          SLOT(SelectListObject()));

  connect(ui.npcs, SIGNAL(objectMoved(std::shared_ptr<libcomp::Object>, bool)),
          this, SLOT(NPCMoved(std::shared_ptr<libcomp::Object>, bool)));
  connect(ui.objects,
          SIGNAL(objectMoved(std::shared_ptr<libcomp::Object>, bool)), this,
          SLOT(ObjectMoved(std::shared_ptr<libcomp::Object>, bool)));

  connect(ui.zoneView, SIGNAL(currentIndexChanged(const QString&)), this,
          SLOT(ZoneViewUpdated()));
  connect(ui.tabSpawnTypes, SIGNAL(currentChanged(int)), this,
          SLOT(SpawnTabChanged()));
  connect(ui.zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(Zoom()));

  connect(
      ui.treeBoundaries->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
      this, SLOT(GeometrySelectionChanged()));
  connect(
      ui.tblNavPoints->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
      this, SLOT(GeometrySelectionChanged()));

  // Override the standard scroll behavior for the map scroll area
  ui.mapScrollArea->installEventFilter(this);
  ui.mapScrollArea->horizontalScrollBar()->installEventFilter(this);
  ui.mapScrollArea->verticalScrollBar()->installEventFilter(this);
}

ZoneWindow::~ZoneWindow() {}

std::shared_ptr<MergedZone> ZoneWindow::GetMergedZone() const {
  return mMergedZone;
}

std::map<uint32_t, std::shared_ptr<objects::ServerZonePartial>>
ZoneWindow::GetLoadedPartials() const {
  return mZonePartials;
}

std::set<uint32_t> ZoneWindow::GetSelectedPartials() const {
  return mSelectedPartials;
}

bool ZoneWindow::ShowZone() {
  auto zone = mMergedZone->CurrentZone;
  if (!zone) {
    LogGeneralErrorMsg("No zone currently loaded\n");
    return false;
  }

  // Don't bother showing the bazaar settings if none are configured
  if (zone->BazaarsCount() == 0) {
    ui.grpBazaar->hide();
  } else {
    ui.grpBazaar->show();
  }

  mSelectedPartials.clear();
  ResetAppliedPartials();

  UpdateMergedZone(false);

  LoadProperties();

  setWindowTitle(
      libcomp::String("COMP_hack Cathedral of Content - Zone %1 (%2)")
          .Arg(zone->GetID())
          .Arg(zone->GetDynamicMapID())
          .C());

  if (LoadMapFromZone()) {
    show();
    return true;
  }

  return false;
}

void ZoneWindow::RebuildNamedDataSet(const libcomp::String& objType) {
  std::vector<libcomp::String> names;
  if (objType == "Actor") {
    auto hnpcDataSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet("hNPCData"));
    auto onpcDataSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet("oNPCData"));

    std::map<int32_t, std::shared_ptr<objects::ServerObject>> actorMap;
    for (auto npc : mMergedZone->Definition->GetNPCs()) {
      if (npc->GetActorID() &&
          actorMap.find(npc->GetActorID()) == actorMap.end()) {
        actorMap[npc->GetActorID()] = npc;
      }
    }

    for (auto obj : mMergedZone->Definition->GetObjects()) {
      if (obj->GetActorID() &&
          actorMap.find(obj->GetActorID()) == actorMap.end()) {
        actorMap[obj->GetActorID()] = obj;
      }
    }

    std::vector<std::shared_ptr<libcomp::Object>> actors;
    for (auto& aPair : actorMap) {
      auto sObj = aPair.second;
      auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(sObj);

      libcomp::String name;
      if (npc) {
        name = hnpcDataSet->GetName(hnpcDataSet->GetObjectByID(npc->GetID()));
        name = libcomp::String("%1 [%2:H]")
                   .Arg(!name.IsEmpty() ? name : "[Unnamed]")
                   .Arg(npc->GetID());
      } else {
        name = onpcDataSet->GetName(onpcDataSet->GetObjectByID(sObj->GetID()));
        name = libcomp::String("%1 [%2:O]")
                   .Arg(!name.IsEmpty() ? name : "[Unnamed]")
                   .Arg(sObj->GetID());
      }

      actors.push_back(sObj);
      names.push_back(name);
    }

    auto newData = std::make_shared<BinaryDataNamedSet>(
        [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t {
          return (uint32_t)std::dynamic_pointer_cast<objects::ServerObject>(obj)
              ->GetActorID();
        });
    newData->MapRecords(actors, names);
    mMainWindow->RegisterBinaryDataSet("Actor", newData);
  } else if (objType == "Spawn") {
    auto devilDataSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet("DevilData"));
    auto titleDataSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet("CTitleData"));

    std::map<uint32_t, std::shared_ptr<objects::Spawn>> sort;
    for (auto& sPair : mMergedZone->Definition->GetSpawns()) {
      sort[sPair.first] = sPair.second;
    }

    std::vector<std::shared_ptr<libcomp::Object>> spawns;
    for (auto& sPair : sort) {
      auto spawn = sPair.second;
      auto devilData = std::dynamic_pointer_cast<objects::MiDevilData>(
          devilDataSet->GetObjectByID(spawn->GetEnemyType()));

      libcomp::String name(devilData ? devilDataSet->GetName(devilData)
                                     : "[Unknown]");

      uint32_t titleID =
          spawn->GetVariantType()
              ? spawn->GetVariantType()
              : (devilData ? (uint32_t)devilData->GetBasic()->GetTitle() : 0);
      if (titleID) {
        auto title = std::dynamic_pointer_cast<objects::MiCTitleData>(
            titleDataSet->GetObjectByID(titleID));
        if (title) {
          name = libcomp::String("%1 %2").Arg(title->GetTitle()).Arg(name);
        }
      }

      int8_t lvl = spawn->GetLevel();
      if (lvl == -1 && devilData) {
        lvl = (int8_t)devilData->GetGrowth()->GetBaseLevel();
      }

      name = libcomp::String("%1 Lv:%2").Arg(name).Arg(lvl);

      if (spawn->GetCategory() == objects::Spawn::Category_t::ALLY) {
        name = libcomp::String("%1 [Ally]").Arg(name);
      }

      spawns.push_back(spawn);
      names.push_back(name);
    }

    auto newData = std::make_shared<BinaryDataNamedSet>(
        [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t {
          return std::dynamic_pointer_cast<objects::Spawn>(obj)->GetID();
        });
    newData->MapRecords(spawns, names);
    mMainWindow->RegisterBinaryDataSet("Spawn", newData);
  } else if (objType == "SpawnGroup") {
    auto spawnSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet("Spawn"));

    std::map<uint32_t, std::shared_ptr<objects::SpawnGroup>> sort;
    for (auto& sPair : mMergedZone->Definition->GetSpawnGroups()) {
      sort[sPair.first] = sPair.second;
    }

    std::vector<std::shared_ptr<libcomp::Object>> sgs;
    for (auto& sgPair : sort) {
      std::list<libcomp::String> spawnStrings;

      auto sg = sgPair.second;
      for (auto& spawnPair : sg->GetSpawns()) {
        auto spawn = spawnSet->GetObjectByID(spawnPair.first);
        libcomp::String txt(spawn ? spawnSet->GetName(spawn) : "[Unknown]");
        spawnStrings.push_back(libcomp::String("%1 x%2 [%3]")
                                   .Arg(txt)
                                   .Arg(spawnPair.second)
                                   .Arg(spawnPair.first));
      }

      sgs.push_back(sg);
      names.push_back(libcomp::String::Join(spawnStrings, ",\n\r    "));
    }

    auto newData = std::make_shared<BinaryDataNamedSet>(
        [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t {
          return std::dynamic_pointer_cast<objects::SpawnGroup>(obj)->GetID();
        });
    newData->MapRecords(sgs, names);
    mMainWindow->RegisterBinaryDataSet("SpawnGroup", newData);
  } else if (objType == "SpawnLocationGroup") {
    auto sgSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet("SpawnGroup"));

    std::map<uint32_t, std::shared_ptr<objects::SpawnLocationGroup>> sort;
    for (auto& sPair : mMergedZone->Definition->GetSpawnLocationGroups()) {
      sort[sPair.first] = sPair.second;
    }

    std::vector<std::shared_ptr<libcomp::Object>> slgs;
    for (auto& slgPair : mMergedZone->Definition->GetSpawnLocationGroups()) {
      std::list<libcomp::String> sgStrings;

      auto slg = slgPair.second;
      for (uint32_t sgID : slg->GetGroupIDs()) {
        auto sg = sgSet->GetObjectByID(sgID);
        libcomp::String txt(sg ? sgSet->GetName(sg).Replace("\n\r", "")
                               : "[Unknown]");
        sgStrings.push_back(libcomp::String("{ %1 } @%2").Arg(txt).Arg(sgID));
      }

      slgs.push_back(slg);
      names.push_back(libcomp::String::Join(sgStrings, ",\n\r    "));
    }

    auto newData = std::make_shared<BinaryDataNamedSet>(
        [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t {
          return std::dynamic_pointer_cast<objects::SpawnLocationGroup>(obj)
              ->GetID();
        });
    newData->MapRecords(slgs, names);
    mMainWindow->RegisterBinaryDataSet("SpawnLocationGroup", newData);
  }
}

std::list<std::shared_ptr<objects::Action>> ZoneWindow::GetLoadedActions(
    bool forUpdate) {
  std::list<std::shared_ptr<objects::Action>> actions;
  if (!mMergedZone->Definition) {
    // Nothing loaded
    return actions;
  }

  if (forUpdate) {
    // Make sure all controls are saved and not bound during the update
    Refresh();
  }

  // Get all loaded partial actions
  for (auto& partialPair : mZonePartials) {
    auto partial = partialPair.second;
    for (auto npc : partial->GetNPCs()) {
      for (auto action : npc->GetActions()) {
        actions.push_back(action);
      }
    }

    for (auto obj : partial->GetObjects()) {
      for (auto action : obj->GetActions()) {
        actions.push_back(action);
      }
    }

    for (auto& sgPair : partial->GetSpawnGroups()) {
      for (auto action : sgPair.second->GetSpawnActions()) {
        actions.push_back(action);
      }

      for (auto action : sgPair.second->GetDefeatActions()) {
        actions.push_back(action);
      }
    }

    for (auto& spotPair : partial->GetSpots()) {
      for (auto action : spotPair.second->GetActions()) {
        actions.push_back(action);
      }
    }

    for (auto trigger : partial->GetTriggers()) {
      for (auto action : trigger->GetActions()) {
        actions.push_back(action);
      }
    }
  }

  // Get all current zone actions
  if (mMergedZone->CurrentZone) {
    auto zone = mMergedZone->CurrentZone;
    for (auto npc : zone->GetNPCs()) {
      for (auto action : npc->GetActions()) {
        actions.push_back(action);
      }
    }

    for (auto obj : zone->GetObjects()) {
      for (auto action : obj->GetActions()) {
        actions.push_back(action);
      }
    }

    for (auto& sgPair : zone->GetSpawnGroups()) {
      for (auto action : sgPair.second->GetSpawnActions()) {
        actions.push_back(action);
      }

      for (auto action : sgPair.second->GetDefeatActions()) {
        actions.push_back(action);
      }
    }

    for (auto& spotPair : zone->GetSpots()) {
      for (auto action : spotPair.second->GetActions()) {
        actions.push_back(action);
      }
    }

    for (auto trigger : zone->GetTriggers()) {
      for (auto action : trigger->GetActions()) {
        actions.push_back(action);
      }
    }
  }

  return actions;
}

bool ZoneWindow::ShowSpot(uint32_t spotID) {
  // Check if the spot exists and error if it does not
  uint32_t dynamicMapID = mMergedZone->CurrentZone
                              ? mMergedZone->CurrentZone->GetDynamicMapID()
                              : 0;
  auto definitions = mMainWindow->GetDefinitions();

  auto spots = definitions->GetSpotData(dynamicMapID);
  auto spotIter = spots.find(spotID);
  if (spotIter == spots.end()) {
    QMessageBox err;
    err.setText(QString("Spot %1 is not currently loaded.").arg(spotID));
    err.exec();

    return false;
  }

  // Select the spots tab and select the object
  if (ui.tabs->currentIndex() != 4) {
    ui.tabs->setCurrentIndex(4);
  }

  ui.spots->Select(spotIter->second);

  return true;
}

std::shared_ptr<objects::ServerZone> ZoneWindow::LoadZoneFromFile(
    const libcomp::String& path) {
  tinyxml2::XMLDocument doc;
  if (tinyxml2::XML_SUCCESS != doc.LoadFile(path.C())) {
    LogGeneralError([&]() {
      return libcomp::String("Failed to parse file: %1\n").Arg(path);
    });

    return nullptr;
  }

  libcomp::BinaryDataSet pSet(
      []() { return std::make_shared<objects::ServerZone>(); },

      [](const std::shared_ptr<libcomp::Object>& obj) {
        return std::dynamic_pointer_cast<objects::ServerZone>(obj)->GetID();
      });

  if (!pSet.LoadXml(doc)) {
    LogGeneralError([&]() {
      return libcomp::String("Failed to load file: %1\n").Arg(path);
    });

    return nullptr;
  }

  auto objs = pSet.GetObjects();
  if (1 != objs.size()) {
    LogGeneralError([&]() {
      return libcomp::String("More than 1 zone in the XML file: %1\n")
          .Arg(path);
    });

    return nullptr;
  }

  auto zone = std::dynamic_pointer_cast<objects::ServerZone>(objs.front());
  if (!zone) {
    LogGeneralError([&]() {
      return libcomp::String("Internal error loading zone from file: %1\n")
          .Arg(path);
    });
  }

  return zone;
}

void ZoneWindow::closeEvent(QCloseEvent* event) {
  (void)event;

  mMainWindow->CloseSelectors(this);
}

void ZoneWindow::LoadZoneFile() {
  QString path = QFileDialog::getOpenFileName(this, tr("Open Zone XML"),
                                              mMainWindow->GetDialogDirectory(),
                                              tr("Zone XML (*.xml)"));
  if (path.isEmpty()) {
    return;
  }

  mMainWindow->SetDialogDirectory(path, true);

  auto zone = LoadZoneFromFile(cs(path));

  // Save any properties currently set (do not save to zone file)
  SaveProperties();

  mMergedZone->Path = cs(path);
  mMergedZone->Definition = zone;
  mMergedZone->CurrentZone = zone;
  mMergedZone->CurrentPartial = nullptr;

  mMainWindow->UpdateActiveZone(mMergedZone->Path);

  // Reset all "show" flags and rebuild the spot filters
  ui.actionShowNPCs->blockSignals(true);
  ui.actionShowNPCs->setChecked(true);
  ui.actionShowNPCs->blockSignals(false);

  ui.actionShowObjects->blockSignals(true);
  ui.actionShowObjects->setChecked(true);
  ui.actionShowObjects->blockSignals(false);

  ui.actionShowBoundaries->blockSignals(true);
  ui.actionShowBoundaries->setChecked(false);
  ui.actionShowBoundaries->blockSignals(false);

  ui.actionShowNavPoints->blockSignals(true);
  ui.actionShowNavPoints->setChecked(false);
  ui.actionShowNavPoints->blockSignals(false);

  auto definitions = mMainWindow->GetDefinitions();

  std::set<uint8_t> spotTypes = {0};
  for (auto spotDef : definitions->GetSpotData(zone->GetDynamicMapID())) {
    spotTypes.insert((uint8_t)spotDef.second->GetType());
  }

  // Duplicate the values from the SpotProperties dropdown
  QWidget temp;
  Ui::SpotProperties* prop = new Ui::SpotProperties;
  prop->setupUi(&temp);

  ui.menuShowSpots->clear();
  for (uint8_t spotType : spotTypes) {
    auto act = ui.menuShowSpots->addAction(
        spotType ? prop->type->itemText((int)spotType) : "All");
    act->setData(spotType);
    act->setCheckable(true);
    act->setChecked(true);

    connect(act, SIGNAL(toggled(bool)), this, SLOT(ShowToggled(bool)));
  }

  ShowZone();
}

void ZoneWindow::mouseMoveEvent(QMouseEvent* event) {
  if (mDragging) {
    auto pos = event->pos();

    auto hBar = ui.mapScrollArea->horizontalScrollBar();
    auto vBar = ui.mapScrollArea->verticalScrollBar();

    hBar->setValue(hBar->value() + mLastMousePos.x() - pos.x());
    vBar->setValue(vBar->value() + mLastMousePos.y() - pos.y());

    mLastMousePos = pos;
  }
}

void ZoneWindow::mousePressEvent(QMouseEvent* event) {
  if (ui.mapScrollArea->underMouse()) {
    if (event->button() == Qt::MouseButton::RightButton) {
      ui.mapScrollArea->setCursor(Qt::CursorShape::ClosedHandCursor);
      mDragging = true;

      mLastMousePos = event->pos();
    }

    int margin = ui.drawTarget->margin();
    auto drawPos = ui.drawTarget->mapFromGlobal(event->globalPos());
    float x = (float)(drawPos.x() + mOffsetX - margin) *
              (float)ui.zoomSlider->value();
    float y = (float)(-drawPos.y() + mOffsetY + margin) *
              (float)ui.zoomSlider->value();
    ui.lblCoordinates->setText(QString("%1/%2").arg(x).arg(y));
  } else {
    ui.lblCoordinates->setText("-/-");
  }
}

void ZoneWindow::mouseReleaseEvent(QMouseEvent* event) {
  (void)event;

  if (mDragging) {
    ui.mapScrollArea->setCursor(Qt::CursorShape::ArrowCursor);
    mDragging = false;
  }
}

bool ZoneWindow::eventFilter(QObject* o, QEvent* e) {
  if (e->type() == QEvent::Wheel &&
      (o == ui.mapScrollArea || o == ui.mapScrollArea->horizontalScrollBar() ||
       o == ui.mapScrollArea->verticalScrollBar())) {
    // Override mouse wheel to zoom for scroll area
    QWheelEvent* we = static_cast<QWheelEvent*>(e);
    ui.zoomSlider->setValue(ui.zoomSlider->value() + (we->delta() / 20));

    return true;
  }

  return false;
}

void ZoneWindow::LoadPartialDirectory() {
  QString qPath = QFileDialog::getExistingDirectory(
      this, tr("Load Zone Partial XML folder"),
      mMainWindow->GetDialogDirectory());
  if (qPath.isEmpty()) {
    return;
  }

  mMainWindow->SetDialogDirectory(qPath, false);

  SaveProperties();

  bool merged = false;

  QDirIterator it(qPath, QStringList() << "*.xml", QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    libcomp::String path = cs(it.next());
    merged |= LoadZonePartials(path);
  }

  if (merged) {
    UpdateMergedZone(true);
  }
}

void ZoneWindow::LoadPartialFile() {
  QString qPath = QFileDialog::getOpenFileName(
      this, tr("Load Zone Partial XML"), mMainWindow->GetDialogDirectory(),
      tr("Zone Partial XML (*.xml)"));
  if (qPath.isEmpty()) {
    return;
  }

  mMainWindow->SetDialogDirectory(qPath, true);

  SaveProperties();

  libcomp::String path = cs(qPath);
  if (LoadZonePartials(path)) {
    UpdateMergedZone(true);
  }
}

void ZoneWindow::LoadQmpFile() {
  QString qPath = QFileDialog::getOpenFileName(
      this, tr("Load QMP XML"), mMainWindow->GetDialogDirectory(),
      tr("QMP XML (*.xml)"));
  if (qPath.isEmpty()) {
    return;
  }

  mMainWindow->SetDialogDirectory(qPath, true);

  SaveProperties();

  libcomp::String path = cs(qPath);

  tinyxml2::XMLDocument doc;
  if (tinyxml2::XML_SUCCESS != doc.LoadFile(path.C())) {
    LogGeneralError([path]() {
      return libcomp::String("Failed to parse file: %1\n").Arg(path);
    });

    return;
  }

  auto rootElem = doc.RootElement();
  if (!rootElem) {
    LogGeneralError([path]() {
      return libcomp::String("No root element in file: %1\n").Arg(path);
    });

    return;
  }

  std::list<std::shared_ptr<objects::ServerZonePartial>> partials;

  auto objNode = rootElem->FirstChildElement("object");
  if (objNode) {
    auto qmp = std::make_shared<objects::QmpFile>();
    if (!qmp->Load(doc, *objNode)) {
      LogGeneralError([path]() {
        return libcomp::String("Failed to load QMP file definition from: %1\n")
            .Arg(path);
      });

      return;
    }

    mQmpFile = qmp;
  }

  ResetQmpFileLines();

  ui.lblCurrentQmpFile->setText(qPath);

  mExternalQmpFile = true;
  ui.btnQmpReset->setEnabled(true);

  RebuildBoundariesTree();
  RebuildNavPointTable();

  DrawMap();
}

void ZoneWindow::SaveFile() {
  // Save off all properties first
  SaveProperties();

  if (mMergedZone) {
    if (mMergedZone->CurrentPartial) {
      std::set<uint32_t> partialIDs;
      partialIDs.insert(mMergedZone->CurrentPartial->GetID());
      SavePartials(partialIDs);
    } else if (mMergedZone->CurrentZone &&
               mMergedZone->Definition == mMergedZone->CurrentZone) {
      SaveZone();
    } else {
      QMessageBox err;
      err.setText(
          "Merged zone definitions cannot be saved directly. Please use 'Save "
          "All' instead or select which file you want to save in the 'View' "
          "dropdown.");
      err.exec();
    }
  } else {
    QMessageBox err;
    err.setText("No zone loaded. Nothing will be saved.");
    err.exec();
  }
}

void ZoneWindow::SaveAllFiles() {
  // Save off all properties first
  SaveProperties();

  SaveFile();

  std::set<uint32_t> partialIDs;
  for (auto& pair : mZonePartials) {
    partialIDs.insert(pair.first);
  }

  SavePartials(partialIDs);
}

void ZoneWindow::SaveQmpFile() {
  if (!mQmpFile) {
    QMessageBox err;
    err.setText("No QMP file loaded. Nothing will be saved.");
    err.exec();

    return;
  }

  QString qPath = QFileDialog::getSaveFileName(
      this, tr("Save QMP XML"), mMainWindow->GetDialogDirectory(),
      tr("QMP XML (*.xml)"));
  if (qPath.isEmpty()) {
    return;
  }

  mMainWindow->SetDialogDirectory(qPath, true);

  SaveProperties();

  tinyxml2::XMLDocument doc;

  auto rootElem = doc.NewElement("objects");
  doc.InsertEndChild(rootElem);

  mQmpFile->Save(doc, *rootElem);

  libcomp::String path = cs(qPath);

  doc.SaveFile(path.C());

  mExternalQmpFile = true;
  ui.btnQmpReset->setEnabled(true);

  ui.lblCurrentQmpFile->setText(qPath);

  LogGeneralDebug(
      [path]() { return libcomp::String("Saved QMP file '%1'\n").Arg(path); });
}

void ZoneWindow::ResetQmpFile() {
  if (mZoneData) {
    auto definitions = mMainWindow->GetDefinitions();
    mQmpFile = definitions->LoadQmpFile(mZoneData->GetFile()->GetQmpFile(),
                                        &*mMainWindow->GetDatastore());
  }

  if (!mQmpFile) {
    LogGeneralError([&]() {
      return libcomp::String("Failed to load QMP file: %1\n")
          .Arg(mZoneData->GetFile()->GetQmpFile());
    });
  } else {
    ResetQmpFileLines();

    mExternalQmpFile = false;
    ui.btnQmpReset->setEnabled(false);

    ui.lblCurrentQmpFile->setText("[Use Zone]");

    RebuildBoundariesTree();
    RebuildNavPointTable();

    DrawMap();
  }
}

void ZoneWindow::ApplyPartials() {
  ZonePartialSelector* selector = new ZonePartialSelector(mMainWindow);
  selector->setWindowModality(Qt::ApplicationModal);

  mSelectedPartials = selector->Select();

  delete selector;

  RebuildCurrentZoneDisplay();
  UpdateMergedZone(true);
}

void ZoneWindow::AddNPC() {
  auto npc = std::make_shared<objects::ServerNPC>();
  if (mMergedZone->CurrentPartial) {
    mMergedZone->CurrentPartial->AppendNPCs(npc);
  } else {
    mMergedZone->CurrentZone->AppendNPCs(npc);
  }

  UpdateMergedZone(true);
}

void ZoneWindow::AddObject() {
  auto obj = std::make_shared<objects::ServerObject>();
  if (mMergedZone->CurrentPartial) {
    mMergedZone->CurrentPartial->AppendObjects(obj);
  } else {
    mMergedZone->CurrentZone->AppendObjects(obj);
  }

  UpdateMergedZone(true);
}

void ZoneWindow::AddSpawn(bool cloneSelected) {
  uint32_t nextID = 1;
  std::shared_ptr<libcomp::Object> clone;
  switch (ui.tabSpawnTypes->currentIndex()) {
    case 1:
      while (nextID && mMergedZone->Definition->SpawnGroupsKeyExists(nextID)) {
        nextID = (uint32_t)(nextID + 1);
      }

      if (cloneSelected) {
        clone = ui.spawnGroups->GetActiveObject();
      }
      break;
    case 2:
      while (nextID &&
             mMergedZone->Definition->SpawnLocationGroupsKeyExists(nextID)) {
        nextID = (uint32_t)(nextID + 1);
      }

      if (cloneSelected) {
        clone = ui.spawnLocationGroups->GetActiveObject();
      }
      break;
    case 0:
    default:
      while (nextID && mMergedZone->Definition->SpawnsKeyExists(nextID)) {
        nextID = (uint32_t)(nextID + 1);
      }

      if (cloneSelected) {
        clone = ui.spawns->GetActiveObject();
      }
      break;
  }

  if (cloneSelected && !clone) {
    // Nothing selected
    return;
  }

  int spawnID =
      QInputDialog::getInt(this, "Enter an ID", "New ID", (int32_t)nextID, 0);
  if (!spawnID) {
    return;
  }

  libcomp::String errMsg;
  switch (ui.tabSpawnTypes->currentIndex()) {
    case 1:
      if (mMergedZone->Definition->SpawnGroupsKeyExists((uint32_t)spawnID)) {
        errMsg =
            libcomp::String("Spawn Group ID %1 already exists").Arg(spawnID);
      } else {
        std::shared_ptr<objects::SpawnGroup> sg;
        if (clone) {
          sg = std::make_shared<objects::SpawnGroup>(
              *std::dynamic_pointer_cast<objects::SpawnGroup>(clone));

          sg->ClearSpawnActions();
          sg->ClearDefeatActions();

          if (sg->GetRestrictions()) {
            // Restrictions are the only exception to the shallow copy
            // to keep so make a copy of that too
            sg->SetRestrictions(std::make_shared<objects::SpawnRestriction>(
                *sg->GetRestrictions()));
          }
        } else {
          sg = std::make_shared<objects::SpawnGroup>();
        }

        sg->SetID((uint32_t)spawnID);
        if (mMergedZone->CurrentPartial) {
          mMergedZone->CurrentPartial->SetSpawnGroups((uint32_t)spawnID, sg);
        } else {
          mMergedZone->CurrentZone->SetSpawnGroups((uint32_t)spawnID, sg);
        }

        // Update then select new spawn group
        UpdateMergedZone(true);
        ui.spawnGroups->Select(sg);
      }
      break;
    case 2:
      if (mMergedZone->Definition->SpawnLocationGroupsKeyExists(
              (uint32_t)spawnID)) {
        errMsg = libcomp::String("Spawn Location Group ID %1 already exists")
                     .Arg(spawnID);
      } else {
        std::shared_ptr<objects::SpawnLocationGroup> slg;
        if (clone) {
          slg = std::make_shared<objects::SpawnLocationGroup>(
              *std::dynamic_pointer_cast<objects::SpawnLocationGroup>(clone));

          slg->ClearLocations();
        } else {
          slg = std::make_shared<objects::SpawnLocationGroup>();
        }

        slg->SetID((uint32_t)spawnID);
        if (mMergedZone->CurrentPartial) {
          mMergedZone->CurrentPartial->SetSpawnLocationGroups((uint32_t)spawnID,
                                                              slg);
        } else {
          mMergedZone->CurrentZone->SetSpawnLocationGroups((uint32_t)spawnID,
                                                           slg);
        }

        // Update then select new spawn location group
        UpdateMergedZone(true);
        ui.spawnLocationGroups->Select(slg);
      }
      break;
    case 0:
    default:
      if (mMergedZone->Definition->SpawnsKeyExists((uint32_t)spawnID)) {
        errMsg = libcomp::String("Spawn ID %1 already exists").Arg(spawnID);
      } else {
        std::shared_ptr<objects::Spawn> spawn;
        if (clone) {
          spawn = std::make_shared<objects::Spawn>(
              *std::dynamic_pointer_cast<objects::Spawn>(clone));

          spawn->ClearDrops();
          spawn->ClearGifts();
        } else {
          spawn = std::make_shared<objects::Spawn>();
        }

        spawn->SetID((uint32_t)spawnID);
        if (mMergedZone->CurrentPartial) {
          mMergedZone->CurrentPartial->SetSpawns((uint32_t)spawnID, spawn);
        } else {
          mMergedZone->CurrentZone->SetSpawns((uint32_t)spawnID, spawn);
        }

        // Update then select new spawn
        UpdateMergedZone(true);
        ui.spawns->Select(spawn);
      }
      break;
  }

  if (errMsg.Length() > 0) {
    QMessageBox err;
    err.setText(qs(errMsg));
    err.exec();
  }
}

void ZoneWindow::CloneSpawn() { AddSpawn(true); }

void ZoneWindow::RemoveNPC() {
  auto npc =
      std::dynamic_pointer_cast<objects::ServerNPC>(ui.npcs->GetActiveObject());
  if (npc) {
    if (mMergedZone->CurrentPartial) {
      size_t count = mMergedZone->CurrentPartial->NPCsCount();
      for (size_t idx = 0; idx < count; idx++) {
        if (mMergedZone->CurrentPartial->GetNPCs(idx) == npc) {
          mMergedZone->CurrentPartial->RemoveNPCs(idx);
          UpdateMergedZone(true);
          return;
        }
      }
    } else {
      size_t count = mMergedZone->CurrentZone->NPCsCount();
      for (size_t idx = 0; idx < count; idx++) {
        if (mMergedZone->CurrentZone->GetNPCs(idx) == npc) {
          mMergedZone->CurrentZone->RemoveNPCs(idx);
          UpdateMergedZone(true);
          return;
        }
      }
    }
  }
}

void ZoneWindow::RemoveObject() {
  auto obj = std::dynamic_pointer_cast<objects::ServerObject>(
      ui.objects->GetActiveObject());
  if (obj) {
    if (mMergedZone->CurrentPartial) {
      size_t count = mMergedZone->CurrentPartial->ObjectsCount();
      for (size_t idx = 0; idx < count; idx++) {
        if (mMergedZone->CurrentPartial->GetObjects(idx) == obj) {
          mMergedZone->CurrentPartial->RemoveObjects(idx);
          UpdateMergedZone(true);
          return;
        }
      }
    } else {
      size_t count = mMergedZone->CurrentZone->ObjectsCount();
      for (size_t idx = 0; idx < count; idx++) {
        if (mMergedZone->CurrentZone->GetObjects(idx) == obj) {
          mMergedZone->CurrentZone->RemoveObjects(idx);
          UpdateMergedZone(true);
          return;
        }
      }
    }
  }
}

void ZoneWindow::RemoveSpawn() {
  bool updated = false;
  switch (ui.tabSpawnTypes->currentIndex()) {
    case 1: {
      auto sg = std::dynamic_pointer_cast<objects::SpawnGroup>(
          ui.spawnGroups->GetActiveObject());
      if (sg) {
        if (mMergedZone->CurrentPartial) {
          mMergedZone->CurrentPartial->RemoveSpawnGroups(sg->GetID());
        } else {
          mMergedZone->CurrentZone->RemoveSpawnGroups(sg->GetID());
        }

        updated = true;
      }
    } break;
    case 2: {
      auto slg = std::dynamic_pointer_cast<objects::SpawnLocationGroup>(
          ui.spawnLocationGroups->GetActiveObject());
      if (slg) {
        if (mMergedZone->CurrentPartial) {
          mMergedZone->CurrentPartial->RemoveSpawnLocationGroups(slg->GetID());
        } else {
          mMergedZone->CurrentZone->RemoveSpawnLocationGroups(slg->GetID());
        }

        updated = true;
      }
    } break;
    case 0:
    default: {
      auto spawn = std::dynamic_pointer_cast<objects::Spawn>(
          ui.spawns->GetActiveObject());
      if (spawn) {
        if (mMergedZone->CurrentPartial) {
          mMergedZone->CurrentPartial->RemoveSpawns(spawn->GetID());
        } else {
          mMergedZone->CurrentZone->RemoveSpawns(spawn->GetID());
        }

        updated = true;
      }
    } break;
  }

  if (updated) {
    UpdateMergedZone(true);
  }
}

void ZoneWindow::ZoneViewUpdated() {
  SaveProperties();

  UpdateMergedZone(true);
}

void ZoneWindow::SelectListObject() { DrawMap(); }

void ZoneWindow::GeometrySelectionChanged() {
  // Only redraw if showing
  auto s = sender();
  if ((s == ui.tblNavPoints->selectionModel() &&
       ui.actionShowNavPoints->isChecked()) ||
      (s == ui.treeBoundaries->selectionModel() &&
       ui.actionShowBoundaries->isChecked())) {
    DrawMap();
  }
}

void ZoneWindow::MainTabChanged() {
  mMainWindow->CloseSelectors(this);

  DrawMap();
}

void ZoneWindow::SpawnTabChanged() {
  mMainWindow->CloseSelectors(this);

  switch (ui.tabSpawnTypes->currentIndex()) {
    case 1:
      ui.addSpawn->setText("Add Spawn Group");
      ui.removeSpawn->setText("Remove Spawn Group");
      break;
    case 2:
      ui.addSpawn->setText("Add Spawn Location Group");
      ui.removeSpawn->setText("Remove Spawn Location Group");
      break;
    case 0:
    default:
      ui.addSpawn->setText("Add Spawn");
      ui.removeSpawn->setText("Remove Spawn");
      break;
  }

  DrawMap();
}

void ZoneWindow::ResetBoundaries(bool redraw, bool optimize) {
  // Create boundary 1 if its not there
  auto boundary1 = GetBoundary(1);
  if (!boundary1) {
    boundary1 = std::make_shared<objects::QmpBoundary>();
    boundary1->SetID(1);
    mQmpFile->PrependBoundaries(boundary1);
  }

  // Store all nav points and clear all lines as we have those stored in
  // file order already. Also determine which boundaries are currently
  // divided to regen below.
  std::set<uint32_t> divided;
  std::list<std::shared_ptr<objects::QmpNavPoint>> navPoints;
  for (auto& boundary : mQmpFile->GetBoundaries()) {
    for (auto& navPoint : boundary->GetNavPoints()) {
      navPoints.push_back(navPoint);
    }

    boundary->ClearNavPoints();
    boundary->ClearLines();

    if (boundary->GetQuadrants(0) || boundary->GetQuadrants(1) ||
        boundary->GetQuadrants(2) || boundary->GetQuadrants(3)) {
      divided.insert(boundary->GetID());
    }
  }

  navPoints.sort([](const std::shared_ptr<objects::QmpNavPoint>& a,
                    const std::shared_ptr<objects::QmpNavPoint>& b) {
    return a->GetPointID() < b->GetPointID();
  });

  // Put all lines and nav points on boundary 1 and remove the rest
  boundary1->SetNavPoints(navPoints);
  boundary1->SetLines(mFileLines);

  mQmpFile->ClearBoundaries();
  mQmpFile->AppendBoundaries(boundary1);

  // Resize boundary 1 as needed and cascade down to lower boundaries
  if (mFileLines.size() > 0) {
    int32_t xMin = 0;
    int32_t xMax = 0;
    int32_t yMin = 0;
    int32_t yMax = 0;

    bool first = true;
    for (auto& line : mFileLines) {
      if (first) {
        xMin = xMax = line->GetX1();
        yMin = yMax = line->GetY1();
      }

      for (int32_t x : {line->GetX1(), line->GetX2()}) {
        if (xMin > x) xMin = x;
        if (xMax < x) xMax = x;
      }

      for (int32_t y : {line->GetY1(), line->GetY2()}) {
        if (yMin > y) yMin = y;
        if (yMax < y) yMax = y;
      }

      first = false;
    }

    // Set the size from the outer bounds + 100 units
    boundary1->SetSize(
        ((xMax - xMin) >= (yMax - yMin) ? (xMax - xMin) : (yMax - yMin)) + 200);

    // Determine the center point
    boundary1->SetCenterX(xMin + (xMax - xMin) / 2);
    boundary1->SetCenterY(yMin + (yMax - yMin) / 2);
  } else {
    // Nothing we really can do until there is geometry
  }

  // Now split quads
  for (uint32_t boundaryID : divided) {
    DivideBoundary(boundaryID);
  }

  if (optimize) {
    // Keep dividing until soft limit for lines is passed for each boundary
    // (or they are not larger than 1000 units)
    bool repeat = false;
    do {
      divided.clear();

      for (auto& boundary : mQmpFile->GetBoundaries()) {
        if (boundary->LinesCount() > 50 && boundary->GetSize() > 1000 &&
            !boundary->GetQuadrants(0)) {
          divided.insert(boundary->GetID());
        }
      }

      for (uint32_t boundaryID : divided) {
        DivideBoundary(boundaryID);
      }

      repeat = divided.size() > 0;
    } while (repeat);
  }

  if (redraw) {
    RebuildBoundariesTree();

    DrawMap();
  }
}

void ZoneWindow::ResetNavPoints() {
  if (!mQmpFile) {
    return;
  }

  // Clear all existing nav points
  for (auto& boundary : mQmpFile->GetBoundaries()) {
    boundary->ClearNavPoints();
  }

  std::map<int32_t, std::set<int32_t>> newPointMap;
  std::list<std::pair<int32_t, int32_t>> newPoints;

  // Identify points that only exist once that do not form an enclosed shape
  std::map<QPointF, int32_t, QPointFHash> pointCounts;
  for (auto& boundary : mQmpFile->GetBoundaries()) {
    for (auto& line : boundary->GetLines()) {
      QPointF p1(line->GetX1(), line->GetY1());
      QPointF p2(line->GetX2(), line->GetY2());

      if (pointCounts.find(p1) == pointCounts.end()) {
        pointCounts[p1] = 1;
      } else {
        pointCounts[p1]++;
      }

      if (pointCounts.find(p2) == pointCounts.end()) {
        pointCounts[p2] = 1;
      } else {
        pointCounts[p2]++;
      }
    }
  }

  // Keep track of end points to source points so lines can be traced back
  std::map<QPointF, std::set<QPointF, QPointFHash>, QPointFHash> lineMap;
  for (auto& boundary : mQmpFile->GetBoundaries()) {
    for (auto& line : boundary->GetLines()) {
      if (line->GetX1() == line->GetX2() && line->GetY1() == line->GetY2()) {
        // Why?
        continue;
      }

      std::array<QPointF, 2> points = {{QPointF(line->GetX1(), line->GetY1()),
                                        QPointF(line->GetX2(), line->GetY2())}};
      for (size_t i = 0; i < 2; i++) {
        QPointF p1 = points[i == 0 ? 0 : 1];
        QPointF p2 = points[i == 1 ? 0 : 1];

        std::list<QPointF> rawPoints;
        if (pointCounts[p1] == 1) {
          // Calculate "corner" points off the line
          for (auto& point : GetLineNavPointLocations(p2, p1)) {
            rawPoints.push_back(point);
          }
        } else {
          auto& m = lineMap[p1];
          if (m.size() > 0) {
            for (auto& point : m) {
              // Make sure they're not the same line somehow
              if (p2 != point) {
                bool valid = false;
                auto p = GetNavPointLocation(p2, p1, point, valid);
                if (valid) {
                  rawPoints.push_back(p);
                }
              }
            }
          }
        }

        for (auto& rawPoint : rawPoints) {
          int32_t x = (int32_t)round(rawPoint.x());
          int32_t y = (int32_t)round(rawPoint.y());

          if (newPointMap[x].find(y) == newPointMap[x].end()) {
            newPointMap[x].insert(y);
            newPoints.push_back(std::make_pair(x, y));
          }
        }

        lineMap[p1].insert(p2);
      }
    }
  }

  // Determine which elements can be toggled and gather all lines not in
  // that set
  std::set<uint32_t> toggleElems;
  for (auto& elem : mQmpFile->GetElements()) {
    if (elem->GetType() != objects::QmpElement::Type_t::NORMAL) {
      toggleElems.insert(elem->GetID());
    }
  }

  std::list<QLineF> lines;
  for (auto& boundary : mQmpFile->GetBoundaries()) {
    for (auto& line : boundary->GetLines()) {
      if (toggleElems.find(line->GetElementID()) == toggleElems.end()) {
        lines.push_back(
            QLineF(line->GetX1(), line->GetY1(), line->GetX2(), line->GetY2()));
      }
    }
  }

  // Now add the points to boundary 1
  auto boundary1 = GetBoundary(1);
  if (!boundary1) {
    ResetBoundaries(false, false);

    boundary1 = GetBoundary(1);
  }

  int32_t pointID = 0;
  for (auto& pointDef : newPoints) {
    int32_t x = pointDef.first;
    int32_t y = pointDef.second;

    auto navPoint = std::make_shared<objects::QmpNavPoint>();
    navPoint->SetPointID((uint32_t)++pointID);
    navPoint->SetX(x);
    navPoint->SetY(y);

    boundary1->AppendNavPoints(navPoint);
  }

  // Calculate the distances
  std::map<uint32_t, std::set<uint32_t>> measured;
  for (auto& point1 : boundary1->GetNavPoints()) {
    auto& mSet = measured[point1->GetPointID()];
    for (auto& point2 : boundary1->GetNavPoints()) {
      // Skip same point and already measured points
      if (point1 == point2 || mSet.find(point2->GetPointID()) != mSet.end()) {
        continue;
      }

      QPointF collision;
      bool collides = false;

      QLineF path(point1->GetX(), point1->GetY(), point2->GetX(),
                  point2->GetY());
      for (auto& line : lines) {
        if (path.intersect(line, &collision) ==
            QLineF::IntersectType::BoundedIntersection) {
          collides = true;
          break;
        }
      }

      if (!collides) {
        point1->SetDistances(point2->GetPointID(), (float)path.length());
        point2->SetDistances(point1->GetPointID(), (float)path.length());
      }

      mSet.insert(point2->GetPointID());
      measured[point2->GetPointID()].insert(point1->GetPointID());
    }
  }

  // Remove all nav points with no distances to other points
  auto points = boundary1->GetNavPoints();
  points.remove_if([](const std::shared_ptr<objects::QmpNavPoint>& p) {
    return p->DistancesCount() == 0;
  });

  boundary1->SetNavPoints(points);

  RebuildNavPointTable();

  // Lastly reset the boundaries and their contents
  ResetBoundaries(true, false);
}

void ZoneWindow::ToggleBoundaryDivide() {
  QPushButton* src = qobject_cast<QPushButton*>(sender());

  uint32_t boundaryID = (uint32_t)src->property("val").toInt();

  auto boundary = GetBoundary(boundaryID);
  if (!boundary) {
    return;
  }

  if (boundary->GetQuadrants(0) || boundary->GetQuadrants(1) ||
      boundary->GetQuadrants(2) || boundary->GetQuadrants(3)) {
    // Merge
    MergeBoundary(boundary);
  } else {
    // Divide
    DivideBoundary(boundaryID);
  }

  RebuildBoundariesTree();
  RebuildNavPointTable();

  DrawMap();
}

void ZoneWindow::NPCMoved(std::shared_ptr<libcomp::Object> obj, bool up) {
  std::list<std::shared_ptr<objects::ServerNPC>> npcList;
  if (mMergedZone->CurrentPartial) {
    npcList = mMergedZone->CurrentPartial->GetNPCs();
  } else if (mMergedZone->Definition == mMergedZone->CurrentZone) {
    npcList = mMergedZone->Definition->GetNPCs();
  } else {
    // Nothing to do
    return;
  }

  if (ObjectList::Move(
          npcList, std::dynamic_pointer_cast<objects::ServerNPC>(obj), up)) {
    if (mMergedZone->CurrentPartial) {
      mMergedZone->CurrentPartial->SetNPCs(npcList);
    } else {
      mMergedZone->Definition->SetNPCs(npcList);
    }

    BindNPCs();
    Refresh();
    ui.npcs->Select(obj);
  }
}

void ZoneWindow::ObjectMoved(std::shared_ptr<libcomp::Object> obj, bool up) {
  std::list<std::shared_ptr<objects::ServerObject>> objList;
  if (mMergedZone->CurrentPartial) {
    objList = mMergedZone->CurrentPartial->GetObjects();
  } else if (mMergedZone->Definition == mMergedZone->CurrentZone) {
    objList = mMergedZone->Definition->GetObjects();
  } else {
    // Nothing to do
    return;
  }

  if (ObjectList::Move(
          objList, std::dynamic_pointer_cast<objects::ServerObject>(obj), up)) {
    if (mMergedZone->CurrentPartial) {
      mMergedZone->CurrentPartial->SetObjects(objList);
    } else {
      mMergedZone->Definition->SetObjects(objList);
    }

    BindObjects();
    Refresh();
    ui.objects->Select(obj);
  }
}

void ZoneWindow::Zoom() { DrawMap(); }

void ZoneWindow::ShowToggled(bool checked) {
  QAction* qAct = qobject_cast<QAction*>(sender());
  if (qAct && qAct->parentWidget() == ui.menuShowSpots) {
    auto showAll = ui.menuShowSpots->actions()[0];
    if (qAct == showAll) {
      // All toggled
      for (auto act : ui.menuShowSpots->actions()) {
        if (act != qAct) {
          act->blockSignals(true);
          act->setChecked(checked);
          act->blockSignals(false);
        }
      }
    } else {
      // Specific type toggled, update "All"
      bool allChecked = true;
      for (auto act : ui.menuShowSpots->actions()) {
        int type = act->data().toInt();
        if (type) {
          allChecked &= act->isChecked();
        }
      }

      if (showAll->isChecked() != allChecked) {
        showAll->blockSignals(true);
        showAll->setChecked(allChecked);
        showAll->blockSignals(false);
      }
    }
  }

  DrawMap();
}

void ZoneWindow::Refresh() {
  SaveProperties();

  LoadMapFromZone();
}

bool ZoneWindow::LoadZonePartials(const libcomp::String& path) {
  tinyxml2::XMLDocument doc;
  if (tinyxml2::XML_SUCCESS != doc.LoadFile(path.C())) {
    LogGeneralError([&]() {
      return libcomp::String("Failed to parse file: %1\n").Arg(path);
    });

    return false;
  }

  auto rootElem = doc.RootElement();
  if (!rootElem) {
    LogGeneralError([&]() {
      return libcomp::String("No root element in file: %1\n").Arg(path);
    });

    return false;
  }

  std::list<std::shared_ptr<objects::ServerZonePartial>> partials;

  auto objNode = rootElem->FirstChildElement("object");
  while (objNode) {
    auto partial = std::make_shared<objects::ServerZonePartial>();
    if (!partial || !partial->Load(doc, *objNode)) {
      break;
    }

    partials.push_back(partial);

    objNode = objNode->NextSiblingElement("object");
  }

  // Add the file if it has partials or no child nodes
  if (partials.size() > 0 || rootElem->FirstChild() == nullptr) {
    LogGeneralInfo([&]() {
      return libcomp::String("Loading %1 zone partial(s) from file: %2\n")
          .Arg(partials.size())
          .Arg(path);
    });

    std::set<uint32_t> loadedPartials;
    for (auto partial : partials) {
      if (mZonePartials.find(partial->GetID()) != mZonePartials.end()) {
        LogGeneralWarning([&]() {
          return libcomp::String("Reloaded zone partial %1 from file: %2\n")
              .Arg(partial->GetID())
              .Arg(path);
        });
      }

      mZonePartials[partial->GetID()] = partial;

      mZonePartialFiles[partial->GetID()] = path;

      loadedPartials.insert(partial->GetID());
    }

    ResetAppliedPartials(loadedPartials);

    return true;
  } else {
    LogGeneralWarning([&]() {
      return libcomp::String("No zone partials found in file: %1\n").Arg(path);
    });
  }

  return false;
}

void ZoneWindow::SaveZone() {
  if (mMergedZone->Path.Length() == 0 || !mMergedZone ||
      !mMergedZone->CurrentZone) {
    // No zone file loaded
    return;
  }

  auto zone = mMergedZone->CurrentZone;

  tinyxml2::XMLDocument doc;

  auto rootElem = doc.NewElement("objects");
  doc.InsertEndChild(rootElem);

  zone->Save(doc, *rootElem);

  tinyxml2::XMLNode* zNode = rootElem->LastChild();

  std::list<tinyxml2::XMLNode*> updatedNodes;
  updatedNodes.push_back(zNode);

  XmlHandler::SimplifyObjects(updatedNodes);

  doc.SaveFile(mMergedZone->Path.C());

  LogGeneralDebug([&]() {
    return libcomp::String("Updated zone file '%1'\n").Arg(mMergedZone->Path);
  });
}

void ZoneWindow::SavePartials(const std::set<uint32_t>& partialIDs) {
  std::unordered_map<libcomp::String, std::set<uint32_t>> fileMap;
  for (uint32_t partialID : partialIDs) {
    fileMap[mZonePartialFiles[partialID]].insert(partialID);
  }

  if (fileMap.size() == 0) {
    // Nothing to save
    return;
  }

  for (auto& filePair : fileMap) {
    auto path = filePair.first;

    tinyxml2::XMLDocument doc;
    if (tinyxml2::XML_SUCCESS != doc.LoadFile(path.C())) {
      LogGeneralError([&]() {
        return libcomp::String("Failed to parse file for saving: %1\n")
            .Arg(path);
      });

      continue;
    }

    std::unordered_map<uint32_t, tinyxml2::XMLNode*> existing;

    auto rootElem = doc.RootElement();
    if (!rootElem) {
      // If for whatever reason we don't have a root element, create
      // one now
      rootElem = doc.NewElement("objects");
      doc.InsertEndChild(rootElem);
    } else {
      // Load all existing partials for replacement
      auto child = rootElem->FirstChild();
      while (child != 0) {
        auto member = child->FirstChildElement("member");
        while (member != 0) {
          libcomp::String memberName(member->Attribute("name"));
          if (memberName == "ID") {
            auto txtChild = member->FirstChild();
            auto txt = txtChild ? txtChild->ToText() : 0;
            if (txt) {
              existing[libcomp::String(txt->Value()).ToInteger<uint32_t>()] =
                  child;
            }
            break;
          }

          member = member->NextSiblingElement("member");
        }

        child = child->NextSibling();
      }
    }

    // Now handle updates
    std::list<tinyxml2::XMLNode*> updatedNodes;
    for (uint32_t partialID : filePair.second) {
      auto partial = mZonePartials[partialID];

      // Append to the existing file
      partial->Save(doc, *rootElem);

      tinyxml2::XMLNode* pNode = rootElem->LastChild();

      // If the partial already existed in the file, move it to the
      // same location and drop the old one
      auto iter = existing.find(partialID);
      if (iter != existing.end()) {
        if (iter->second->NextSibling() != pNode) {
          rootElem->InsertAfterChild(iter->second, pNode);
        }

        rootElem->DeleteChild(iter->second);
        existing[partialID] = pNode;
      }

      updatedNodes.push_back(pNode);
    }

    if (updatedNodes.size() > 0) {
      XmlHandler::SimplifyObjects(updatedNodes);
    }

    doc.SaveFile(path.C());

    LogGeneralDebug([&]() {
      return libcomp::String("Updated zone partial file '%1'\n").Arg(path);
    });
  }
}

void ZoneWindow::ResetAppliedPartials(std::set<uint32_t> newPartials) {
  uint32_t dynamicMapID = mMergedZone->CurrentZone->GetDynamicMapID();
  for (auto& pair : mZonePartials) {
    if (newPartials.size() == 0 ||
        newPartials.find(pair.first) != newPartials.end()) {
      auto partial = pair.second;

      if (partial->GetAutoApply() && dynamicMapID &&
          partial->DynamicMapIDsContains(dynamicMapID)) {
        // Automatically add auto applies
        mSelectedPartials.insert(partial->GetID());
      }
    }
  }

  RebuildCurrentZoneDisplay();
}

void ZoneWindow::RebuildCurrentZoneDisplay() {
  ui.zoneView->blockSignals(true);

  ui.zoneView->clear();
  if (mSelectedPartials.size() > 0) {
    ui.zoneView->addItem("Merged Zone", -2);
    ui.zoneView->addItem("Zone Only", -1);

    for (uint32_t partialID : mSelectedPartials) {
      if (partialID) {
        ui.zoneView->addItem(QString("Partial %1").arg(partialID),
                             (int32_t)partialID);
      } else {
        ui.zoneView->addItem("Global Partial", 0);
      }
    }

    ui.zoneViewWidget->show();
  } else {
    ui.zoneViewWidget->hide();
  }

  ui.zoneView->blockSignals(false);
}

void ZoneWindow::UpdateMergedZone(bool redraw) {
  mMainWindow->CloseSelectors(this);

  // Set control defaults
  ui.lblZoneViewNotes->setText("");

  ui.zoneHeaderWidget->hide();
  ui.grpZone->setDisabled(true);
  ui.xpMultiplier->setDisabled(true);
  ui.grpBonuses->setDisabled(true);
  ui.grpSkills->setDisabled(true);
  ui.grpTriggers->setDisabled(true);

  ui.grpPartial->hide();
  ui.partialAutoApply->setChecked(false);
  ui.partialDynamicMapIDs->Clear();

  mMergedZone->CurrentPartial = nullptr;

  bool canEdit = true;

  bool zoneOnly = mSelectedPartials.size() == 0;
  if (!zoneOnly) {
    // Build merged zone based on
    int viewing = ui.zoneView->currentData().toInt();
    switch (viewing) {
      case -2:
        // Copy the base zone definition and apply all partials
        {
          auto copyZone =
              std::make_shared<objects::ServerZone>(*mMergedZone->CurrentZone);

          for (uint32_t partialID : mSelectedPartials) {
            auto partial = mZonePartials[partialID];
            libcomp::ServerDataManager::ApplyZonePartial(copyZone, partial,
                                                         true);
          }

          mMergedZone->Definition = copyZone;

          // Show the zone details but do not enable editing
          ui.zoneHeaderWidget->show();

          ui.lblZoneViewNotes->setText(
              "No zone or zone partial fields can be modified while viewing a "
              "merged zone.");

          canEdit = false;
        }
        break;
      case -1:
        // Merge no partials
        zoneOnly = true;
        break;
      default:
        // Build zone just from selected partial
        if (viewing >= 0) {
          auto newZone = std::make_shared<objects::ServerZone>();
          newZone->SetID(mMergedZone->CurrentZone->GetID());
          newZone->SetDynamicMapID(mMergedZone->CurrentZone->GetDynamicMapID());

          auto partial = mZonePartials[(uint32_t)viewing];
          libcomp::ServerDataManager::ApplyZonePartial(newZone, partial, false);

          mMergedZone->Definition = newZone;
          mMergedZone->CurrentPartial = partial;

          // Show the partial controls
          ui.grpPartial->show();
          ui.partialID->setValue((int)partial->GetID());

          ui.partialAutoApply->setChecked(partial->GetAutoApply());

          ui.partialDynamicMapIDs->Clear();
          for (uint32_t dynamicMapID : partial->GetDynamicMapIDs()) {
            ui.partialDynamicMapIDs->AddUnsignedInteger(dynamicMapID);
          }

          ui.grpBonuses->setDisabled(false);
          ui.grpSkills->setDisabled(false);
          ui.grpTriggers->setDisabled(false);

          ui.lblZoneViewNotes->setText(
              "Changes made while viewing a zone partial will not be applied "
              "directly to the zone.");
        }
        break;
    }
  }

  if (zoneOnly) {
    // Only the zone is loaded, merged zone equals
    mMergedZone->Definition = mMergedZone->CurrentZone;

    ui.zoneHeaderWidget->show();
    ui.grpZone->setDisabled(false);
    ui.xpMultiplier->setDisabled(false);
    ui.grpBonuses->setDisabled(false);
    ui.grpSkills->setDisabled(false);
    ui.grpTriggers->setDisabled(false);
  }

  ui.npcs->SetReadOnly(!canEdit);
  ui.objects->SetReadOnly(!canEdit);
  ui.npcs->ToggleMoveControls(canEdit);
  ui.objects->ToggleMoveControls(canEdit);

  ui.spawns->SetReadOnly(!canEdit);
  ui.spawnGroups->SetReadOnly(!canEdit);
  ui.spawnLocationGroups->SetReadOnly(!canEdit);
  ui.spots->SetReadOnly(!canEdit);

  ui.addNPC->setDisabled(!canEdit);
  ui.addObject->setDisabled(!canEdit);
  ui.addSpawn->setDisabled(!canEdit);
  ui.cloneSpawn->setDisabled(!canEdit);
  ui.removeNPC->setDisabled(!canEdit);
  ui.removeObject->setDisabled(!canEdit);
  ui.removeSpawn->setDisabled(!canEdit);

  // Update merged collection properties
  ui.dropSetIDs->Clear();
  for (uint32_t dropSetID : mMergedZone->Definition->GetDropSetIDs()) {
    ui.dropSetIDs->AddUnsignedInteger(dropSetID);
  }

  ui.skillBlacklist->Clear();
  for (uint32_t skillID : mMergedZone->Definition->GetSkillBlacklist()) {
    ui.skillBlacklist->AddUnsignedInteger(skillID);
  }

  ui.skillWhitelist->Clear();
  for (uint32_t skillID : mMergedZone->Definition->GetSkillWhitelist()) {
    ui.skillWhitelist->AddUnsignedInteger(skillID);
  }

  ui.triggers->Clear();
  for (auto trigger : mMergedZone->Definition->GetTriggers()) {
    ui.triggers->AddObject(trigger);
  }

  if (redraw) {
    LoadMapFromZone();
  }
}

void ZoneWindow::ResetQmpFileLines() {
  mFileLines.clear();
  for (auto& boundary : mQmpFile->GetBoundaries()) {
    for (auto& line : boundary->GetLines()) {
      mFileLines.push_back(line);
    }
  }
}

void ZoneWindow::RebuildBoundariesTree() {
  ui.treeBoundaries->clear();

  if (!mQmpFile) return;

  std::map<uint32_t, std::shared_ptr<objects::QmpBoundary>> boundaryMap;
  std::map<uint32_t, uint32_t> parentMap;
  for (auto boundary : mQmpFile->GetBoundaries()) {
    // Ignore ID zero
    if (boundary->GetID()) {
      boundaryMap[boundary->GetID()] = boundary;
      for (uint32_t quad : boundary->GetQuadrants()) {
        if (quad) {
          parentMap[quad] = boundary->GetID();
        }
      }

      if (parentMap.find(boundary->GetID()) == parentMap.end()) {
        parentMap[boundary->GetID()] = 0;
      }
    }
  }

  // Write top level
  std::map<uint32_t, QTreeWidgetItem*> nodes;
  for (auto& pair : parentMap) {
    if (pair.second == 0) {
      auto boundary = boundaryMap[pair.first];

      QTreeWidgetItem* item = GetBoundaryNode(boundary, pair.first, nullptr);
      QPushButton* btn = GetBoundaryActionButton(boundary);

      ui.treeBoundaries->addTopLevelItem(item);

      if (btn) {
        ui.treeBoundaries->setItemWidget(item, 6, btn);
      }

      nodes[pair.first] = item;
    }
  }

  // Now write child nodes (should be in order but loop in case its not
  // upon loading)
  bool updated = false;
  do {
    updated = false;

    for (auto& pair : parentMap) {
      if (nodes.find(pair.first) == nodes.end() &&
          nodes.find(pair.second) != nodes.end()) {
        auto boundary = boundaryMap[pair.first];

        auto parent = nodes[pair.second];

        QTreeWidgetItem* item = GetBoundaryNode(boundary, pair.first, parent);
        QPushButton* btn = GetBoundaryActionButton(boundary);

        if (btn) {
          ui.treeBoundaries->setItemWidget(item, 6, btn);
        }

        nodes[boundary->GetID()] = item;

        updated = true;
      }
    }
  } while (updated);

  ui.treeBoundaries->expandAll();
  ui.treeBoundaries->resizeColumnToContents(0);
  ui.treeBoundaries->resizeColumnToContents(1);
  ui.treeBoundaries->resizeColumnToContents(2);
  ui.treeBoundaries->resizeColumnToContents(3);
  ui.treeBoundaries->resizeColumnToContents(4);
  ui.treeBoundaries->resizeColumnToContents(5);
}

QTreeWidgetItem* ZoneWindow::GetBoundaryNode(
    std::shared_ptr<objects::QmpBoundary> boundary, uint32_t id,
    QTreeWidgetItem* parent) {
  QTreeWidgetItem* item = new QTreeWidgetItem(parent);

  item->setText(0, libcomp::String("%1").Arg(id).C());

  if (boundary) {
    item->setText(1, libcomp::String("%1").Arg(boundary->GetSize()).C());

    item->setText(2, libcomp::String("(%1, %2)")
                         .Arg(boundary->GetCenterX())
                         .Arg(boundary->GetCenterY())
                         .C());

    int32_t span = boundary->GetSize() / 2;
    int32_t x1 = boundary->GetCenterX() - span;
    int32_t y1 = boundary->GetCenterY() + span;
    int32_t x2 = boundary->GetCenterX() + span;
    int32_t y2 = boundary->GetCenterY() - span;

    item->setText(3, libcomp::String("(%1, %2)->(%3, %4)")
                         .Arg(x1)
                         .Arg(y1)
                         .Arg(x2)
                         .Arg(y2)
                         .C());

    item->setText(4, libcomp::String("%1").Arg(boundary->LinesCount()).C());
    item->setText(5, libcomp::String("%1").Arg(boundary->NavPointsCount()).C());

    if (boundary->LinesCount() > 50) {
      item->setTextColor(4, QColor(Qt::red));
    }
  } else {
    item->setTextColor(0, QColor(Qt::red));
  }

  return item;
}

QPushButton* ZoneWindow::GetBoundaryActionButton(
    std::shared_ptr<objects::QmpBoundary> boundary) {
  if (!boundary) {
    return nullptr;
  }

  QPushButton* actionBtn = nullptr;
  if (boundary->GetQuadrants(0) || boundary->GetQuadrants(1) ||
      boundary->GetQuadrants(2) || boundary->GetQuadrants(3)) {
    // Add merge button
    actionBtn = new QPushButton("Merge");
  } else if (boundary->GetSize() > 1000) {
    // Add divide button (stop allowing when it gets pretty small)
    actionBtn = new QPushButton("Divide");
  }

  if (actionBtn) {
    actionBtn->setFixedWidth(50);
    actionBtn->setProperty("val", QVariant(boundary->GetID()));

    connect(actionBtn, SIGNAL(clicked()), this, SLOT(ToggleBoundaryDivide()));
  }

  return actionBtn;
}

std::shared_ptr<objects::QmpBoundary> ZoneWindow::GetBoundary(uint32_t id) {
  if (mQmpFile) {
    for (auto b : mQmpFile->GetBoundaries()) {
      if (b->GetID() == id) {
        return b;
      }
    }
  }

  return nullptr;
}

void ZoneWindow::DivideBoundary(uint32_t boundaryID) {
  auto boundary = GetBoundary(boundaryID);
  if (!boundary) {
    return;
  }

  uint32_t min = 1;
  int32_t depth = 0;
  while (min > boundaryID) {
    min = (uint32_t)(min + std::pow(4, depth++));
  }

  uint32_t offset = (uint32_t)(boundaryID - min);
  uint32_t firstID = (uint32_t)(min + (offset * 4) + 1);
  int32_t step = boundary->GetSize() / 4;

  for (size_t i = 0; i < 4; i++) {
    uint32_t newID = (uint32_t)(firstID + i);

    auto q = std::make_shared<objects::QmpBoundary>();
    q->SetID(newID);
    q->SetSize(boundary->GetSize() / 2);

    int32_t xOffset = (i % 2) == 0 ? -step : step;
    int32_t yOffset = (i < 2) == 0 ? -step : step;

    q->SetCenterX(boundary->GetCenterX() + xOffset);
    q->SetCenterY(boundary->GetCenterY() + yOffset);

    boundary->SetQuadrants(i, newID);
    mQmpFile->AppendBoundaries(q);

    // Determine which lines and nav points to move down
    QRect bounds(q->GetCenterX() - q->GetSize() / 2,
                 q->GetCenterY() - q->GetSize() / 2, q->GetSize(),
                 q->GetSize());

    bool updated = false;
    for (auto& line : boundary->GetLines()) {
      if (bounds.contains(line->GetX1(), line->GetY1()) &&
          bounds.contains(line->GetX2(), line->GetY2())) {
        q->AppendLines(line);
        updated = true;
      }
    }

    for (auto& navPoint : boundary->GetNavPoints()) {
      if (bounds.contains(navPoint->GetX(), navPoint->GetY())) {
        q->AppendNavPoints(navPoint);
        updated = true;
      }
    }

    if (updated) {
      // Remove lines and nav points moved down
      auto lines = boundary->GetLines();
      lines.remove_if([q](const std::shared_ptr<objects::QmpBoundaryLine>& l) {
        for (auto l2 : q->GetLines()) {
          if (l2 == l) return true;
        }

        return false;
      });

      boundary->SetLines(lines);

      auto navPoints = boundary->GetNavPoints();
      navPoints.remove_if([q](const std::shared_ptr<objects::QmpNavPoint>& p) {
        for (auto p2 : q->GetNavPoints()) {
          if (p2 == p) return true;
        }

        return false;
      });

      boundary->SetNavPoints(navPoints);
    }
  }

  auto boundaries = mQmpFile->GetBoundaries();
  boundaries.sort([](const std::shared_ptr<objects::QmpBoundary>& a,
                     const std::shared_ptr<objects::QmpBoundary>& b) {
    return a->GetID() < b->GetID();
  });

  mQmpFile->SetBoundaries(boundaries);
}

void ZoneWindow::MergeBoundary(std::shared_ptr<objects::QmpBoundary> boundary) {
  if (boundary && (boundary->GetQuadrants(0) || boundary->GetQuadrants(1) ||
                   boundary->GetQuadrants(2) || boundary->GetQuadrants(3))) {
    for (uint32_t qID : boundary->GetQuadrants()) {
      auto q = GetBoundary(qID);
      if (q && qID) {
        MergeBoundary(q);

        // Move all lines and nav points into the parent
        for (auto& l : q->GetLines()) {
          boundary->AppendLines(l);
        }

        for (auto& navPoint : q->GetNavPoints()) {
          boundary->AppendNavPoints(navPoint);
        }
      }
    }

    auto boundaries = mQmpFile->GetBoundaries();
    boundaries.remove_if(
        [boundary](const std::shared_ptr<objects::QmpBoundary>& b) {
          return b->GetID() && (b->GetID() == boundary->GetQuadrants(0) ||
                                b->GetID() == boundary->GetQuadrants(1) ||
                                b->GetID() == boundary->GetQuadrants(2) ||
                                b->GetID() == boundary->GetQuadrants(3));
        });

    mQmpFile->SetBoundaries(boundaries);

    boundary->SetQuadrants(0, 0);
    boundary->SetQuadrants(1, 0);
    boundary->SetQuadrants(2, 0);
    boundary->SetQuadrants(3, 0);
  }
}

void ZoneWindow::RebuildNavPointTable() {
  ui.tblNavPoints->setRowCount(0);

  if (!mQmpFile) return;

  std::map<uint32_t, std::shared_ptr<objects::QmpNavPoint>> navMap;
  std::map<uint32_t, uint32_t> boundaryMap;
  for (auto& boundary : mQmpFile->GetBoundaries()) {
    for (auto& navPoint : boundary->GetNavPoints()) {
      navMap[navPoint->GetPointID()] = navPoint;
      boundaryMap[navPoint->GetPointID()] = boundary->GetID();
    }
  }

  ui.tblNavPoints->setRowCount((int)navMap.size());

  int idx = 0;
  for (auto& pair : navMap) {
    auto navPoint = pair.second;

    ui.tblNavPoints->setItem(idx, 0,
                             new QTableWidgetItem(QString::number(pair.first)));
    ui.tblNavPoints->setItem(idx, 1,
                             new QTableWidgetItem(libcomp::String("(%1, %2)")
                                                      .Arg(navPoint->GetX())
                                                      .Arg(navPoint->GetY())
                                                      .C()));
    ui.tblNavPoints->setItem(
        idx, 2,
        new QTableWidgetItem(QString::number(navPoint->DistancesCount())));
    ui.tblNavPoints->setItem(
        idx, 3, new QTableWidgetItem(QString::number(boundaryMap[pair.first])));

    idx++;
  }

  ui.tblNavPoints->resizeColumnsToContents();
}

bool ZoneWindow::LoadMapFromZone() {
  mMainWindow->CloseSelectors(this);

  auto zone = mMergedZone->Definition;

  auto dataset = mMainWindow->GetBinaryDataSet("ZoneData");
  mZoneData = std::dynamic_pointer_cast<objects::MiZoneData>(
      dataset->GetObjectByID(zone->GetID()));
  if (!mZoneData) {
    LogGeneralError([zone]() {
      return libcomp::String("No MiZoneData found for ID %1\n")
          .Arg(zone->GetID());
    });
    return false;
  }

  auto definitions = mMainWindow->GetDefinitions();
  mQmpFile = definitions->LoadQmpFile(mZoneData->GetFile()->GetQmpFile(),
                                      &*mMainWindow->GetDatastore());
  if (!mQmpFile) {
    LogGeneralError([&]() {
      return libcomp::String("Failed to load QMP file: %1\n")
          .Arg(mZoneData->GetFile()->GetQmpFile());
    });
    return false;
  }

  ResetQmpFileLines();

  mExternalQmpFile = false;
  ui.btnQmpReset->setEnabled(false);

  ui.lblCurrentQmpFile->setText("[Use Zone]");

  BindNPCs();
  BindObjects();

  RebuildNamedDataSet("Actor");

  BindSpawns();
  BindSpots();

  RebuildBoundariesTree();
  RebuildNavPointTable();

  DrawMap();

  return true;
}

void ZoneWindow::LoadProperties() {
  if (!mMergedZone->Definition) {
    return;
  }

  auto zone = mMergedZone->Definition;
  ui.zoneID->SetValue(zone->GetID());
  ui.dynamicMapID->setValue((int32_t)zone->GetDynamicMapID());
  ui.globalZone->setChecked(zone->GetGlobal());
  ui.zoneRestricted->setChecked(zone->GetRestricted());
  ui.groupID->setValue((int32_t)zone->GetGroupID());
  ui.globalBossGroup->setValue((int32_t)zone->GetGlobalBossGroup());
  ui.zoneStartingX->setValue((double)zone->GetStartingX());
  ui.zoneStartingY->setValue((double)zone->GetStartingY());
  ui.zoneStartingRotation->setValue((double)zone->GetStartingRotation());
  ui.xpMultiplier->setValue((double)zone->GetXPMultiplier());
  ui.bazaarMarketCost->setValue((int32_t)zone->GetBazaarMarketCost());
  ui.bazaarMarketTime->setValue((int32_t)zone->GetBazaarMarketTime());
  ui.mountDisabled->setChecked(zone->GetMountDisabled());
  ui.bikeDisabled->setChecked(zone->GetBikeDisabled());
  ui.bikeBoostEnabled->setChecked(zone->GetBikeBoostEnabled());

  ui.validTeamTypes->Clear();
  for (int8_t teamType : zone->GetValidTeamTypes()) {
    ui.validTeamTypes->AddInteger(teamType);
  }

  ui.trackTeam->setChecked(zone->GetTrackTeam());
}

void ZoneWindow::SaveProperties() {
  // Pull all properties into their respective parent
  ui.npcs->SaveActiveProperties();
  ui.objects->SaveActiveProperties();
  ui.spawns->SaveActiveProperties();
  ui.spawnGroups->SaveActiveProperties();
  ui.spawnLocationGroups->SaveActiveProperties();
  ui.spots->SaveActiveProperties();

  if (mMergedZone->CurrentPartial) {
    // Partial selected
    auto partial = mMergedZone->CurrentPartial;

    partial->SetAutoApply(ui.partialAutoApply->isChecked());

    partial->ClearDynamicMapIDs();
    for (uint32_t dynamicMapID :
         ui.partialDynamicMapIDs->GetUnsignedIntegerList()) {
      partial->InsertDynamicMapIDs(dynamicMapID);
    }

    partial->ClearDropSetIDs();
    for (uint32_t dropSetID : ui.dropSetIDs->GetUnsignedIntegerList()) {
      partial->InsertDropSetIDs(dropSetID);
    }

    partial->ClearSkillBlacklist();
    for (uint32_t skillID : ui.skillBlacklist->GetUnsignedIntegerList()) {
      partial->InsertSkillBlacklist(skillID);
    }

    partial->ClearSkillWhitelist();
    for (uint32_t skillID : ui.skillWhitelist->GetUnsignedIntegerList()) {
      partial->InsertSkillWhitelist(skillID);
    }

    auto triggers = ui.triggers->GetObjectList<objects::ServerZoneTrigger>();
    partial->SetTriggers(triggers);
  } else if (mMergedZone->CurrentZone &&
             mMergedZone->CurrentZone == mMergedZone->Definition) {
    // Zone selected
    auto zone = mMergedZone->CurrentZone;

    zone->SetGlobal(ui.globalZone->isChecked());
    zone->SetRestricted(ui.zoneRestricted->isChecked());
    zone->SetGroupID((uint32_t)ui.groupID->value());
    zone->SetGlobalBossGroup((uint32_t)ui.globalBossGroup->value());
    zone->SetStartingX((float)ui.zoneStartingX->value());
    zone->SetStartingY((float)ui.zoneStartingY->value());
    zone->SetStartingRotation((float)ui.zoneStartingRotation->value());
    zone->SetXPMultiplier((float)ui.xpMultiplier->value());
    zone->SetBazaarMarketCost((uint32_t)ui.bazaarMarketCost->value());
    zone->SetBazaarMarketTime((uint32_t)ui.bazaarMarketTime->value());
    zone->SetMountDisabled(ui.mountDisabled->isChecked());
    zone->SetBikeDisabled(ui.bikeDisabled->isChecked());
    zone->SetBikeBoostEnabled(ui.bikeBoostEnabled->isChecked());

    zone->ClearValidTeamTypes();
    for (int32_t teamType : ui.validTeamTypes->GetIntegerList()) {
      zone->InsertValidTeamTypes((int8_t)teamType);
    }

    zone->SetTrackTeam(ui.trackTeam->isChecked());

    zone->ClearDropSetIDs();
    for (uint32_t dropSetID : ui.dropSetIDs->GetUnsignedIntegerList()) {
      zone->InsertDropSetIDs(dropSetID);
    }

    zone->ClearSkillBlacklist();
    for (uint32_t skillID : ui.skillBlacklist->GetUnsignedIntegerList()) {
      zone->InsertSkillBlacklist(skillID);
    }

    zone->ClearSkillWhitelist();
    for (uint32_t skillID : ui.skillWhitelist->GetUnsignedIntegerList()) {
      zone->InsertSkillWhitelist(skillID);
    }

    auto triggers = ui.triggers->GetObjectList<objects::ServerZoneTrigger>();
    zone->SetTriggers(triggers);
  }
}

bool ZoneWindow::GetSpotPosition(uint32_t dynamicMapID, uint32_t spotID,
                                 float& x, float& y, float& rot) const {
  if (spotID == 0 || dynamicMapID == 0) {
    return false;
  }

  auto definitions = mMainWindow->GetDefinitions();

  auto spots = definitions->GetSpotData(dynamicMapID);
  auto spotIter = spots.find(spotID);
  if (spotIter != spots.end()) {
    x = spotIter->second->GetCenterX();
    y = spotIter->second->GetCenterY();
    rot = spotIter->second->GetRotation();

    return true;
  }

  return false;
}

void ZoneWindow::BindNPCs() {
  std::vector<std::shared_ptr<libcomp::Object>> npcs;
  for (auto npc : mMergedZone->Definition->GetNPCs()) {
    npcs.push_back(npc);
  }

  ui.npcs->SetObjectList(npcs);
}

void ZoneWindow::BindObjects() {
  std::vector<std::shared_ptr<libcomp::Object>> objs;
  for (auto obj : mMergedZone->Definition->GetObjects()) {
    objs.push_back(obj);
  }

  ui.objects->SetObjectList(objs);
}

void ZoneWindow::BindSpawns() {
  // Sort by key
  std::map<uint32_t, std::shared_ptr<libcomp::Object>> spawnSort;
  std::map<uint32_t, std::shared_ptr<libcomp::Object>> sgSort;
  std::map<uint32_t, std::shared_ptr<libcomp::Object>> slgSort;

  for (auto& sPair : mMergedZone->Definition->GetSpawns()) {
    spawnSort[sPair.first] = sPair.second;
  }

  for (auto& sgPair : mMergedZone->Definition->GetSpawnGroups()) {
    sgSort[sgPair.first] = sgPair.second;
  }

  for (auto& slgPair : mMergedZone->Definition->GetSpawnLocationGroups()) {
    slgSort[slgPair.first] = slgPair.second;
  }

  std::vector<std::shared_ptr<libcomp::Object>> spawns;
  for (auto& sPair : spawnSort) {
    spawns.push_back(sPair.second);
  }

  std::vector<std::shared_ptr<libcomp::Object>> sgs;
  for (auto& sgPair : sgSort) {
    sgs.push_back(sgPair.second);
  }

  std::vector<std::shared_ptr<libcomp::Object>> slgs;
  for (auto& slgPair : slgSort) {
    slgs.push_back(slgPair.second);
  }

  ui.spawns->SetObjectList(spawns);
  ui.spawnGroups->SetObjectList(sgs);
  ui.spawnLocationGroups->SetObjectList(slgs);

  // Build these in order as they are dependent
  RebuildNamedDataSet("Spawn");
  RebuildNamedDataSet("SpawnGroup");
  RebuildNamedDataSet("SpawnLocationGroup");
}

void ZoneWindow::BindSpots() {
  auto zone = mMergedZone->Definition;

  std::vector<std::shared_ptr<libcomp::Object>> spots;

  auto definitions = mMainWindow->GetDefinitions();
  auto spotDefs = definitions->GetSpotData(zone->GetDynamicMapID());

  // Add defined spots first (valid or not)
  for (auto& spotPair : zone->GetSpots()) {
    auto iter = spotDefs.find(spotPair.first);
    if (iter != spotDefs.end()) {
      spots.push_back(iter->second);
    } else {
      spots.push_back(spotPair.second);
    }
  }

  // Add all remaining definitions next
  for (auto& spotPair : spotDefs) {
    if (!zone->SpotsKeyExists(spotPair.first)) {
      spots.push_back(spotPair.second);
    }
  }

  ui.spots->SetObjectList(spots);
}

void ZoneWindow::DrawMap() {
  auto zone = mMergedZone->Definition;
  if (!mZoneData || !zone) {
    return;
  }

  auto xScroll = ui.mapScrollArea->horizontalScrollBar()->value();
  auto yScroll = ui.mapScrollArea->verticalScrollBar()->value();

  ui.drawTarget->clear();

  QPicture pic;
  QPainter painter(&pic);

  // Keep track of what is selected to draw on top of everything else
  std::set<std::shared_ptr<libcomp::Object>> highlight;

  // Draw boundaries and AI nav points first
  if (ui.actionShowBoundaries->isChecked()) {
    std::set<uint32_t> selectedRows;
    if (ui.tabs->currentWidget() == ui.tabGeometry) {
      for (auto item : ui.treeBoundaries->selectedItems()) {
        selectedRows.insert((uint32_t)item->data(0, Qt::DisplayRole).toInt());
      }
    }

    auto boundaries = mQmpFile->GetBoundaries();
    if (selectedRows.size() > 0) {
      // Move selected to the end of the list so they paint on top
      auto selected = boundaries;
      selected.remove_if(
          [selectedRows](const std::shared_ptr<objects::QmpBoundary>& b) {
            return selectedRows.find(b->GetID()) == selectedRows.end();
          });

      boundaries.remove_if(
          [selectedRows](const std::shared_ptr<objects::QmpBoundary>& b) {
            return selectedRows.find(b->GetID()) != selectedRows.end();
          });

      for (auto& b : selected) {
        boundaries.push_back(b);
      }
    }

    painter.setPen(QPen(COLOR_BOUNDARY, 1, Qt::DashLine));
    painter.setBrush(QBrush(COLOR_BOUNDARY));

    for (auto boundary : boundaries) {
      bool isSelected =
          selectedRows.find(boundary->GetID()) != selectedRows.end();
      if (isSelected) {
        painter.setPen(QPen(COLOR_SELECTED));
        painter.setBrush(QBrush(COLOR_SELECTED));
      }

      int32_t span = boundary->GetSize() / 2;
      int32_t x1 = Scale(boundary->GetCenterX() + span);
      int32_t y1 = Scale(-(boundary->GetCenterY() - span));
      int32_t x2 = Scale(boundary->GetCenterX() - span);
      int32_t y2 = Scale(-(boundary->GetCenterY() + span));

      painter.drawLine(x1, y1, x2, y1);
      painter.drawLine(x2, y1, x2, y2);
      painter.drawLine(x2, y2, x1, y2);
      painter.drawLine(x1, y2, x1, y1);

      // Write ID in the center
      painter.drawText(
          QPoint(Scale(boundary->GetCenterX()), Scale(-boundary->GetCenterY())),
          libcomp::String("%1").Arg(boundary->GetID()).C());

      if (isSelected) {
        painter.setPen(QPen(COLOR_BOUNDARY, 1, Qt::DashLine));
        painter.setBrush(QBrush(COLOR_BOUNDARY));
      }
    }
  }

  if (ui.actionShowNavPoints->isChecked()) {
    std::set<uint32_t> selectedRows;
    if (ui.tabs->currentWidget() == ui.tabGeometry) {
      auto selected = ui.tblNavPoints->selectedItems();
      for (auto s : selected) {
        auto w = ui.tblNavPoints->item(s->row(), 0);
        if (w) {
          selectedRows.insert((uint32_t)w->text().toInt());
        }
      }
    }

    std::list<std::shared_ptr<objects::QmpNavPoint>> navPoints;
    for (auto boundary : mQmpFile->GetBoundaries()) {
      for (auto p : boundary->GetNavPoints()) {
        bool isSelected =
            selectedRows.find(p->GetPointID()) != selectedRows.end();
        if (!isSelected) {
          DrawNavPoint(p, false, painter);
        } else {
          highlight.insert(p);
        }
      }
    }
  }

  // Draw geometry
  std::unordered_map<uint32_t, uint8_t> elems;
  for (auto elem : mQmpFile->GetElements()) {
    elems[elem->GetID()] = (uint8_t)elem->GetType();
  }

  for (auto boundary : mQmpFile->GetBoundaries()) {
    for (auto line : boundary->GetLines()) {
      switch (elems[line->GetElementID()]) {
        case 1:
          // One way
          painter.setPen(QPen(COLOR_1WAY));
          painter.setBrush(QBrush(COLOR_1WAY));
          break;
        case 2:
          // Toggleable
          painter.setPen(QPen(COLOR_TOGGLE1));
          painter.setBrush(QBrush(COLOR_TOGGLE1));
          break;
        case 3:
          // Toggleable (wired up to close?)
          painter.setPen(QPen(COLOR_TOGGLE2));
          painter.setBrush(QBrush(COLOR_TOGGLE2));
          break;
        default:
          painter.setPen(QPen(COLOR_GENERIC));
          painter.setBrush(QBrush(COLOR_GENERIC));
          break;
      }

      painter.drawLine(Scale(line->GetX1()), Scale(-line->GetY1()),
                       Scale(line->GetX2()), Scale(-line->GetY2()));
    }
  }

  auto definitions = mMainWindow->GetDefinitions();
  auto spots = definitions->GetSpotData(zone->GetDynamicMapID());

  switch (ui.tabs->currentIndex()) {
    case 1:  // NPCs
    {
      auto npc = ui.npcs->GetActiveObject();
      if (npc) {
        highlight.insert(npc);
      }
    } break;
    case 2:  // Objects
    {
      auto obj = ui.objects->GetActiveObject();
      if (obj) {
        highlight.insert(obj);
      }
    } break;
    case 3:  // Spawn Types
    {
      // If a SpawnLocationGroup is selected, highlight all bound
      // spots
      if (ui.tabSpawnTypes->currentIndex() == 2) {
        auto slg = std::dynamic_pointer_cast<objects::SpawnLocationGroup>(
            ui.spawnLocationGroups->GetActiveObject());
        if (slg) {
          for (uint32_t spotID : slg->GetSpotIDs()) {
            auto spotIter = spots.find(spotID);
            if (spotIter != spots.end()) {
              highlight.insert(spotIter->second);
            }
          }

          for (auto loc : slg->GetLocations()) {
            highlight.insert(loc);
          }
        }
      }
    } break;
    case 4:  // Spots
    {
      auto spot = std::dynamic_pointer_cast<objects::MiSpotData>(
          ui.spots->GetActiveObject());
      if (spot) {
        highlight.insert(spot);

        auto serverSpot = zone->GetSpots(spot->GetID());
        if (serverSpot && serverSpot->GetSpawnArea()) {
          highlight.insert(serverSpot->GetSpawnArea());
        }
      }
    } break;
    case 0:  // Zone
    default:
      break;
  }

  QFont font = painter.font();
  font.setPixelSize(10);
  painter.setFont(font);

  // Draw spots
  std::set<uint8_t> showSpotTypes;
  for (auto act : ui.menuShowSpots->actions()) {
    int type = act->data().toInt();
    if (type && act->isChecked()) {
      showSpotTypes.insert((uint8_t)type);
    }
  }

  for (auto spotPair : spots) {
    auto spotDef = spotPair.second;
    if (highlight.find(spotDef) == highlight.end() &&
        showSpotTypes.find((uint8_t)spotDef->GetType()) !=
            showSpotTypes.end()) {
      DrawSpot(spotDef, false, painter);
    }
  }

  // Draw the starting point
  painter.setPen(QPen(COLOR_PLAYER));
  painter.setBrush(QBrush(COLOR_PLAYER));

  painter.drawEllipse(QPoint(Scale(mMergedZone->CurrentZone->GetStartingX()),
                             Scale(-mMergedZone->CurrentZone->GetStartingY())),
                      3, 3);

  // Draw NPCs
  if (ui.actionShowNPCs->isChecked()) {
    for (auto npc : zone->GetNPCs()) {
      if (highlight.find(npc) == highlight.end()) {
        DrawNPC(npc, false, painter);
      }
    }
  }

  // Draw Objects
  if (ui.actionShowObjects->isChecked()) {
    for (auto obj : zone->GetObjects()) {
      if (highlight.find(obj) == highlight.end()) {
        DrawObject(obj, false, painter);
      }
    }
  }

  // Draw selected object on top of the others
  for (auto h : highlight) {
    auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(h);
    auto obj = std::dynamic_pointer_cast<objects::ServerObject>(h);
    auto spot = std::dynamic_pointer_cast<objects::MiSpotData>(h);
    auto loc = std::dynamic_pointer_cast<objects::SpawnLocation>(h);
    auto navPoint = std::dynamic_pointer_cast<objects::QmpNavPoint>(h);
    if (npc) {
      DrawNPC(npc, true, painter);
    } else if (obj) {
      DrawObject(obj, true, painter);
    } else if (spot) {
      DrawSpot(spot, true, painter);
    } else if (loc) {
      DrawSpawnLocation(loc, painter);
    } else if (navPoint) {
      DrawNavPoint(navPoint, true, painter);
    }
  }

  painter.end();

  auto bounds = pic.boundingRect();
  mOffsetX = bounds.topLeft().x();
  mOffsetY = -bounds.topLeft().y();

  ui.drawTarget->setPicture(pic);

  ui.mapScrollArea->horizontalScrollBar()->setValue(xScroll);
  ui.mapScrollArea->verticalScrollBar()->setValue(yScroll);
}

void ZoneWindow::DrawNPC(const std::shared_ptr<objects::ServerNPC>& npc,
                         bool selected, QPainter& painter) {
  float x = npc->GetX();
  float y = npc->GetY();
  float rot = npc->GetRotation();
  GetSpotPosition(mMergedZone->Definition->GetDynamicMapID(), npc->GetSpotID(),
                  x, y, rot);

  if (selected) {
    painter.setPen(QPen(COLOR_SELECTED));
    painter.setBrush(QBrush(COLOR_SELECTED));
  } else {
    painter.setPen(QPen(COLOR_NPC));
    painter.setBrush(QBrush(COLOR_NPC));
  }

  painter.drawEllipse(QPoint(Scale(x), Scale(-y)), 3, 3);

  painter.drawText(QPoint(Scale(x) + 5, Scale(-y)),
                   libcomp::String("%1").Arg(npc->GetID()).C());
}

void ZoneWindow::DrawObject(const std::shared_ptr<objects::ServerObject>& obj,
                            bool selected, QPainter& painter) {
  float x = obj->GetX();
  float y = obj->GetY();
  float rot = obj->GetRotation();
  GetSpotPosition(mMergedZone->Definition->GetDynamicMapID(), obj->GetSpotID(),
                  x, y, rot);

  if (selected) {
    painter.setPen(QPen(COLOR_SELECTED));
    painter.setBrush(QBrush(COLOR_SELECTED));
  } else {
    painter.setPen(QPen(COLOR_OBJECT));
    painter.setBrush(QBrush(COLOR_OBJECT));
  }

  painter.drawEllipse(QPoint(Scale(x), Scale(-y)), 3, 3);

  painter.drawText(QPoint(Scale(x) + 5, Scale(-y)),
                   libcomp::String("%1").Arg(obj->GetID()).C());
}

void ZoneWindow::DrawSpawnLocation(
    const std::shared_ptr<objects::SpawnLocation>& loc, QPainter& painter) {
  float x1 = loc->GetX();
  float y1 = -loc->GetY();

  float x2 = x1 + loc->GetWidth();
  float y2 = y1 + loc->GetHeight();

  std::vector<std::pair<float, float>> points;
  points.push_back(std::pair<float, float>(x1, y1));
  points.push_back(std::pair<float, float>(x2, y1));
  points.push_back(std::pair<float, float>(x2, y2));
  points.push_back(std::pair<float, float>(x1, y2));

  // Spawn locs only show when selected so no second color here
  painter.setPen(QPen(COLOR_SPAWN_LOC));
  painter.setBrush(QBrush(COLOR_SPAWN_LOC));

  painter.drawLine(Scale(points[0].first), Scale(points[0].second),
                   Scale(points[1].first), Scale(points[1].second));
  painter.drawLine(Scale(points[1].first), Scale(points[1].second),
                   Scale(points[2].first), Scale(points[2].second));
  painter.drawLine(Scale(points[2].first), Scale(points[2].second),
                   Scale(points[3].first), Scale(points[3].second));
  painter.drawLine(Scale(points[3].first), Scale(points[3].second),
                   Scale(points[0].first), Scale(points[0].second));
}

void ZoneWindow::DrawSpot(const std::shared_ptr<objects::MiSpotData>& spotDef,
                          bool selected, QPainter& painter) {
  float xc = spotDef->GetCenterX();
  float yc = -spotDef->GetCenterY();
  float rot = -spotDef->GetRotation();

  float x1 = xc - spotDef->GetSpanX();
  float y1 = yc + spotDef->GetSpanY();

  float x2 = xc + spotDef->GetSpanX();
  float y2 = yc - spotDef->GetSpanY();

  std::vector<std::pair<float, float>> points;
  points.push_back(std::pair<float, float>(x1, y1));
  points.push_back(std::pair<float, float>(x2, y1));
  points.push_back(std::pair<float, float>(x2, y2));
  points.push_back(std::pair<float, float>(x1, y2));

  for (auto& p : points) {
    float x = p.first;
    float y = p.second;
    p.first = (float)(((x - xc) * cos(rot)) - ((y - yc) * sin(rot)) + xc);
    p.second = (float)(((x - xc) * sin(rot)) + ((y - yc) * cos(rot)) + yc);
  }

  if (selected) {
    painter.setPen(QPen(COLOR_SELECTED));
    painter.setBrush(QBrush(COLOR_SELECTED));
  } else {
    painter.setPen(QPen(COLOR_SPOT));
    painter.setBrush(QBrush(COLOR_SPOT));
  }

  painter.drawLine(Scale(points[0].first), Scale(points[0].second),
                   Scale(points[1].first), Scale(points[1].second));
  painter.drawLine(Scale(points[1].first), Scale(points[1].second),
                   Scale(points[2].first), Scale(points[2].second));
  painter.drawLine(Scale(points[2].first), Scale(points[2].second),
                   Scale(points[3].first), Scale(points[3].second));
  painter.drawLine(Scale(points[3].first), Scale(points[3].second),
                   Scale(points[0].first), Scale(points[0].second));

  painter.drawText(QPoint(Scale(points[3].first), Scale(points[3].second) + 10),
                   libcomp::String("[%1] %2")
                       .Arg((uint8_t)spotDef->GetType())
                       .Arg(spotDef->GetID())
                       .C());
}

void ZoneWindow::DrawNavPoint(
    const std::shared_ptr<objects::QmpNavPoint>& navPoint, bool selected,
    QPainter& painter) {
  if (selected) {
    painter.setPen(QPen(COLOR_SELECTED));
    painter.setBrush(QBrush(COLOR_SELECTED));
  } else {
    painter.setPen(QPen(COLOR_NAVPOINT));
    painter.setBrush(QBrush(COLOR_NAVPOINT));
  }

  painter.drawEllipse(QPoint(Scale(navPoint->GetX()), Scale(-navPoint->GetY())),
                      3, 3);

  painter.drawText(
      QPoint(Scale(navPoint->GetX()) + 5, Scale(-navPoint->GetY())),
      libcomp::String("%1").Arg(navPoint->GetPointID()).C());

  if (selected && navPoint->DistancesCount() > 0) {
    // Draw distances
    for (auto& boundary : mQmpFile->GetBoundaries()) {
      for (auto& other : boundary->GetNavPoints()) {
        if (navPoint->DistancesKeyExists(other->GetPointID())) {
          painter.drawLine(QLine(Scale(navPoint->GetX()),
                                 Scale(-navPoint->GetY()), Scale(other->GetX()),
                                 Scale(-other->GetY())));
        }
      }
    }
  }
}

int32_t ZoneWindow::Scale(int32_t point) {
  return (int32_t)(point / ui.zoomSlider->value());
}

int32_t ZoneWindow::Scale(float point) {
  return (int32_t)(point / (float)ui.zoomSlider->value());
}

QPointF ZoneWindow::GetNavPointLocation(QPointF p1, QPointF vert, QPointF p2,
                                        bool& valid) {
  if ((p1.x() == p2.x() && p2.x() == vert.x()) ||
      (p1.y() == p2.y() && p1.y() == vert.y()) || p1 == vert || p2 == vert) {
    // Point, or vertical/horizontal line, no reason to add a nav point
    valid = false;
    return QPointF();
  }

  // First place a point the set distance from the vertex along point 1
  QPointF p = vert;
  double distance = 50.0;

  if (p1.x() != vert.x()) {
    double slope = (p1.y() - vert.y()) / (p1.x() - vert.x());
    double denom = std::sqrt(1.0 + std::pow(slope, 2));

    double xOffset = distance / denom;
    double yOffset = fabs((slope * distance) / denom);

    p.setX(p1.x() <= vert.x() ? (vert.x() - xOffset) : (vert.x() + xOffset));
    p.setY(p1.y() <= vert.y() ? (vert.y() - yOffset) : (vert.y() + yOffset));
  } else {
    p.setY(p1.y() <= vert.y() ? (vert.y() - distance) : (vert.y() + distance));
  }

  // Next calculate the angle created by the three points
  auto cProd = (p2.x() - vert.x()) * (vert.y() - p1.y()) -
               (p2.y() - vert.y()) * (vert.x() - p1.x());
  auto dProd = (p2.x() - vert.x()) * (vert.x() - p1.x()) +
               (p2.y() - vert.y()) * (vert.y() - p1.y());
  double radians = atan2(cProd, dProd);

  if (radians == libcomp::PI) {
    // Straight line
    valid = false;
    return QPointF();
  }

  // Finally rotate the point around the vertex so it "extends" the corner
  valid = true;

  if (radians < 0.0) {
    // Obtuse angle
    radians = libcomp::PI / 2.0 - radians / 2.0;
  } else {
    // Acute angle
    radians = libcomp::PI * 1.5 - radians / 2.0;
  }

  return RotatePoint(p, vert, radians);
}

std::list<QPointF> ZoneWindow::GetLineNavPointLocations(QPointF p1,
                                                        QPointF p2) {
  std::list<QPointF> points;

  if (p1 == p2) {
    // Coordinates are single point, ignore
    return points;
  }

  double distance = 50.0;
  if (p1.x() == p2.x()) {
    if (p2.y() > p1.y()) {
      // Up
      points.push_back(QPointF(p2.x() - distance, p2.y() + distance));
      points.push_back(QPointF(p2.x() + distance, p2.y() + distance));
    } else {
      // Down
      points.push_back(QPointF(p2.x() + distance, p2.y() - distance));
      points.push_back(QPointF(p2.x() - distance, p2.y() - distance));
    }

    return points;
  } else if (p1.y() == p2.y()) {
    if (p2.x() > p1.x()) {
      // Right
      points.push_back(QPointF(p2.x() + distance, p2.y() + distance));
      points.push_back(QPointF(p2.x() + distance, p2.y() - distance));
    } else {
      // Left
      points.push_back(QPointF(p2.x() - distance, p2.y() - distance));
      points.push_back(QPointF(p2.x() - distance, p2.y() + distance));
    }

    return points;
  }

  // Extend 50 units and move 50 units "left/right" using slope
  double slope = (p2.y() - p1.y()) / (p2.x() - p1.x());
  double denom = std::sqrt(1.0 + std::pow(slope, 2));

  double xOffset = distance / denom;
  double yOffset = fabs((slope * distance) / denom);

  // Get the perpendicular slope to calculate the second transforms
  double pSlope = -1.0 / slope;
  denom = std::sqrt(1.0 + std::pow(pSlope, 2));

  double xOffset2 = distance / denom;
  double yOffset2 = fabs((pSlope * distance) / denom);

  if (p2.x() > p1.x()) {
    // Right
    double pY1 = slope > 0.0 ? (p2.y() + yOffset + yOffset2)
                             : (p2.y() - yOffset - yOffset2);
    double pY2 = slope > 0.0 ? (p2.y() + yOffset - yOffset2)
                             : (p2.y() - yOffset + yOffset2);

    points.push_back(QPointF(p2.x() + xOffset - xOffset2, pY1));
    points.push_back(QPointF(p2.x() + xOffset + xOffset2, pY2));
  } else {
    // Left
    double pY1 = slope > 0.0 ? (p2.y() - yOffset + yOffset2)
                             : (p2.y() + yOffset - yOffset2);
    double pY2 = slope > 0.0 ? (p2.y() - yOffset - yOffset2)
                             : (p2.y() + yOffset + yOffset2);

    points.push_back(QPointF(p2.x() - xOffset - xOffset2, pY1));
    points.push_back(QPointF(p2.x() - xOffset + xOffset2, pY2));
  }

  return points;
}

QPointF ZoneWindow::RotatePoint(QPointF p, QPointF origin, double radians) {
  double xDelta = p.x() - origin.x();
  double yDelta = p.y() - origin.y();

  return QPointF(
      ((xDelta * cos(radians)) - (yDelta * sin(radians))) + origin.x(),
      ((xDelta * sin(radians)) + (yDelta * cos(radians))) + origin.y());
}
