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
#include <ServerZone.h>
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

    connect(ui.actionLoad, SIGNAL(triggered()), this, SLOT(LoadZoneFile()));

    connect(ui.actionPartialsLoadFile, SIGNAL(triggered()), this,
        SLOT(LoadPartialFile()));
    connect(ui.actionPartialsLoadDirectory, SIGNAL(triggered()),
        this, SLOT(LoadPartialDirectory()));
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

        std::vector<std::shared_ptr<libcomp::Object>> spawns;
        for(auto& sPair : mMergedZone->Definition->GetSpawns())
        {
            auto spawn = sPair.second;
            auto devilData = std::dynamic_pointer_cast<objects::MiDevilData>(
                devilDataSet->GetObjectByID(spawn->GetEnemyType()));

            libcomp::String name(devilData
                ? devilDataSet->GetName(devilData) : "[Unknown]");

            int8_t lvl = spawn->GetLevel();
            if(lvl == -1 && devilData)
            {
                lvl = devilData->GetGrowth()->GetBaseLevel();
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

        std::vector<std::shared_ptr<libcomp::Object>> sgs;
        for(auto& sgPair : mMergedZone->Definition->GetSpawnGroups())
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

    mMergedZone->Definition = zone;
    mMergedZone->CurrentZone = zone;
    mMergedZone->CurrentPartial = nullptr;

    mMainWindow->UpdateActiveZone(cs(path));

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
    std::vector<std::shared_ptr<libcomp::Object>> spawns;
    for(auto& sPair : mMergedZone->Definition->GetSpawns())
    {
        spawns.push_back(sPair.second);
    }

    std::vector<std::shared_ptr<libcomp::Object>> sgs;
    for(auto& sgPair : mMergedZone->Definition->GetSpawnGroups())
    {
        sgs.push_back(sgPair.second);
    }

    std::vector<std::shared_ptr<libcomp::Object>> slgs;
    for(auto& slgPair : mMergedZone->Definition->GetSpawnLocationGroups())
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
