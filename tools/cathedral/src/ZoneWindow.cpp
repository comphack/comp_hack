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
#include "MainWindow.h"
#include "ZonePartialSelector.h"

// Qt Includes
#include <PushIgnore.h>
#include <QDirIterator>
#include <QFileDialog>
#include <QIODevice>
#include <QMessageBox>
#include <QPainter>
#include <QPicture>
#include <QScrollBar>
#include <QSettings>
#include <QToolTip>
#include <PopIgnore.h>

// object Includes
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
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>

// C++11 Standard Includes
#include <cmath>

// libcomp Includes
#include <Log.h>
#include <ServerDataManager.h>

ZoneWindow::ZoneWindow(MainWindow *pMainWindow, QWidget *p)
    : QMainWindow(p), mMainWindow(pMainWindow), mDrawTarget(0)
{
    ui.setupUi(this);

    mMergedZone = std::make_shared<MergedZone>();

    ui.npcs->Bind(pMainWindow, true);
    ui.objects->Bind(pMainWindow, false);
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

    connect(ui.zoom200, SIGNAL(triggered()),
        this, SLOT(Zoom200()));
    connect(ui.zoom100, SIGNAL(triggered()),
        this, SLOT(Zoom100()));
    connect(ui.zoom50, SIGNAL(triggered()),
        this, SLOT(Zoom50()));
    connect(ui.zoom25, SIGNAL(triggered()),
        this, SLOT(Zoom25()));
    connect(ui.actionRefresh, SIGNAL(triggered()),
        this, SLOT(Refresh()));
    connect(ui.showNPCs, SIGNAL(toggled(bool)),
        this, SLOT(ShowToggled(bool)));
    connect(ui.showObjects, SIGNAL(toggled(bool)),
        this, SLOT(ShowToggled(bool)));
    connect(ui.showSpawns, SIGNAL(toggled(bool)),
        this, SLOT(ShowToggled(bool)));

    connect(ui.actionPartialsLoadFile, SIGNAL(triggered()), this,
        SLOT(LoadFile()));
    connect(ui.actionPartialsLoadDirectory, SIGNAL(triggered()),
        this, SLOT(LoadDirectory()));
    connect(ui.actionPartialsApply, SIGNAL(triggered()),
        this, SLOT(ApplyPartials()));

    connect(ui.zoneView, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(ZoneViewUpdated()));

    mZoomScale = 20;
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

bool ZoneWindow::ShowZone(const std::shared_ptr<objects::ServerZone>& zone)
{
    if(!zone)
    {
        return false;
    }

    mMergedZone->Definition = zone;
    mMergedZone->CurrentZone = zone;
    mMergedZone->CurrentPartial = nullptr;

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

void ZoneWindow::LoadDirectory()
{
    QSettings settings;

    QString qPath = QFileDialog::getExistingDirectory(this,
        tr("Load Zone Partial XML folder"),
        settings.value("datastore").toString());
    if(qPath.isEmpty())
    {
        return;
    }

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

void ZoneWindow::LoadFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getOpenFileName(this,
        tr("Load Zone Partial XML"), settings.value("datastore").toString(),
        tr("Zone Partial XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

    libcomp::String path = cs(qPath);
    if(LoadZonePartials(path))
    {
        UpdateMergedZone(true);
    }
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

void ZoneWindow::ZoneViewUpdated()
{
    UpdateMergedZone(true);
}

void ZoneWindow::Zoom200()
{
    mZoomScale = 10;
    ui.zoom200->setChecked(true);

    ui.zoom100->setChecked(false);
    ui.zoom50->setChecked(false);
    ui.zoom25->setChecked(false);

    DrawMap();
}

void ZoneWindow::Zoom100()
{
    mZoomScale = 20;
    ui.zoom100->setChecked(true);

    ui.zoom200->setChecked(false);
    ui.zoom50->setChecked(false);
    ui.zoom25->setChecked(false);

    DrawMap();
}

void ZoneWindow::Zoom50()
{
    mZoomScale = 40;
    ui.zoom50->setChecked(true);

    ui.zoom200->setChecked(false);
    ui.zoom100->setChecked(false);
    ui.zoom25->setChecked(false);

    DrawMap();
}

void ZoneWindow::Zoom25()
{
    mZoomScale = 80;
    ui.zoom25->setChecked(true);

    ui.zoom200->setChecked(false);
    ui.zoom100->setChecked(false);
    ui.zoom50->setChecked(false);

    DrawMap();
}

void ZoneWindow::ShowToggled(bool checked)
{
    (void)checked;

    DrawMap();
}

void ZoneWindow::Refresh()
{
    DrawMap();
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
    ui.grpTriggers->setDisabled(true);

    ui.grpPartial->hide();
    ui.partialAutoApply->setChecked(false);
    ui.partialDynamicMapIDs->Clear();

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
                        partial);
                }

                mMergedZone->Definition = copyZone;

                // Show the zone details but do not enable editing
                ui.zoneHeaderWidget->show();

                ui.lblZoneViewNotes->setText("No zone or zone partial fields"
                    " can be modified while viewing a merged zone.");
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
                    partial);

                mMergedZone->Definition = newZone;

                // Show the partial controls
                ui.grpPartial->show();
                ui.partialID->setValue((int)partial->GetID());

                ui.partialAutoApply->setChecked(partial->GetAutoApply());

                ui.partialDynamicMapIDs->Clear();
                for(uint32_t dynamicMapID : partial->GetDynamicMapIDs())
                {
                    ui.partialDynamicMapIDs->AddUnsignedInteger(dynamicMapID);
                }

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
        ui.grpTriggers->setDisabled(false);
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

    ui.comboBox_SpawnEdit->clear();
    ui.comboBox_SpawnEdit->addItem("All");
    for(auto pair : zone->GetSpawnLocationGroups())
    {
        ui.comboBox_SpawnEdit->addItem(libcomp::String("%1")
            .Arg(pair.first).C());
    }

    // Convert spot IDs
    for(auto npc : zone->GetNPCs())
    {
        if(npc->GetSpotID())
        {
            float x = npc->GetX();
            float y = npc->GetY();
            float rot = npc->GetRotation();
            if(GetSpotPosition(zone->GetDynamicMapID(), npc->GetSpotID(),
                x, y, rot))
            {
                npc->SetX(x);
                npc->SetY(y);
                npc->SetRotation(rot);
            }
        }
    }

    for(auto obj : zone->GetObjects())
    {
        if(obj->GetSpotID())
        {
            float x = obj->GetX();
            float y = obj->GetY();
            float rot = obj->GetRotation();
            if(GetSpotPosition(zone->GetDynamicMapID(), obj->GetSpotID(),
                x, y, rot))
            {
                obj->SetX(x);
                obj->SetY(y);
                obj->SetRotation(rot);
            }
        }
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
    ui.groupID->setValue((int32_t)zone->GetDynamicMapID());
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

    ui.dropSetIDs->Clear();
    for(uint32_t dropSetID : zone->GetDropSetIDs())
    {
        ui.dropSetIDs->AddUnsignedInteger(dropSetID);
    }

    ui.skillBlacklist->Clear();
    for(uint32_t skillID : zone->GetSkillBlacklist())
    {
        ui.skillBlacklist->AddUnsignedInteger(skillID);
    }

    ui.skillWhitelist->Clear();
    for(uint32_t skillID : zone->GetSkillWhitelist())
    {
        ui.skillWhitelist->AddUnsignedInteger(skillID);
    }

    ui.triggers->Clear();
    for(auto trigger : zone->GetTriggers())
    {
        ui.triggers->AddObject(trigger);
    }
}

bool ZoneWindow::GetSpotPosition(uint32_t dynamicMapID, uint32_t spotID,
    float& x, float& y, float& rot) const
{
    if (spotID == 0 || dynamicMapID == 0)
    {
        return false;
    }

    auto definitions = mMainWindow->GetDefinitions();

    auto spots = definitions->GetSpotData(dynamicMapID);
    auto spotIter = spots.find(spotID);
    if (spotIter != spots.end())
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
    auto zone = mMergedZone->Definition;

    auto dataset = mMainWindow->GetBinaryDataSet("DevilData");

    // Set up the Spawn table
    ui.tableWidget_Spawn->clear();
    ui.tableWidget_Spawn->setColumnCount(5);
    ui.tableWidget_Spawn->setHorizontalHeaderItem(0,
        GetTableWidget("ID"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(1,
        GetTableWidget("Type"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(2,
        GetTableWidget("Variant"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(3,
        GetTableWidget("Name"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(4,
        GetTableWidget("Level"));

    ui.tableWidget_Spawn->setRowCount((int)zone->SpawnsCount());
    int i = 0;
    for(auto sPair : zone->GetSpawns())
    {
        auto s = sPair.second;
        auto def = std::dynamic_pointer_cast<objects::MiDevilData>(
            dataset->GetObjectByID(s->GetEnemyType()));

        ui.tableWidget_Spawn->setItem(i, 0, GetTableWidget(
            libcomp::String("%1").Arg(s->GetID()).C()));
        ui.tableWidget_Spawn->setItem(i, 1, GetTableWidget(
            libcomp::String("%1").Arg(s->GetEnemyType()).C()));
        ui.tableWidget_Spawn->setItem(i, 2, GetTableWidget(
            libcomp::String("%1").Arg(s->GetVariantType()).C()));
        ui.tableWidget_Spawn->setItem(i, 3, GetTableWidget(
            libcomp::String("%1").Arg(def
                ? def->GetBasic()->GetName() : "?").C()));
        ui.tableWidget_Spawn->setItem(i, 4, GetTableWidget(
            libcomp::String("%1").Arg(def
                ? def->GetGrowth()->GetBaseLevel() : 0).C()));
        i++;
    }

    ui.tableWidget_Spawn->resizeColumnsToContents();

    // Set up the Spawn Group table
    ui.tableWidget_SpawnGroup->clear();
    ui.tableWidget_SpawnGroup->setColumnCount(3);
    ui.tableWidget_SpawnGroup->setHorizontalHeaderItem(0,
        GetTableWidget("GroupID"));
    ui.tableWidget_SpawnGroup->setHorizontalHeaderItem(1,
        GetTableWidget("SpawnID"));
    ui.tableWidget_SpawnGroup->setHorizontalHeaderItem(3,
        GetTableWidget("Count"));

    ui.tableWidget_SpawnGroup->setRowCount((int)zone->SpawnGroupsCount());
    i = 0;
    for(auto sgPair : zone->GetSpawnGroups())
    {
        auto sg = sgPair.second;
        for(auto sPair : sg->GetSpawns())
        {
            ui.tableWidget_SpawnGroup->setItem(i, 0, GetTableWidget(
                libcomp::String("%1").Arg(sg->GetID()).C()));
            ui.tableWidget_SpawnGroup->setItem(i, 1, GetTableWidget(
                libcomp::String("%1").Arg(sPair.first).C()));
            ui.tableWidget_SpawnGroup->setItem(i, 3, GetTableWidget(
                libcomp::String("%1").Arg(sPair.second).C()));
            i++;
        }
    }

    ui.tableWidget_SpawnGroup->resizeColumnsToContents();

    // Set up the Spawn Location table
    ui.tableWidget_SpawnLocation->clear();
    ui.tableWidget_SpawnLocation->setColumnCount(6);
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(0,
        GetTableWidget("LGroupID"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(1,
        GetTableWidget("X"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(2,
        GetTableWidget("Y"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(3,
        GetTableWidget("Width"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(4,
        GetTableWidget("Height"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(5,
        GetTableWidget("RespawnTime"));

    uint32_t locKey = static_cast<uint32_t>(-1);
    QString selectedLGroup = ui.comboBox_SpawnEdit->currentText();
    bool allLocs = selectedLGroup == "All";
    if(selectedLGroup.length() > 0 && !allLocs)
    {
        bool success = false;
        locKey = libcomp::String(selectedLGroup.toStdString())
            .ToInteger<uint32_t>(&success);
    }

    auto locGroup = zone->GetSpawnLocationGroups(locKey);

    int locCount = 0;
    for(auto grpPair : zone->GetSpawnLocationGroups())
    {
        if(allLocs || grpPair.second == locGroup)
        {
            locCount += (int)grpPair.second->LocationsCount();
        }
    }

    ui.tableWidget_SpawnLocation->setRowCount(locCount);
    i = 0;
    for(auto grpPair : zone->GetSpawnLocationGroups())
    {
        auto grp = grpPair.second;
        if(allLocs || grp == locGroup)
        {
            for(auto loc : grp->GetLocations())
            {
                ui.tableWidget_SpawnLocation->setItem(i, 0, GetTableWidget(
                    libcomp::String("%1").Arg(grp->GetID()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 1, GetTableWidget(
                    libcomp::String("%1").Arg(loc->GetX()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 2, GetTableWidget(
                    libcomp::String("%1").Arg(loc->GetY()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 3, GetTableWidget(
                    libcomp::String("%1").Arg(loc->GetWidth()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 4, GetTableWidget(
                    libcomp::String("%1").Arg(loc->GetHeight()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 5, GetTableWidget(
                    libcomp::String("%1").Arg(grp->GetRespawnTime()).C()));
                i++;
            }
        }
    }

    ui.tableWidget_SpawnLocation->resizeColumnsToContents();
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

QTableWidgetItem* ZoneWindow::GetTableWidget(std::string name, bool readOnly)
{
    auto item = new QTableWidgetItem(name.c_str());

    if(readOnly)
    {
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
    }

    return item;
}

void ZoneWindow::DrawMap()
{
    auto zone = mMergedZone->Definition;
    if(!mZoneData || !zone)
    {
        return;
    }

    auto xScroll = ui.scrollArea->horizontalScrollBar()->value();
    auto yScroll = ui.scrollArea->verticalScrollBar()->value();

    mDrawTarget = new QLabel();

    QPicture pic;
    QPainter painter(&pic);

    // Draw geometry
    std::unordered_map<uint32_t, uint8_t> elems;
    for(auto elem : mQmpFile->GetElements())
    {
        elems[elem->GetID()] = (uint8_t)elem->GetType();
    }

    std::set<float> xVals;
    std::set<float> yVals;
    for(auto boundary : mQmpFile->GetBoundaries())
    {
        for(auto line : boundary->GetLines())
        {
            switch(elems[line->GetElementID()])
            {
            case 1:
                // One way
                painter.setPen(QPen(Qt::blue));
                painter.setBrush(QBrush(Qt::blue));
                break;
            case 2:
                // Toggleable
                painter.setPen(QPen(Qt::green));
                painter.setBrush(QBrush(Qt::green));
                break;
            case 3:
                // Toggleable (wired up to close?)
                painter.setPen(QPen(Qt::red));
                painter.setBrush(QBrush(Qt::red));
                break;
            default:
                painter.setPen(QPen(Qt::black));
                painter.setBrush(QBrush(Qt::black));
                break;
            }
            xVals.insert((float)line->GetX1());
            xVals.insert((float)line->GetX2());
            yVals.insert((float)line->GetY1());
            yVals.insert((float)line->GetY2());

            painter.drawLine(Scale(line->GetX1()), Scale(-line->GetY1()),
                Scale(line->GetX2()), Scale(-line->GetY2()));
        }
    }

    // Draw spots
    painter.setPen(QPen(Qt::darkGreen));
    painter.setBrush(QBrush(Qt::darkGreen));

    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);

    auto definitions = mMainWindow->GetDefinitions();

    auto spots = definitions->GetSpotData(zone->GetDynamicMapID());
    for(auto spotPair : spots)
    {
        float xc = spotPair.second->GetCenterX();
        float yc = -spotPair.second->GetCenterY();
        float rot = -spotPair.second->GetRotation();

        float x1 = xc - spotPair.second->GetSpanX();
        float y1 = yc + spotPair.second->GetSpanY();

        float x2 = xc + spotPair.second->GetSpanX();
        float y2 = yc - spotPair.second->GetSpanY();

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

        painter.drawLine(Scale(points[0].first), Scale(points[0].second),
            Scale(points[1].first), Scale(points[1].second));
        painter.drawLine(Scale(points[1].first), Scale(points[1].second),
            Scale(points[2].first), Scale(points[2].second));
        painter.drawLine(Scale(points[2].first), Scale(points[2].second),
            Scale(points[3].first), Scale(points[3].second));
        painter.drawLine(Scale(points[3].first), Scale(points[3].second),
            Scale(points[0].first), Scale(points[0].second));

        painter.drawText(QPoint(Scale(x1), Scale(y2)),
            libcomp::String("[%1] %2").Arg(spotPair.second->GetType())
            .Arg(spotPair.first).C());
    }

    // Draw the starting point
    painter.setPen(QPen(Qt::magenta));
    painter.setBrush(QBrush(Qt::magenta));

    xVals.insert(zone->GetStartingX());
    yVals.insert(zone->GetStartingY());

    painter.drawEllipse(QPoint(Scale(zone->GetStartingX()),
        Scale(-zone->GetStartingY())), 3, 3);

    // Draw NPCs
    if(ui.showNPCs->isChecked())
    {
        painter.setPen(QPen(Qt::green));
        painter.setBrush(QBrush(Qt::green));

        for(auto npc : zone->GetNPCs())
        {
            xVals.insert(npc->GetX());
            yVals.insert(npc->GetY());
            painter.drawEllipse(QPoint(Scale(npc->GetX()),
                Scale(-npc->GetY())), 3, 3);

            painter.drawText(QPoint(Scale(npc->GetX() + 20.f),
                Scale(-npc->GetY())), libcomp::String("%1")
                .Arg(npc->GetID()).C());
        }
    }

    // Draw Objects
    if(ui.showObjects->isChecked())
    {
        painter.setPen(QPen(Qt::blue));
        painter.setBrush(QBrush(Qt::blue));

        for(auto obj : zone->GetObjects())
        {
            xVals.insert(obj->GetX());
            yVals.insert(obj->GetY());
            painter.drawEllipse(QPoint(Scale(obj->GetX()),
                Scale(-obj->GetY())), 3, 3);

            painter.drawText(QPoint(Scale(obj->GetX() + 20.f),
                Scale(-obj->GetY())), libcomp::String("%1")
                .Arg(obj->GetID()).C());
        }
    }

    // Draw Spawn Locations
    if(ui.showSpawns->isChecked())
    {
        painter.setPen(QPen(Qt::red));
        painter.setBrush(QBrush(Qt::red));

        uint32_t locKey = static_cast<uint32_t>(-1);
        QString selectedLGroup = ui.comboBox_SpawnEdit->currentText();
        bool allLocs = selectedLGroup == "All";
        if(selectedLGroup.length() > 0 && !allLocs)
        {
            bool success = false;
            locKey = libcomp::String(selectedLGroup.toStdString())
                .ToInteger<uint32_t>(&success);
        }

        auto locGroup = zone->GetSpawnLocationGroups(locKey);

        for(auto grpPair : zone->GetSpawnLocationGroups())
        {
            auto grp = grpPair.second;
            if(allLocs || locGroup == grp)
            {
                for(auto loc : grp->GetLocations())
                {
                    float x1 = loc->GetX();
                    float y1 = loc->GetY();
                    float x2 = loc->GetX() + loc->GetWidth();
                    float y2 = loc->GetY() - loc->GetHeight();

                    xVals.insert(x1);
                    xVals.insert(x2);
                    yVals.insert(y1);
                    yVals.insert(y2);

                    painter.drawLine(Scale(x1), Scale(-y1),
                        Scale(x2), Scale(-y1));
                    painter.drawLine(Scale(x2), Scale(-y1),
                        Scale(x2), Scale(-y2));
                    painter.drawLine(Scale(x2), Scale(-y2),
                        Scale(x1), Scale(-y2));
                    painter.drawLine(Scale(x1), Scale(-y2),
                        Scale(x1), Scale(-y1));
                }
            }
        }
    }

    painter.end();

    mDrawTarget->setPicture(pic);
    ui.scrollArea->setWidget(mDrawTarget);

    ui.scrollArea->horizontalScrollBar()->setValue(xScroll);
    ui.scrollArea->verticalScrollBar()->setValue(yScroll);
}

int32_t ZoneWindow::Scale(int32_t point)
{
    return (int32_t)(point / mZoomScale);
}

int32_t ZoneWindow::Scale(float point)
{
    return (int32_t)(point / mZoomScale);
}
