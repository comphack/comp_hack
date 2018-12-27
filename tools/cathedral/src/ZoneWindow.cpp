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

// Qt Includes
#include <PushIgnore.h>
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
#include <ServerZoneSpot.h>

// C++11 Standard Includes
#include <cmath>

ZoneWindow::ZoneWindow(MainWindow *pMainWindow, QWidget *p)
    : QMainWindow(p), mMainWindow(pMainWindow), mDrawTarget(0)
{
    ui.setupUi(this);

    ui.npcs->Bind(pMainWindow, true);
    ui.objects->Bind(pMainWindow, false);
    ui.spots->SetMainWindow(pMainWindow);

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

    mZoomScale = 20;
}

ZoneWindow::~ZoneWindow()
{
}

std::shared_ptr<objects::ServerZone> ZoneWindow::GetZone() const
{
    return mZone;
}

bool ZoneWindow::ShowZone(const std::shared_ptr<objects::ServerZone>& zone)
{
    if(!zone)
    {
        return false;
    }

    mZone = zone;

    if(LoadMapFromZone())
    {
        show();
        return true;
    }

    return false;
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

bool ZoneWindow::LoadMapFromZone()
{
    auto dataset = mMainWindow->GetBinaryDataSet("ZoneData");
    mZoneData = std::dynamic_pointer_cast<objects::MiZoneData>(
        dataset->GetObjectByID(mZone->GetID()));
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

    setWindowTitle(libcomp::String("COMP_hack Cathedral of Content - Zone %1"
        " (%2)").Arg(mZone->GetID()).Arg(mZone->GetDynamicMapID()).C());

    ui.comboBox_SpawnEdit->clear();
    ui.comboBox_SpawnEdit->addItem("All");
    for(auto pair : mZone->GetSpawnLocationGroups())
    {
        ui.comboBox_SpawnEdit->addItem(libcomp::String("%1")
            .Arg(pair.first).C());
    }

    // Convert spot IDs
    for(auto npc : mZone->GetNPCs())
    {
        if(npc->GetSpotID())
        {
            float x = npc->GetX();
            float y = npc->GetY();
            float rot = npc->GetRotation();
            if(GetSpotPosition(mZone->GetDynamicMapID(), npc->GetSpotID(),
                x, y, rot))
            {
                npc->SetX(x);
                npc->SetY(y);
                npc->SetRotation(rot);
            }
        }
    }

    for(auto obj : mZone->GetObjects())
    {
        if(obj->GetSpotID())
        {
            float x = obj->GetX();
            float y = obj->GetY();
            float rot = obj->GetRotation();
            if(GetSpotPosition(mZone->GetDynamicMapID(), obj->GetSpotID(),
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
    for(auto npc : mZone->GetNPCs())
    {
        npcs.push_back(npc);
    }

    ui.npcs->SetObjectList(npcs);
}

void ZoneWindow::BindObjects()
{
    std::vector<std::shared_ptr<libcomp::Object>> objs;
    for(auto obj : mZone->GetObjects())
    {
        objs.push_back(obj);
    }

    ui.objects->SetObjectList(objs);
}

void ZoneWindow::BindSpawns()
{
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

    ui.tableWidget_Spawn->setRowCount((int)mZone->SpawnsCount());
    int i = 0;
    for(auto sPair : mZone->GetSpawns())
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

    ui.tableWidget_SpawnGroup->setRowCount((int)mZone->SpawnGroupsCount());
    i = 0;
    for(auto sgPair : mZone->GetSpawnGroups())
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

    auto locGroup = mZone->GetSpawnLocationGroups(locKey);

    int locCount = 0;
    for(auto grpPair : mZone->GetSpawnLocationGroups())
    {
        if(allLocs || grpPair.second == locGroup)
        {
            locCount += (int)grpPair.second->LocationsCount();
        }
    }

    ui.tableWidget_SpawnLocation->setRowCount(locCount);
    i = 0;
    for(auto grpPair : mZone->GetSpawnLocationGroups())
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
    std::vector<std::shared_ptr<libcomp::Object>> spots;

    auto definitions = mMainWindow->GetDefinitions();
    auto spotDefs = definitions->GetSpotData(mZone->GetDynamicMapID());

    // Add defined spots first (valid or not)
    for(auto& spotPair : mZone->GetSpots())
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
        if(!mZone->SpotsKeyExists(spotPair.first))
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
    if(!mZoneData)
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

    auto spots = definitions->GetSpotData(mZone->GetDynamicMapID());
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

    xVals.insert(mZone->GetStartingX());
    yVals.insert(mZone->GetStartingY());

    painter.drawEllipse(QPoint(Scale(mZone->GetStartingX()),
        Scale(-mZone->GetStartingY())), 3, 3);

    // Draw NPCs
    if(ui.showNPCs->isChecked())
    {
        painter.setPen(QPen(Qt::green));
        painter.setBrush(QBrush(Qt::green));

        for(auto npc : mZone->GetNPCs())
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

        for(auto obj : mZone->GetObjects())
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

        auto locGroup = mZone->GetSpawnLocationGroups(locKey);

        for(auto grpPair : mZone->GetSpawnLocationGroups())
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
