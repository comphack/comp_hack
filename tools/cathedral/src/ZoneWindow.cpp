/**
 * @file tools/cathedral/src/ZoneWindow.cpp
 * @ingroup map
 *
 * @author HACKfrost
 *
 * @brief Zone window which allows for visualization and modification of zone
 *  map data.
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include <PushIgnore.h>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QIODevice>
#include <QMessageBox>
#include <QPainter>
#include <QPicture>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QToolTip>
#include <PopIgnore.h>

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
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>
#include <ServerZoneTrigger.h>

// C++11 Standard Includes
#include <cmath>
#include <map>

// libcomp Includes
#include <Log.h>
#include <ServerDataManager.h>

const QColor COLOR_SELECTED = Qt::red;
const QColor COLOR_PLAYER = Qt::magenta;
const QColor COLOR_NPC = Qt::darkRed;
const QColor COLOR_OBJECT = Qt::blue;
const QColor COLOR_SPOT = Qt::darkGreen;

// Barrier colors
const QColor COLOR_GENERIC = Qt::black;
const QColor COLOR_1WAY = Qt::darkGray;
const QColor COLOR_TOGGLE1 = Qt::darkYellow;
const QColor COLOR_TOGGLE2 = Qt::darkCyan;

ZoneWindow::ZoneWindow(MainWindow *pMainWindow, QWidget *p)
    : QMainWindow(p), mMainWindow(pMainWindow), mDrawTarget(0)
{
    ui.setupUi(this);

    mMergedZone = std::make_shared<MergedZone>();

    ui.npcs->Bind(pMainWindow, true);
    ui.objects->Bind(pMainWindow, false);
    ui.spawns->SetMainWindow(pMainWindow);
    ui.spawnGroups->SetMainWindow(pMainWindow);
    ui.spawnLocationGroups->SetMainWindow(pMainWindow);
    ui.spots->SetMainWindow(pMainWindow);

    ui.zoneID->Bind(pMainWindow, "ZoneData");
    ui.validTeamTypes->Setup(DynamicItemType_t::PRIMITIVE_INT,
        pMainWindow);
    ui.dropSetIDs->Setup(DynamicItemType_t::PRIMITIVE_UINT,
        pMainWindow);
    ui.skillBlacklist->Setup(DynamicItemType_t::PRIMITIVE_UINT,
        pMainWindow);
    ui.skillWhitelist->Setup(DynamicItemType_t::PRIMITIVE_UINT,
        pMainWindow);
    ui.triggers->Setup(DynamicItemType_t::OBJ_ZONE_TRIGGER,
        pMainWindow);

    ui.partialDynamicMapIDs->Setup(DynamicItemType_t::PRIMITIVE_UINT,
        pMainWindow);

    connect(ui.actionRefresh, SIGNAL(triggered()), this, SLOT(Refresh()));
    connect(ui.showNPCs, SIGNAL(toggled(bool)), this, SLOT(ShowToggled(bool)));
    connect(ui.showObjects, SIGNAL(toggled(bool)), this,
        SLOT(ShowToggled(bool)));

    connect(ui.addNPC, SIGNAL(clicked()), this, SLOT(AddNPC()));
    connect(ui.addObject, SIGNAL(clicked()), this, SLOT(AddObject()));
    connect(ui.addSpawn, SIGNAL(clicked()), this, SLOT(AddSpawn()));
    connect(ui.removeNPC, SIGNAL(clicked()), this, SLOT(RemoveNPC()));
    connect(ui.removeObject, SIGNAL(clicked()), this, SLOT(RemoveObject()));
    connect(ui.removeSpawn, SIGNAL(clicked()), this, SLOT(RemoveSpawn()));

    connect(ui.actionLoad, SIGNAL(triggered()), this, SLOT(LoadZoneFile()));
    connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(SaveFile()));
    connect(ui.actionSaveAll, SIGNAL(triggered()), this, SLOT(SaveAllFiles()));

    connect(ui.actionPartialsLoadFile, SIGNAL(triggered()), this,
        SLOT(LoadPartialFile()));
    connect(ui.actionPartialsLoadDirectory, SIGNAL(triggered()),
        this, SLOT(LoadPartialDirectory()));
    connect(ui.actionPartialsApply, SIGNAL(triggered()),
        this, SLOT(ApplyPartials()));

    connect(ui.tabs, SIGNAL(selectedObjectChanged()), this,
        SLOT(SelectListObject()));
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

    connect(ui.zoneView, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(ZoneViewUpdated()));
    connect(ui.tabSpawnTypes, SIGNAL(currentChanged(int)), this,
        SLOT(SpawnTabChanged()));
    connect(ui.zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(Zoom()));

    QShortcut *shortcut = new QShortcut(QKeySequence("F5"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(Refresh()));
}

ZoneWindow::~ZoneWindow()
{
}

std::shared_ptr<MergedZone> ZoneWindow::GetMergedZone() const
{
    return mMergedZone;
}

std::map<uint32_t, std::shared_ptr<
    objects::ServerZonePartial>> ZoneWindow::GetLoadedPartials() const
{
    return mZonePartials;
}

std::set<uint32_t> ZoneWindow::GetSelectedPartials() const
{
    return mSelectedPartials;
}

bool ZoneWindow::ShowZone()
{
    auto zone = mMergedZone->CurrentZone;
    if(!zone)
    {
        return false;
    }

    // Don't bother showing the bazaar settings if none are configured
    if(zone->BazaarsCount() == 0)
    {
        ui.grpBazaar->hide();
    }
    else
    {
        ui.grpBazaar->show();
    }

    mSelectedPartials.clear();
    ResetAppliedPartials();

    UpdateMergedZone(false);

    LoadProperties();

    setWindowTitle(libcomp::String("COMP_hack Cathedral of Content - Zone %1"
        " (%2)").Arg(zone->GetID()).Arg(zone->GetDynamicMapID()).C());

    if(LoadMapFromZone())
    {
        show();
        return true;
    }

    return false;
}

void ZoneWindow::RebuildNamedDataSet(const libcomp::String& objType)
{
    std::vector<libcomp::String> names;
    if(objType == "Spawn")
    {
        auto devilDataSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("DevilData"));
        auto titleDataSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("CTitleData"));

        std::map<uint32_t, std::shared_ptr<objects::Spawn>> sort;
        for(auto& sPair : mMergedZone->Definition->GetSpawns())
        {
            sort[sPair.first] = sPair.second;
        }

        std::vector<std::shared_ptr<libcomp::Object>> spawns;
        for(auto& sPair : sort)
        {
            auto spawn = sPair.second;
            auto devilData = std::dynamic_pointer_cast<objects::MiDevilData>(
                devilDataSet->GetObjectByID(spawn->GetEnemyType()));

            libcomp::String name(devilData
                ? devilDataSet->GetName(devilData) : "[Unknown]");

            uint32_t titleID = spawn->GetVariantType()
                ? spawn->GetVariantType() :
                (devilData ? (uint32_t)devilData->GetBasic()->GetTitle() : 0);
            if(titleID)
            {
                auto title = std::dynamic_pointer_cast<objects::MiCTitleData>(
                    titleDataSet->GetObjectByID(titleID));
                if(title)
                {
                    name = libcomp::String("%1 %2").Arg(title->GetTitle())
                        .Arg(name);
                }
            }

            int8_t lvl = spawn->GetLevel();
            if(lvl == -1 && devilData)
            {
                lvl = (int8_t)devilData->GetGrowth()->GetBaseLevel();
            }

            name = libcomp::String("%1 Lv:%2").Arg(name).Arg(lvl);

            if(spawn->GetCategory() == objects::Spawn::Category_t::ALLY)
            {
                name = libcomp::String("%1 [Ally]").Arg(name);
            }

            spawns.push_back(spawn);
            names.push_back(name);
        }
        
        auto newData = std::make_shared<BinaryDataNamedSet>(
            [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
            {
                return std::dynamic_pointer_cast<objects::Spawn>(obj)
                    ->GetID();
            });
        newData->MapRecords(spawns, names);
        mMainWindow->RegisterBinaryDataSet("Spawn", newData);
    }
    else if(objType == "SpawnGroup")
    {
        auto spawnSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("Spawn"));

        std::map<uint32_t, std::shared_ptr<objects::SpawnGroup>> sort;
        for(auto& sPair : mMergedZone->Definition->GetSpawnGroups())
        {
            sort[sPair.first] = sPair.second;
        }

        std::vector<std::shared_ptr<libcomp::Object>> sgs;
        for(auto& sgPair : sort)
        {
            std::list<libcomp::String> spawnStrings;

            auto sg = sgPair.second;
            for(auto& spawnPair : sg->GetSpawns())
            {
                auto spawn = spawnSet->GetObjectByID(spawnPair.first);
                libcomp::String txt(spawn
                    ? spawnSet->GetName(spawn) : "[Unknown]");
                spawnStrings.push_back(libcomp::String("%1 x%2 [%3]")
                    .Arg(txt).Arg(spawnPair.second).Arg(spawnPair.first));
            }

            sgs.push_back(sg);
            names.push_back(libcomp::String::Join(spawnStrings, ",\n\r    "));
        }

        auto newData = std::make_shared<BinaryDataNamedSet>(
            [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
            {
                return std::dynamic_pointer_cast<objects::SpawnGroup>(obj)
                    ->GetID();
            });
        newData->MapRecords(sgs, names);
        mMainWindow->RegisterBinaryDataSet("SpawnGroup", newData);
    }
    else if(objType == "SpawnLocationGroup")
    {
        auto sgSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("SpawnGroup"));

        std::map<uint32_t, std::shared_ptr<objects::SpawnLocationGroup>> sort;
        for(auto& sPair : mMergedZone->Definition->GetSpawnLocationGroups())
        {
            sort[sPair.first] = sPair.second;
        }

        std::vector<std::shared_ptr<libcomp::Object>> slgs;
        for(auto& slgPair : mMergedZone->Definition->GetSpawnLocationGroups())
        {
            std::list<libcomp::String> sgStrings;

            auto slg = slgPair.second;
            for(uint32_t sgID : slg->GetGroupIDs())
            {
                auto sg = sgSet->GetObjectByID(sgID);
                libcomp::String txt(sg ? sgSet->GetName(sg).Replace("\n\r", "")
                    : "[Unknown]");
                sgStrings.push_back(libcomp::String("{ %1 } @%2").Arg(txt)
                    .Arg(sgID));
            }

            slgs.push_back(slg);
            names.push_back(libcomp::String::Join(sgStrings, ",\n\r    "));
        }

        auto newData = std::make_shared<BinaryDataNamedSet>(
            [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
            {
                return std::dynamic_pointer_cast<objects::SpawnLocationGroup>(
                    obj)->GetID();
            });
        newData->MapRecords(slgs, names);
        mMainWindow->RegisterBinaryDataSet("SpawnLocationGroup", newData);
    }
}

std::list<std::shared_ptr<objects::Action>>
    ZoneWindow::GetLoadedActions(bool forUpdate)
{
    std::list<std::shared_ptr<objects::Action>> actions;
    if(forUpdate)
    {
        // Make sure all controls are saved and not bound during the update
        Refresh();
    }

    // Get all loaded partial actions
    for(auto& partialPair : mZonePartials)
    {
        auto partial = partialPair.second;
        for(auto npc : partial->GetNPCs())
        {
            for(auto action : npc->GetActions())
            {
                actions.push_back(action);
            }
        }

        for(auto obj : partial->GetObjects())
        {
            for(auto action : obj->GetActions())
            {
                actions.push_back(action);
            }
        }

        for(auto& sgPair : partial->GetSpawnGroups())
        {
            for(auto action : sgPair.second->GetSpawnActions())
            {
                actions.push_back(action);
            }

            for(auto action : sgPair.second->GetDefeatActions())
            {
                actions.push_back(action);
            }
        }

        for(auto& spotPair : partial->GetSpots())
        {
            for(auto action : spotPair.second->GetActions())
            {
                actions.push_back(action);
            }
        }

        for(auto trigger : partial->GetTriggers())
        {
            for(auto action : trigger->GetActions())
            {
                actions.push_back(action);
            }
        }
    }

    // Get all current zone actions
    if(mMergedZone->CurrentZone)
    {
        auto zone = mMergedZone->CurrentZone;
        for(auto npc : zone->GetNPCs())
        {
            for(auto action : npc->GetActions())
            {
                actions.push_back(action);
            }
        }

        for(auto obj : zone->GetObjects())
        {
            for(auto action : obj->GetActions())
            {
                actions.push_back(action);
            }
        }

        for(auto& sgPair : zone->GetSpawnGroups())
        {
            for(auto action : sgPair.second->GetSpawnActions())
            {
                actions.push_back(action);
            }

            for(auto action : sgPair.second->GetDefeatActions())
            {
                actions.push_back(action);
            }
        }

        for(auto& spotPair : zone->GetSpots())
        {
            for(auto action : spotPair.second->GetActions())
            {
                actions.push_back(action);
            }
        }

        for(auto trigger : zone->GetTriggers())
        {
            for(auto action : trigger->GetActions())
            {
                actions.push_back(action);
            }
        }
    }

    return actions;
}

void ZoneWindow::LoadZoneFile()
{
    QSettings settings;

    QString path = QFileDialog::getOpenFileName(this, tr("Open Zone XML"),
        settings.value("datastore").toString(), tr("Zone XML (*.xml)"));
    if(path.isEmpty())
    {
        return;
    }

    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_NO_ERROR != doc.LoadFile(path.toLocal8Bit().constData()))
    {
        LOG_ERROR(libcomp::String("Failed to parse file: %1\n").Arg(
            path.toLocal8Bit().constData()));

        return;
    }

    libcomp::BinaryDataSet pSet([]()
        {
            return std::make_shared<objects::ServerZone>();
        },

        [](const std::shared_ptr<libcomp::Object>& obj)
        {
            return std::dynamic_pointer_cast<objects::ServerZone>(
                obj)->GetID();
        }
    );

    if(!pSet.LoadXml(doc))
    {
        LOG_ERROR(libcomp::String("Failed to load file: %1\n").Arg(
            path.toLocal8Bit().constData()));

        return;
    }

    auto objs = pSet.GetObjects();
    if(1 != objs.size())
    {
        LOG_ERROR("There must be exactly 1 zone in the XML file.\n");

        return;
    }

    auto zone = std::dynamic_pointer_cast<objects::ServerZone>(objs.front());
    if(!zone)
    {
        LOG_ERROR("Internal error loading zone.\n");

        return;
    }

    // Save any properties currently set (do not save to zone file)
    SaveProperties();

    mZonePath = cs(path);
    mMergedZone->Definition = zone;
    mMergedZone->CurrentZone = zone;
    mMergedZone->CurrentPartial = nullptr;

    mMainWindow->UpdateActiveZone(mZonePath);

    ShowZone();
}

void ZoneWindow::LoadPartialDirectory()
{
    QSettings settings;

    QString qPath = QFileDialog::getExistingDirectory(this,
        tr("Load Zone Partial XML folder"),
        settings.value("datastore").toString());
    if(qPath.isEmpty())
    {
        return;
    }

    SaveProperties();

    bool merged = false;

    QDirIterator it(qPath, QStringList() << "*.xml", QDir::Files,
        QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        libcomp::String path = cs(it.next());
        merged |= LoadZonePartials(path);
    }

    if(merged)
    {
        UpdateMergedZone(true);
    }
}

void ZoneWindow::LoadPartialFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getOpenFileName(this,
        tr("Load Zone Partial XML"), settings.value("datastore").toString(),
        tr("Zone Partial XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

    SaveProperties();

    libcomp::String path = cs(qPath);
    if(LoadZonePartials(path))
    {
        UpdateMergedZone(true);
    }
}

void ZoneWindow::SaveFile()
{
    // Save off all properties first
    SaveProperties();

    if(mMergedZone && mMergedZone->CurrentPartial)
    {
        std::set<uint32_t> partialIDs;
        partialIDs.insert(mMergedZone->CurrentPartial->GetID());
        SavePartials(partialIDs);
    }
    else if(mMergedZone && mMergedZone->CurrentZone &&
        mMergedZone->Definition == mMergedZone->CurrentZone)
    {
        SaveZone();
    }
}

void ZoneWindow::SaveAllFiles()
{
    // Save off all properties first
    SaveProperties();

    SaveFile();

    std::set<uint32_t> partialIDs;
    for(auto& pair : mZonePartials)
    {
        partialIDs.insert(pair.first);
    }

    SavePartials(partialIDs);
}

void ZoneWindow::ApplyPartials()
{
    ZonePartialSelector* selector = new ZonePartialSelector(mMainWindow);
    selector->setWindowModality(Qt::ApplicationModal);

    mSelectedPartials = selector->Select();

    delete selector;

    RebuildCurrentZoneDisplay();
    UpdateMergedZone(true);
}

void ZoneWindow::AddNPC()
{
    auto npc = std::make_shared<objects::ServerNPC>();
    if(mMergedZone->CurrentPartial)
    {
        mMergedZone->CurrentPartial->AppendNPCs(npc);
    }
    else
    {
        mMergedZone->CurrentZone->AppendNPCs(npc);
    }

    UpdateMergedZone(true);
}

void ZoneWindow::AddObject()
{
    auto obj = std::make_shared<objects::ServerObject>();
    if(mMergedZone->CurrentPartial)
    {
        mMergedZone->CurrentPartial->AppendObjects(obj);
    }
    else
    {
        mMergedZone->CurrentZone->AppendObjects(obj);
    }

    UpdateMergedZone(true);
}

void ZoneWindow::AddSpawn()
{
    uint32_t nextID = 1;
    switch(ui.tabSpawnTypes->currentIndex())
    {
    case 1:
        while(nextID && mMergedZone->Definition->SpawnGroupsKeyExists(nextID))
        {
            nextID = (uint32_t)(nextID + 1);
        }
        break;
    case 2:
        while(nextID &&
            mMergedZone->Definition->SpawnLocationGroupsKeyExists(nextID))
        {
            nextID = (uint32_t)(nextID + 1);
        }
        break;
    case 0:
    default:
        while(nextID && mMergedZone->Definition->SpawnsKeyExists(nextID))
        {
            nextID = (uint32_t)(nextID + 1);
        }
        break;
    }

    int spawnID = QInputDialog::getInt(this, "Enter an ID", "New ID",
        (int32_t)nextID, 0);
    if(!spawnID)
    {
        return;
    }

    libcomp::String errMsg;
    switch(ui.tabSpawnTypes->currentIndex())
    {
    case 1:
        if(mMergedZone->Definition->SpawnGroupsKeyExists((uint32_t)spawnID))
        {
            errMsg = libcomp::String("Spawn Group ID %1 already exists")
                .Arg(spawnID);
        }
        else
        {
            auto sg = std::make_shared<objects::SpawnGroup>();
            sg->SetID((uint32_t)spawnID);
            if(mMergedZone->CurrentPartial)
            {
                mMergedZone->CurrentPartial->SetSpawnGroups((uint32_t)spawnID,
                    sg);
            }
            else
            {
                mMergedZone->CurrentZone->SetSpawnGroups((uint32_t)spawnID,
                    sg);
            }
        }
        break;
    case 2:
        if(mMergedZone->Definition->SpawnLocationGroupsKeyExists(
            (uint32_t)spawnID))
        {
            errMsg = libcomp::String("Spawn Location Group ID %1 already"
                " exists").Arg(spawnID);
        }
        else
        {
            auto slg = std::make_shared<objects::SpawnLocationGroup>();
            slg->SetID((uint32_t)spawnID);
            if(mMergedZone->CurrentPartial)
            {
                mMergedZone->CurrentPartial->SetSpawnLocationGroups(
                    (uint32_t)spawnID, slg);
            }
            else
            {
                mMergedZone->CurrentZone->SetSpawnLocationGroups(
                    (uint32_t)spawnID, slg);
            }
        }
        break;
    case 0:
    default:
        if(mMergedZone->Definition->SpawnsKeyExists((uint32_t)spawnID))
        {
            errMsg = libcomp::String("Spawn ID %1 already exists")
                .Arg(spawnID);
        }
        else
        {
            auto spawn = std::make_shared<objects::Spawn>();
            spawn->SetID((uint32_t)spawnID);
            if(mMergedZone->CurrentPartial)
            {
                mMergedZone->CurrentPartial->SetSpawns((uint32_t)spawnID,
                    spawn);
            }
            else
            {
                mMergedZone->CurrentZone->SetSpawns((uint32_t)spawnID,
                    spawn);
            }
        }
        break;
    }

    if(errMsg.Length() > 0)
    {
        QMessageBox err;
        err.setText(qs(errMsg));
        err.exec();
    }
    else
    {
        UpdateMergedZone(true);
    }
}

void ZoneWindow::RemoveNPC()
{
    auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(
        ui.npcs->GetActiveObject());
    if(npc)
    {
        if(mMergedZone->CurrentPartial)
        {
            size_t count = mMergedZone->CurrentPartial->NPCsCount();
            for(size_t idx = 0; idx < count; idx++)
            {
                if(mMergedZone->CurrentPartial->GetNPCs(idx) == npc)
                {
                    mMergedZone->CurrentPartial->RemoveNPCs(idx);
                    UpdateMergedZone(true);
                    return;
                }
            }
        }
        else
        {
            size_t count = mMergedZone->CurrentZone->NPCsCount();
            for(size_t idx = 0; idx < count; idx++)
            {
                if(mMergedZone->CurrentZone->GetNPCs(idx) == npc)
                {
                    mMergedZone->CurrentZone->RemoveNPCs(idx);
                    UpdateMergedZone(true);
                    return;
                }
            }
        }
    }
}

void ZoneWindow::RemoveObject()
{
    auto obj = std::dynamic_pointer_cast<objects::ServerObject>(
        ui.objects->GetActiveObject());
    if(obj)
    {
        if(mMergedZone->CurrentPartial)
        {
            size_t count = mMergedZone->CurrentPartial->ObjectsCount();
            for(size_t idx = 0; idx < count; idx++)
            {
                if(mMergedZone->CurrentPartial->GetObjects(idx) == obj)
                {
                    mMergedZone->CurrentPartial->RemoveObjects(idx);
                    UpdateMergedZone(true);
                    return;
                }
            }
        }
        else
        {
            size_t count = mMergedZone->CurrentZone->ObjectsCount();
            for(size_t idx = 0; idx < count; idx++)
            {
                if(mMergedZone->CurrentZone->GetObjects(idx) == obj)
                {
                    mMergedZone->CurrentZone->RemoveObjects(idx);
                    UpdateMergedZone(true);
                    return;
                }
            }
        }
    }
}

void ZoneWindow::RemoveSpawn()
{
    bool updated = false;
    switch(ui.tabSpawnTypes->currentIndex())
    {
    case 1:
        {
            auto sg = std::dynamic_pointer_cast<objects::SpawnGroup>(
                ui.spawnGroups->GetActiveObject());
            if(sg)
            {
                if(mMergedZone->CurrentPartial)
                {
                    mMergedZone->CurrentPartial->RemoveSpawnGroups(sg
                        ->GetID());
                }
                else
                {
                    mMergedZone->CurrentZone->RemoveSpawnGroups(sg->GetID());
                }

                updated = true;
            }
        }
        break;
    case 2:
        {
            auto slg = std::dynamic_pointer_cast<objects::SpawnLocationGroup>(
                ui.spawnLocationGroups->GetActiveObject());
            if(slg)
            {
                if(mMergedZone->CurrentPartial)
                {
                    mMergedZone->CurrentPartial->RemoveSpawnLocationGroups(slg
                        ->GetID());
                }
                else
                {
                    mMergedZone->CurrentZone->RemoveSpawnLocationGroups(slg
                        ->GetID());
                }

                updated = true;
            }
        }
        break;
    case 0:
    default:
        {
            auto spawn = std::dynamic_pointer_cast<objects::Spawn>(
                ui.spawns->GetActiveObject());
            if(spawn)
            {
                if(mMergedZone->CurrentPartial)
                {
                    mMergedZone->CurrentPartial->RemoveSpawns(spawn->GetID());
                }
                else
                {
                    mMergedZone->CurrentZone->RemoveSpawns(spawn->GetID());
                }

                updated = true;
            }
        }
        break;
    }

    if(updated)
    {
        UpdateMergedZone(true);
    }
}

void ZoneWindow::ZoneViewUpdated()
{
    SaveProperties();

    UpdateMergedZone(true);
}

void ZoneWindow::SelectListObject()
{
    DrawMap();
}

void ZoneWindow::SpawnTabChanged()
{
    switch(ui.tabSpawnTypes->currentIndex())
    {
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

void ZoneWindow::Zoom()
{
    DrawMap();
}

void ZoneWindow::ShowToggled(bool checked)
{
    (void)checked;

    DrawMap();
}

void ZoneWindow::Refresh()
{
    LoadMapFromZone();
}

bool ZoneWindow::LoadZonePartials(const libcomp::String& path)
{
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_NO_ERROR != doc.LoadFile(path.C()))
    {
        LOG_ERROR(libcomp::String("Failed to parse file: %1\n").Arg(path));
        return false;
    }
    
    auto rootElem = doc.RootElement();
    if(!rootElem)
    {
        LOG_ERROR(libcomp::String("No root element in file: %1\n").Arg(path));
        return false;
    }

    std::list<std::shared_ptr<objects::ServerZonePartial>> partials;

    auto objNode = rootElem->FirstChildElement("object");
    while(objNode)
    {
        auto partial = std::make_shared<objects::ServerZonePartial>();
        if(!partial || !partial->Load(doc, *objNode))
        {
            break;
        }

        partials.push_back(partial);

        objNode = objNode->NextSiblingElement("object");
    }

    // Add the file if it has partials or no child nodes
    if(partials.size() > 0 || rootElem->FirstChild() == nullptr)
    {
        LOG_INFO(libcomp::String("Loading %1 zone partial(s) from"
            " file: %2\n").Arg(partials.size()).Arg(path));

        std::set<uint32_t> loadedPartials;
        for(auto partial : partials)
        {
            if(mZonePartials.find(partial->GetID()) != mZonePartials.end())
            {
                LOG_WARNING(libcomp::String("Reloaded zone partial %1 from"
                    " file: %2\n").Arg(partial->GetID()).Arg(path));
            }

            mZonePartials[partial->GetID()] = partial;

            mZonePartialFiles[partial->GetID()] = path;

            loadedPartials.insert(partial->GetID());
        }

        ResetAppliedPartials(loadedPartials);

        return true;
    }
    else
    {
        LOG_WARNING(libcomp::String("No zone partials found in file: %1\n")
            .Arg(path));
    }

    return false;
}

void ZoneWindow::SaveZone()
{
    if(mZonePath.Length() == 0 || !mMergedZone || !mMergedZone->CurrentZone)
    {
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

    doc.SaveFile(mZonePath.C());

    LOG_DEBUG(libcomp::String("Updated zone file '%1'\n").Arg(mZonePath));
}

void ZoneWindow::SavePartials(const std::set<uint32_t>& partialIDs)
{
    std::unordered_map<libcomp::String, std::set<uint32_t>> fileMap;
    for(uint32_t partialID : partialIDs)
    {
        fileMap[mZonePartialFiles[partialID]].insert(partialID);
    }

    if(fileMap.size() == 0)
    {
        // Nothing to save
        return;
    }

    for(auto& filePair : fileMap)
    {
        auto path = filePair.first;

        tinyxml2::XMLDocument doc;
        if(tinyxml2::XML_NO_ERROR != doc.LoadFile(path.C()))
        {
            LOG_ERROR(libcomp::String("Failed to parse file for saving: %1\n")
                .Arg(path));
            continue;
        }

        std::unordered_map<uint32_t, tinyxml2::XMLNode*> existing;

        auto rootElem = doc.RootElement();
        if(!rootElem)
        {
            // If for whatever reason we don't have a root element, create
            // one now
            rootElem = doc.NewElement("objects");
            doc.InsertEndChild(rootElem);
        }
        else
        {
            // Load all existing partials for replacement
            auto child = rootElem->FirstChild();
            while(child != 0)
            {
                auto member = child->FirstChildElement("member");
                while(member != 0)
                {
                    libcomp::String memberName(member->Attribute("name"));
                    if(memberName == "ID")
                    {
                        auto txtChild = member->FirstChild();
                        auto txt = txtChild ? txtChild->ToText() : 0;
                        if(txt)
                        {
                            existing[libcomp::String(txt->Value())
                                .ToInteger<uint32_t>()] = child;
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
        for(uint32_t partialID : filePair.second)
        {
            auto partial = mZonePartials[partialID];

            // Append to the existing file
            partial->Save(doc, *rootElem);

            tinyxml2::XMLNode* pNode = rootElem->LastChild();

            // If the partial already existed in the file, move it to the
            // same location and drop the old one
            auto iter = existing.find(partialID);
            if(iter != existing.end())
            {
                if(iter->second->NextSibling() != pNode)
                {
                    rootElem->InsertAfterChild(iter->second, pNode);
                }

                rootElem->DeleteChild(iter->second);
                existing[partialID] = pNode;
            }

            updatedNodes.push_back(pNode);
        }

        if(updatedNodes.size() > 0)
        {
            XmlHandler::SimplifyObjects(updatedNodes);
        }

        doc.SaveFile(path.C());

        LOG_DEBUG(libcomp::String("Updated zone partial file '%1'\n")
            .Arg(path));
    }
}

void ZoneWindow::ResetAppliedPartials(std::set<uint32_t> newPartials)
{
    uint32_t dynamicMapID = mMergedZone->CurrentZone->GetDynamicMapID();
    for(auto& pair : mZonePartials)
    {
        if(newPartials.size() == 0 ||
            newPartials.find(pair.first) != newPartials.end())
        {
            auto partial = pair.second;

            if(partial->GetAutoApply() && dynamicMapID &&
                partial->DynamicMapIDsContains(dynamicMapID))
            {
                // Automatically add auto applies
                mSelectedPartials.insert(partial->GetID());
            }
        }
    }

    RebuildCurrentZoneDisplay();
}

void ZoneWindow::RebuildCurrentZoneDisplay()
{
    ui.zoneView->blockSignals(true);

    ui.zoneView->clear();
    if(mSelectedPartials.size() > 0)
    {
        ui.zoneView->addItem("Merged Zone", -2);
        ui.zoneView->addItem("Zone Only", -1);

        for(uint32_t partialID : mSelectedPartials)
        {
            if(partialID)
            {
                ui.zoneView->addItem(QString("Partial %1")
                    .arg(partialID), (int32_t)partialID);
            }
            else
            {
                ui.zoneView->addItem("Global Partial", 0);
            }
        }

        ui.zoneViewWidget->show();
    }
    else
    {
        ui.zoneViewWidget->hide();
    }

    ui.zoneView->blockSignals(false);
}

void ZoneWindow::UpdateMergedZone(bool redraw)
{
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
    if(!zoneOnly)
    {
        // Build merged zone based on
        int viewing = ui.zoneView->currentData().toInt();
        switch(viewing)
        {
        case -2:
            // Copy the base zone definition and apply all partials
            {
                auto copyZone = std::make_shared<objects::ServerZone>(
                    *mMergedZone->CurrentZone);

                for(uint32_t partialID : mSelectedPartials)
                {
                    auto partial = mZonePartials[partialID];
                    libcomp::ServerDataManager::ApplyZonePartial(copyZone,
                        partial, true);
                }

                mMergedZone->Definition = copyZone;

                // Show the zone details but do not enable editing
                ui.zoneHeaderWidget->show();

                ui.lblZoneViewNotes->setText("No zone or zone partial fields"
                    " can be modified while viewing a merged zone.");

                canEdit = false;
            }
            break;
        case -1:
            // Merge no partials
            zoneOnly = true;
            break;
        default:
            // Build zone just from selected partial
            if(viewing >= 0)
            {
                auto newZone = std::make_shared<objects::ServerZone>();
                newZone->SetID(mMergedZone->CurrentZone->GetID());
                newZone->SetDynamicMapID(mMergedZone->CurrentZone
                    ->GetDynamicMapID());

                auto partial = mZonePartials[(uint32_t)viewing];
                libcomp::ServerDataManager::ApplyZonePartial(newZone,
                    partial, false);

                mMergedZone->Definition = newZone;
                mMergedZone->CurrentPartial = partial;

                // Show the partial controls
                ui.grpPartial->show();
                ui.partialID->setValue((int)partial->GetID());

                ui.partialAutoApply->setChecked(partial->GetAutoApply());

                ui.partialDynamicMapIDs->Clear();
                for(uint32_t dynamicMapID : partial->GetDynamicMapIDs())
                {
                    ui.partialDynamicMapIDs->AddUnsignedInteger(dynamicMapID);
                }

                ui.grpBonuses->setDisabled(false);
                ui.grpSkills->setDisabled(false);
                ui.grpTriggers->setDisabled(false);

                ui.lblZoneViewNotes->setText("Changes made while viewing a"
                    " zone partial will not be applied directly to the zone.");
            }
            break;
        }
    }

    if(zoneOnly)
    {
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
    ui.spawns->SetReadOnly(!canEdit);
    ui.spawnGroups->SetReadOnly(!canEdit);
    ui.spawnLocationGroups->SetReadOnly(!canEdit);
    ui.spots->SetReadOnly(!canEdit);

    ui.addNPC->setDisabled(!canEdit);
    ui.addObject->setDisabled(!canEdit);
    ui.addSpawn->setDisabled(!canEdit);
    ui.removeNPC->setDisabled(!canEdit);
    ui.removeObject->setDisabled(!canEdit);
    ui.removeSpawn->setDisabled(!canEdit);

    // Update merged collection properties
    ui.dropSetIDs->Clear();
    for(uint32_t dropSetID : mMergedZone->Definition->GetDropSetIDs())
    {
        ui.dropSetIDs->AddUnsignedInteger(dropSetID);
    }

    ui.skillBlacklist->Clear();
    for(uint32_t skillID : mMergedZone->Definition->GetSkillBlacklist())
    {
        ui.skillBlacklist->AddUnsignedInteger(skillID);
    }

    ui.skillWhitelist->Clear();
    for(uint32_t skillID : mMergedZone->Definition->GetSkillWhitelist())
    {
        ui.skillWhitelist->AddUnsignedInteger(skillID);
    }

    ui.triggers->Clear();
    for(auto trigger : mMergedZone->Definition->GetTriggers())
    {
        ui.triggers->AddObject(trigger);
    }

    if(redraw)
    {
        LoadMapFromZone();
    }
}

bool ZoneWindow::LoadMapFromZone()
{
    auto zone = mMergedZone->Definition;

    auto dataset = mMainWindow->GetBinaryDataSet("ZoneData");
    mZoneData = std::dynamic_pointer_cast<objects::MiZoneData>(
        dataset->GetObjectByID(zone->GetID()));
    if(!mZoneData)
    {
        return false;
    }

    auto definitions = mMainWindow->GetDefinitions();
    mQmpFile = definitions->LoadQmpFile(mZoneData->GetFile()->GetQmpFile(),
        &*mMainWindow->GetDatastore());
    if(!mQmpFile)
    {
        return false;
    }

    BindNPCs();
    BindObjects();
    BindSpawns();
    BindSpots();

    DrawMap();

    return true;
}

void ZoneWindow::LoadProperties()
{
    if(!mMergedZone->Definition)
    {
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
    for(int8_t teamType : zone->GetValidTeamTypes())
    {
        ui.validTeamTypes->AddInteger(teamType);
    }

    ui.trackTeam->setChecked(zone->GetTrackTeam());
}

void ZoneWindow::SaveProperties()
{
    // Pull all properties into their respective parent
    ui.npcs->SaveActiveProperties();
    ui.objects->SaveActiveProperties();
    ui.spawns->SaveActiveProperties();
    ui.spawnGroups->SaveActiveProperties();
    ui.spawnLocationGroups->SaveActiveProperties();
    ui.spots->SaveActiveProperties();

    if(mMergedZone->CurrentPartial)
    {
        // Partial selected
        auto partial = mMergedZone->CurrentPartial;

        partial->SetAutoApply(ui.partialAutoApply->isChecked());

        partial->ClearDynamicMapIDs();
        for(uint32_t dynamicMapID : ui.partialDynamicMapIDs
            ->GetUnsignedIntegerList())
        {
            partial->InsertDynamicMapIDs(dynamicMapID);
        }

        partial->ClearDropSetIDs();
        for(uint32_t dropSetID : ui.dropSetIDs->GetUnsignedIntegerList())
        {
            partial->InsertDropSetIDs(dropSetID);
        }

        partial->ClearSkillBlacklist();
        for(uint32_t skillID : ui.skillBlacklist->GetUnsignedIntegerList())
        {
            partial->InsertSkillBlacklist(skillID);
        }

        partial->ClearSkillWhitelist();
        for(uint32_t skillID : ui.skillWhitelist->GetUnsignedIntegerList())
        {
            partial->InsertSkillWhitelist(skillID);
        }

        auto triggers = ui.triggers->GetObjectList<
            objects::ServerZoneTrigger>();
        partial->SetTriggers(triggers);
    }
    else if(mMergedZone->CurrentZone &&
        mMergedZone->CurrentZone == mMergedZone->Definition)
    {
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
        for(int32_t teamType : ui.validTeamTypes->GetIntegerList())
        {
            zone->InsertValidTeamTypes((int8_t)teamType);
        }

        zone->SetTrackTeam(ui.trackTeam->isChecked());

        zone->ClearDropSetIDs();
        for(uint32_t dropSetID : ui.dropSetIDs->GetUnsignedIntegerList())
        {
            zone->InsertDropSetIDs(dropSetID);
        }

        zone->ClearSkillBlacklist();
        for(uint32_t skillID : ui.skillBlacklist->GetUnsignedIntegerList())
        {
            zone->InsertSkillBlacklist(skillID);
        }

        zone->ClearSkillWhitelist();
        for(uint32_t skillID : ui.skillWhitelist->GetUnsignedIntegerList())
        {
            zone->InsertSkillWhitelist(skillID);
        }

        auto triggers = ui.triggers->GetObjectList<
            objects::ServerZoneTrigger>();
        zone->SetTriggers(triggers);
    }
}

bool ZoneWindow::GetSpotPosition(uint32_t dynamicMapID, uint32_t spotID,
    float& x, float& y, float& rot) const
{
    if(spotID == 0 || dynamicMapID == 0)
    {
        return false;
    }

    auto definitions = mMainWindow->GetDefinitions();

    auto spots = definitions->GetSpotData(dynamicMapID);
    auto spotIter = spots.find(spotID);
    if(spotIter != spots.end())
    {
        x = spotIter->second->GetCenterX();
        y = spotIter->second->GetCenterY();
        rot = spotIter->second->GetRotation();

        return true;
    }

    return false;
}

void ZoneWindow::BindNPCs()
{
    std::vector<std::shared_ptr<libcomp::Object>> npcs;
    for(auto npc : mMergedZone->Definition->GetNPCs())
    {
        npcs.push_back(npc);
    }

    ui.npcs->SetObjectList(npcs);
}

void ZoneWindow::BindObjects()
{
    std::vector<std::shared_ptr<libcomp::Object>> objs;
    for(auto obj : mMergedZone->Definition->GetObjects())
    {
        objs.push_back(obj);
    }

    ui.objects->SetObjectList(objs);
}

void ZoneWindow::BindSpawns()
{
    // Sort by key
    std::map<uint32_t, std::shared_ptr<libcomp::Object>> spawnSort;
    std::map<uint32_t, std::shared_ptr<libcomp::Object>> sgSort;
    std::map<uint32_t, std::shared_ptr<libcomp::Object>> slgSort;

    for(auto& sPair : mMergedZone->Definition->GetSpawns())
    {
        spawnSort[sPair.first] = sPair.second;
    }

    for(auto& sgPair : mMergedZone->Definition->GetSpawnGroups())
    {
        sgSort[sgPair.first] = sgPair.second;
    }

    for(auto& slgPair : mMergedZone->Definition->GetSpawnLocationGroups())
    {
        slgSort[slgPair.first] = slgPair.second;
    }

    std::vector<std::shared_ptr<libcomp::Object>> spawns;
    for(auto& sPair : spawnSort)
    {
        spawns.push_back(sPair.second);
    }

    std::vector<std::shared_ptr<libcomp::Object>> sgs;
    for(auto& sgPair : sgSort)
    {
        sgs.push_back(sgPair.second);
    }

    std::vector<std::shared_ptr<libcomp::Object>> slgs;
    for(auto& slgPair : slgSort)
    {
        slgs.push_back(slgPair.second);
    }

    ui.spawns->SetObjectList(spawns);
    ui.spawnGroups ->SetObjectList(sgs);
    ui.spawnLocationGroups->SetObjectList(slgs);

    // Build these in order as they are dependent
    RebuildNamedDataSet("Spawn");
    RebuildNamedDataSet("SpawnGroup");
    RebuildNamedDataSet("SpawnLocationGroup");
}

void ZoneWindow::BindSpots()
{
    auto zone = mMergedZone->Definition;

    std::vector<std::shared_ptr<libcomp::Object>> spots;

    auto definitions = mMainWindow->GetDefinitions();
    auto spotDefs = definitions->GetSpotData(zone->GetDynamicMapID());

    // Add defined spots first (valid or not)
    for(auto& spotPair : zone->GetSpots())
    {
        auto iter = spotDefs.find(spotPair.first);
        if(iter != spotDefs.end())
        {
            spots.push_back(iter->second);
        }
        else
        {
            spots.push_back(spotPair.second);
        }
    }

    // Add all remaining definitions next
    for(auto& spotPair : spotDefs)
    {
        if(!zone->SpotsKeyExists(spotPair.first))
        {
            spots.push_back(spotPair.second);
        }
    }

    ui.spots->SetObjectList(spots);
}

void ZoneWindow::DrawMap()
{
    auto zone = mMergedZone->Definition;
    if(!mZoneData || !zone)
    {
        return;
    }

    auto xScroll = ui.mapScrollArea->horizontalScrollBar()->value();
    auto yScroll = ui.mapScrollArea->verticalScrollBar()->value();

    mDrawTarget = new QLabel();

    QPicture pic;
    QPainter painter(&pic);

    // Draw geometry
    std::unordered_map<uint32_t, uint8_t> elems;
    for(auto elem : mQmpFile->GetElements())
    {
        elems[elem->GetID()] = (uint8_t)elem->GetType();
    }

    for(auto boundary : mQmpFile->GetBoundaries())
    {
        for(auto line : boundary->GetLines())
        {
            switch(elems[line->GetElementID()])
            {
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

    std::set<std::shared_ptr<libcomp::Object>> highlight;
    switch(ui.tabs->currentIndex())
    {
    case 1: // NPCs
        {
            auto npc = ui.npcs->GetActiveObject();
            if(npc)
            {
                highlight.insert(npc);
            }
        }
        break;
    case 2: // Objects
        {
            auto obj = ui.objects->GetActiveObject();
            if(obj)
            {
                highlight.insert(obj);
            }
        }
        break;
    case 3: // Spawn Types
        {
            // If a SpawnLocationGroup is selected, highlight all bound
            // spots
            if(ui.tabSpawnTypes->currentIndex() == 2)
            {
                auto slg = std::dynamic_pointer_cast<
                    objects::SpawnLocationGroup>(ui.spawnLocationGroups
                        ->GetActiveObject());
                if(slg)
                {
                    for(uint32_t spotID : slg->GetSpotIDs())
                    {
                        auto spotIter = spots.find(spotID);
                        if(spotIter != spots.end())
                        {
                            highlight.insert(spotIter->second);
                        }
                    }
                }
            }
        }
        break;
    case 4: // Spots
        {
            auto spot = ui.spots->GetActiveObject();
            if(spot)
            {
                highlight.insert(spot);
            }
        }
        break;
    case 0: // Zone
    default:
        break;
    }

    // Draw spots
    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);

    for(auto spotPair : spots)
    {
        if(highlight.find(spotPair.second) == highlight.end())
        {
            DrawSpot(spotPair.second, false, painter);
        }
    }

    // Draw the starting point
    painter.setPen(QPen(COLOR_PLAYER));
    painter.setBrush(QBrush(COLOR_PLAYER));

    painter.drawEllipse(QPoint(Scale(mMergedZone->CurrentZone->GetStartingX()),
        Scale(-mMergedZone->CurrentZone->GetStartingY())), 3, 3);

    // Draw NPCs
    if(ui.showNPCs->isChecked())
    {
        for(auto npc : zone->GetNPCs())
        {
            if(highlight.find(npc) == highlight.end())
            {
                DrawNPC(npc, false, painter);
            }
        }
    }

    // Draw Objects
    if(ui.showObjects->isChecked())
    {
        for(auto obj : zone->GetObjects())
        {
            if(highlight.find(obj) == highlight.end())
            {
                DrawObject(obj, false, painter);
            }
        }
    }

    // Draw selected object on top of the others
    for(auto h : highlight)
    {
        auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(h);
        auto obj = std::dynamic_pointer_cast<objects::ServerObject>(h);
        auto spot = std::dynamic_pointer_cast<objects::MiSpotData>(h);
        if(npc)
        {
            DrawNPC(npc, true, painter);
        }
        else if(obj)
        {
            DrawObject(obj, true, painter);
        }
        else if(spot)
        {
            DrawSpot(spot, true, painter);
        }
    }

    painter.end();

    mDrawTarget->setPicture(pic);
    ui.mapScrollArea->setWidget(mDrawTarget);

    ui.mapScrollArea->horizontalScrollBar()->setValue(xScroll);
    ui.mapScrollArea->verticalScrollBar()->setValue(yScroll);
}

void ZoneWindow::DrawNPC(const std::shared_ptr<objects::ServerNPC>& npc,
    bool selected, QPainter& painter)
{
    float x = npc->GetX();
    float y = npc->GetY();
    float rot = npc->GetRotation();
    GetSpotPosition(mMergedZone->Definition->GetDynamicMapID(),
        npc->GetSpotID(), x, y, rot);

    if(selected)
    {
        painter.setPen(QPen(COLOR_SELECTED));
        painter.setBrush(QBrush(COLOR_SELECTED));
    }
    else
    {
        painter.setPen(QPen(COLOR_NPC));
        painter.setBrush(QBrush(COLOR_NPC));
    }

    painter.drawEllipse(QPoint(Scale(x), Scale(-y)), 3, 3);

    painter.drawText(QPoint(Scale(x + 20.f), Scale(-y)),
        libcomp::String("%1").Arg(npc->GetID()).C());
}

void ZoneWindow::DrawObject(const std::shared_ptr<objects::ServerObject>& obj,
    bool selected, QPainter& painter)
{   
    float x = obj->GetX();
    float y = obj->GetY();
    float rot = obj->GetRotation();
    GetSpotPosition(mMergedZone->Definition->GetDynamicMapID(),
        obj->GetSpotID(), x, y, rot);

    if(selected)
    {
        painter.setPen(QPen(COLOR_SELECTED));
        painter.setBrush(QBrush(COLOR_SELECTED));
    }
    else
    {
        painter.setPen(QPen(COLOR_OBJECT));
        painter.setBrush(QBrush(COLOR_OBJECT));
    }

    painter.drawEllipse(QPoint(Scale(x), Scale(-y)), 3, 3);

    painter.drawText(QPoint(Scale(x + 20.f), Scale(-y)),
        libcomp::String("%1").Arg(obj->GetID()).C());
}

void ZoneWindow::DrawSpot(const std::shared_ptr<objects::MiSpotData>& spotDef,
    bool selected, QPainter& painter)
{
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

    for(auto& p : points)
    {
        float x = p.first;
        float y = p.second;
        p.first = (float)(((x - xc) * cos(rot)) -
            ((y - yc) * sin(rot)) + xc);
        p.second = (float)(((x - xc) * sin(rot)) +
            ((y - yc) * cos(rot)) + yc);
    }

    if(selected)
    {
        painter.setPen(QPen(COLOR_SELECTED));
        painter.setBrush(QBrush(COLOR_SELECTED));
    }
    else
    {
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

    painter.drawText(QPoint(Scale(x1), Scale(y2)),
        libcomp::String("[%1] %2").Arg(spotDef->GetType())
        .Arg(spotDef->GetID()).C());
}

int32_t ZoneWindow::Scale(int32_t point)
{
    return (int32_t)(point / ui.zoomSlider->value());
}

int32_t ZoneWindow::Scale(float point)
{
    return (int32_t)(point / (float)ui.zoomSlider->value());
}
