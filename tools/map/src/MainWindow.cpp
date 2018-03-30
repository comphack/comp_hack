/**
 * @file tools/map/src/MainWindow.cpp
 * @ingroup map
 *
 * @author HACKfrost
 *
 * @brief Main window for the map manager which allows for visualization
 *  and modification of zone map data.
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

#include "MainWindow.h"

 // C++ Standard Includes
#include <cmath>

#include <QFileDialog>
#include <QIODevice>
#include <QMessageBox>
#include <QPainter>
#include <QPicture>
#include <QScrollBar>
#include <QSettings>
#include <QToolTip>

#include <MiDevilData.h>
#include <MiGrowthData.h>
#include <MiNPCBasicData.h>
#include <MiSpotData.h>
#include <MiZoneFileData.h>
#include <QmpBoundary.h>
#include <QmpBoundaryLine.h>
#include <QmpElement.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocation.h>
#include <SpawnLocationGroup.h>

MainWindow::MainWindow(std::shared_ptr<libcomp::DataStore> datastore,
    std::shared_ptr<libcomp::DefinitionManager> definitions, QWidget *p)
    : QMainWindow(p), mDrawTarget(0), mRubberBand(0),
    mDatastore(datastore), mDefinitions(definitions)
{
    ui.setupUi(this);

    connect(ui.action_Open, SIGNAL(triggered()),
            this, SLOT(ShowOpenDialog()));
    connect(ui.actionSave, SIGNAL(triggered()),
        this, SLOT(ShowSaveDialog()));
    connect(ui.action_Quit, SIGNAL(triggered()),
            this, SLOT(close()));
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
    connect(ui.button_PlotPoints, SIGNAL(released()),
        this, SLOT(PlotPoints()));
    connect(ui.button_ClearPoints, SIGNAL(released()),
        this, SLOT(ClearPoints()));
    connect(ui.checkBox_NPC, SIGNAL(toggled(bool)),
        this, SLOT(ShowToggled(bool)));
    connect(ui.checkBox_Object, SIGNAL(toggled(bool)),
        this, SLOT(ShowToggled(bool)));
    connect(ui.checkBox_Spawn, SIGNAL(toggled(bool)),
        this, SLOT(ShowToggled(bool)));
    connect(ui.comboBox_SpawnEdit, SIGNAL(currentIndexChanged(const QString&)),
        this, SLOT(ComboBox_SpawnEdit_IndexChanged(const QString&)));
    connect(ui.actionRemove_Selected_Locations, SIGNAL(triggered()),
        this, SLOT(SpawnLocationRemoveSelected()));

    mZoomScale = 20;
}

MainWindow::~MainWindow()
{
}

void MainWindow::ShowOpenDialog()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Zone Definition"),
        QString(), tr("Zone Definition (*.xml)"));

    if(path.isEmpty())
        return;

    LoadMapFromZone(path);
}

void MainWindow::ShowSaveDialog()
{
    if(!mZoneData)
    {
        // No zone loaded, nothing to do
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Save Zone Definition"),
        QString(), tr("Zone Definition (*.xml)"));

    if(path.isEmpty())
        return;

    tinyxml2::XMLDocument doc;

    tinyxml2::XMLElement* pRoot = doc.NewElement("objects");
    doc.InsertEndChild(pRoot);

    if(mZone.Save(doc, *pRoot))
    {
        doc.SaveFile(path.toUtf8().constData());
    }
}

void MainWindow::Zoom200()
{
    mZoomScale = 10;
    ui.zoom200->setChecked(true);

    ui.zoom100->setChecked(false);
    ui.zoom50->setChecked(false);
    ui.zoom25->setChecked(false);

    DrawMap();
}

void MainWindow::Zoom100()
{
    mZoomScale = 20;
    ui.zoom100->setChecked(true);

    ui.zoom200->setChecked(false);
    ui.zoom50->setChecked(false);
    ui.zoom25->setChecked(false);

    DrawMap();
}

void MainWindow::Zoom50()
{
    mZoomScale = 40;
    ui.zoom50->setChecked(true);

    ui.zoom200->setChecked(false);
    ui.zoom100->setChecked(false);
    ui.zoom25->setChecked(false);

    DrawMap();
}

void MainWindow::Zoom25()
{
    mZoomScale = 80;
    ui.zoom25->setChecked(true);

    ui.zoom200->setChecked(false);
    ui.zoom100->setChecked(false);
    ui.zoom50->setChecked(false);

    DrawMap();
}

void MainWindow::PlotPoints()
{
    mPoints.clear();
    mHiddenPoints.clear();

    QString txt = ui.textEdit_Points->toPlainText();
    libcomp::String all(txt.toStdString());

    all = all.Replace("\r", "\n");
    for(auto splitStr : all.Split("\n"))
    {
        if(splitStr.IsEmpty()) continue;

        splitStr = splitStr.Replace("\t", ",");
        std::vector<libcomp::String> parts;
        for(auto s : splitStr.Split(","))
        {
            if(!s.IsEmpty())
            {
                parts.push_back(s);
            }
        }

        if(parts.size() >= 2)
        {
            GenericPoint p;

            bool parsed = false;

            p.X = parts[0].ToDecimal<float>(&parsed);
            if(!parsed) continue;

            p.Y = parts[1].ToDecimal<float>(&parsed);
            if(!parsed) continue;

            std::string label;
            if(parts.size() > 2)
            {
                // Use the rest of the data as the label
                for(size_t i = 2; i < parts.size(); i++)
                {
                    if(label.length() > 0)
                    {
                        label += ", ";
                    }
                    label += parts[i].ToUtf8();
                }
            }
            else
            {
                label = "[NONE]";
            }

            mPoints[label].push_back(p);
        }
    }

    BindPoints();

    DrawMap();
}

void MainWindow::ClearPoints()
{
    mPoints.clear();
    DrawMap();
}

void MainWindow::ShowToggled(bool checked)
{
    (void)checked;

    DrawMap();
}

void MainWindow::Refresh()
{
    DrawMap();
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    mOrigin = event->pos();
    if(!mRubberBand)
    {
        mRubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    }
    mRubberBand->setGeometry(QRect(mOrigin, QSize()));
    mRubberBand->show();
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if(mRubberBand)
    {
        mRubberBand->setGeometry(QRect(mOrigin, event->pos()).normalized());
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if(mRubberBand)
    {
        mRubberBand->hide();
    }

    // Gets buggy at 25% zoom
    if(mZoomScale >= 80)
    {
        return;
    }

    std::shared_ptr<objects::SpawnLocationGroup> grp;

    QString selectedLGroup = ui.comboBox_SpawnEdit->currentText();
    if(selectedLGroup.length() > 0 && selectedLGroup != "All")
    {
        bool success = false;
        uint32_t key = libcomp::String(selectedLGroup.toStdString())
            .ToInteger<uint32_t>(&success);
        if(success && mZone.SpawnLocationGroupsKeyExists(key))
        {
            grp = mZone.GetSpawnLocationGroups(key);
        }
    }

    if(!grp)
    {
        return;
    }

    QPoint p1 = mDrawTarget->mapFromGlobal(this->mapToGlobal(mOrigin));
    QPoint p2 = mDrawTarget->mapFromGlobal(this->mapToGlobal(event->pos()));

    int32_t x1 = p1.x();
    int32_t y1 = p1.y();
    int32_t x2 = p2.x();
    int32_t y2 = p2.y();

    if(x1 > x2)
    {
        int32_t tempX = x1;
        x1 = x2;
        x2 = tempX;
    }

    if(y1 > y2)
    {
        int32_t tempY = y1;
        y1 = y2;
        y2 = tempY;
    }

    // If no width or height, stop here
    if(x1 == x2 || y1 == y2)
    {
        return;
    }

    auto loc = std::make_shared<objects::SpawnLocation>();
    loc->SetX(((float)x1 * (float)mZoomScale) + mOffsetX);
    loc->SetY(((float)y1 * (float)(-mZoomScale)) + mOffsetY);
    loc->SetWidth((float)abs(x2 - x1) * mZoomScale);
    loc->SetHeight((float)(y2 - y1) * mZoomScale);

    grp->AppendLocations(loc);

    BindSpawns();

    DrawMap();
}

void MainWindow::ComboBox_SpawnEdit_IndexChanged(const QString& str)
{
    (void)str;

    BindSpawns();

    DrawMap();
}

void MainWindow::SpawnLocationRemoveSelected()
{
    QItemSelectionModel* select = ui.tableWidget_SpawnLocation->selectionModel();

    for(auto selection : select->selectedRows())
    {
        int row = selection.row();
        auto cell = ui.tableWidget_SpawnLocation->item(row, 0);
        if(cell)
        {
            bool b1 = false, b2 = false, b3 = false, b4 = false, b5 = false;

            uint32_t groupID = libcomp::String(cell->text().toStdString())
                .ToInteger<uint32_t>(&b1);
            float x = libcomp::String(ui.tableWidget_SpawnLocation->item(row, 1)
                ->text().toStdString()).ToDecimal<float>(&b2);
            float y = libcomp::String(ui.tableWidget_SpawnLocation->item(row, 2)
                ->text().toStdString()).ToDecimal<float>(&b3);
            float width = libcomp::String(ui.tableWidget_SpawnLocation->item(row, 3)
                ->text().toStdString()).ToDecimal<float>(&b4);
            float height = libcomp::String(ui.tableWidget_SpawnLocation->item(row, 4)
                ->text().toStdString()).ToDecimal<float>(&b5);

            if(b1 && b2 && b3 && b4 && b5 && mZone.SpawnLocationGroupsKeyExists(groupID))
            {
                auto grp = mZone.GetSpawnLocationGroups(groupID);
                for(size_t i = 0; i < grp->LocationsCount(); i++)
                {
                    auto loc = grp->GetLocations(i);
                    if(loc->GetX() == x && loc->GetY() == y &&
                        loc->GetWidth() == width && loc->GetHeight() == height)
                    {
                        grp->RemoveLocations(i);
                        break;
                    }
                }
            }
        }
    }

    BindSpawns();

    DrawMap();
}

void MainWindow::PointGroupClicked()
{
    mHiddenPoints.clear();

    for(int i = 0; i < ui.tableWidget_Points->rowCount(); i++)
    {
        auto label = ui.tableWidget_Points->item(i, 0)->text().toStdString();
        auto chk = (QCheckBox*)ui.tableWidget_Points->cellWidget(i, 2);

        if(!chk->isChecked())
        {
            mHiddenPoints.insert(label);
        }
    }

    DrawMap();
}

bool MainWindow::LoadMapFromZone(QString path)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(path.toUtf8().constData());
    const tinyxml2::XMLElement *rootNode = doc.RootElement();
    const tinyxml2::XMLElement *objNode = rootNode->FirstChildElement("object");
    if(objNode == nullptr)
    {
        return false;
    }

    // Reset all fields
    mZone.ClearBazaars();
    mZone.ClearNPCs();
    mZone.ClearObjects();
    mZone.ClearSetupActions();
    mZone.ClearSpawnGroups();
    mZone.ClearSpawnLocationGroups();
    mZone.ClearSpawns();
    mZone.ClearSpots();

    if(!mZone.Load(doc, *objNode))
    {
        return false;
    }

    mZoneData = mDefinitions->GetZoneData(mZone.GetID());
    if(!mZoneData)
    {
        return false;
    }

    mQmpFile = mDefinitions->LoadQmpFile(mZoneData->GetFile()->GetQmpFile(), &*mDatastore);
    if(!mQmpFile)
    {
        return false;
    }

    setWindowTitle(libcomp::String("COMP_hack Map Manager - %1 (%2)")
        .Arg(mZone.GetID()).Arg(mZone.GetDynamicMapID()).C());

    mPoints.clear();

    ui.comboBox_SpawnEdit->clear();
    ui.comboBox_SpawnEdit->addItem("All");
    for(auto pair : mZone.GetSpawnLocationGroups())
    {
        ui.comboBox_SpawnEdit->addItem(libcomp::String("%1").Arg(pair.first).C());
    }

    BindNPCs();
    BindObjects();
    BindSpawns();
    BindPoints();

    DrawMap();

    return true;
}

void MainWindow::BindNPCs()
{
    ui.tableWidget_NPC->clear();
    ui.tableWidget_NPC->setColumnCount(4);
    ui.tableWidget_NPC->setHorizontalHeaderItem(0, GetTableWidget("ID"));
    ui.tableWidget_NPC->setHorizontalHeaderItem(1, GetTableWidget("X"));
    ui.tableWidget_NPC->setHorizontalHeaderItem(2, GetTableWidget("Y"));
    ui.tableWidget_NPC->setHorizontalHeaderItem(3, GetTableWidget("Rotation"));

    ui.tableWidget_NPC->setRowCount((int)mZone.NPCsCount());
    int i = 0;
    for(auto npc : mZone.GetNPCs())
    {
        ui.tableWidget_NPC->setItem(i, 0, GetTableWidget(libcomp::String("%1")
            .Arg(npc->GetID()).C()));
        ui.tableWidget_NPC->setItem(i, 1, GetTableWidget(libcomp::String("%1")
            .Arg(npc->GetX()).C()));
        ui.tableWidget_NPC->setItem(i, 2, GetTableWidget(libcomp::String("%1")
            .Arg(npc->GetY()).C()));
        ui.tableWidget_NPC->setItem(i, 3, GetTableWidget(libcomp::String("%1")
            .Arg(npc->GetRotation()).C()));
        i++;
    }

    ui.tableWidget_NPC->resizeColumnsToContents();
}

void MainWindow::BindObjects()
{
    ui.tableWidget_Object->clear();
    ui.tableWidget_Object->setColumnCount(4);
    ui.tableWidget_Object->setHorizontalHeaderItem(0, GetTableWidget("ID"));
    ui.tableWidget_Object->setHorizontalHeaderItem(1, GetTableWidget("X"));
    ui.tableWidget_Object->setHorizontalHeaderItem(2, GetTableWidget("Y"));
    ui.tableWidget_Object->setHorizontalHeaderItem(3, GetTableWidget("Rotation"));

    ui.tableWidget_Object->setRowCount((int)mZone.ObjectsCount());
    int i = 0;
    for(auto obj : mZone.GetObjects())
    {
        ui.tableWidget_Object->setItem(i, 0, GetTableWidget(libcomp::String("%1")
            .Arg(obj->GetID()).C()));
        ui.tableWidget_Object->setItem(i, 1, GetTableWidget(libcomp::String("%1")
            .Arg(obj->GetX()).C()));
        ui.tableWidget_Object->setItem(i, 2, GetTableWidget(libcomp::String("%1")
            .Arg(obj->GetY()).C()));
        ui.tableWidget_Object->setItem(i, 3, GetTableWidget(libcomp::String("%1")
            .Arg(obj->GetRotation()).C()));
        i++;
    }

    ui.tableWidget_Object->resizeColumnsToContents();
}

void MainWindow::BindSpawns()
{
    // Set up the Spawn table
    ui.tableWidget_Spawn->clear();
    ui.tableWidget_Spawn->setColumnCount(5);
    ui.tableWidget_Spawn->setHorizontalHeaderItem(0, GetTableWidget("ID"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(1, GetTableWidget("Type"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(2, GetTableWidget("Variant"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(3, GetTableWidget("Name"));
    ui.tableWidget_Spawn->setHorizontalHeaderItem(4, GetTableWidget("Level"));

    ui.tableWidget_Spawn->setRowCount((int)mZone.SpawnsCount());
    int i = 0;
    for(auto sPair : mZone.GetSpawns())
    {
        auto s = sPair.second;
        auto def = mDefinitions->GetDevilData(s->GetEnemyType());

        ui.tableWidget_Spawn->setItem(i, 0, GetTableWidget(libcomp::String("%1")
            .Arg(s->GetID()).C()));
        ui.tableWidget_Spawn->setItem(i, 1, GetTableWidget(libcomp::String("%1")
            .Arg(s->GetEnemyType()).C()));
        ui.tableWidget_Spawn->setItem(i, 2, GetTableWidget(libcomp::String("%1")
            .Arg(s->GetVariantType()).C()));
        ui.tableWidget_Spawn->setItem(i, 3, GetTableWidget(libcomp::String("%1")
            .Arg(def ? def->GetBasic()->GetName() : "?").C()));
        ui.tableWidget_Spawn->setItem(i, 4, GetTableWidget(libcomp::String("%1")
            .Arg(def ? def->GetGrowth()->GetBaseLevel() : 0).C()));
        i++;
    }

    ui.tableWidget_Spawn->resizeColumnsToContents();

    // Set up the Spawn Group table
    ui.tableWidget_SpawnGroup->clear();
    ui.tableWidget_SpawnGroup->setColumnCount(3);
    ui.tableWidget_SpawnGroup->setHorizontalHeaderItem(0, GetTableWidget("GroupID"));
    ui.tableWidget_SpawnGroup->setHorizontalHeaderItem(1, GetTableWidget("SpawnID"));
    ui.tableWidget_SpawnGroup->setHorizontalHeaderItem(3, GetTableWidget("Count"));

    ui.tableWidget_SpawnGroup->setRowCount((int)mZone.SpawnGroupsCount());
    i = 0;
    for(auto sgPair : mZone.GetSpawnGroups())
    {
        auto sg = sgPair.second;
        for(auto sPair : sg->GetSpawns())
        {
            ui.tableWidget_SpawnGroup->setItem(i, 0, GetTableWidget(libcomp::String("%1")
                .Arg(sg->GetID()).C()));
            ui.tableWidget_SpawnGroup->setItem(i, 1, GetTableWidget(libcomp::String("%1")
                .Arg(sPair.first).C()));
            ui.tableWidget_SpawnGroup->setItem(i, 3, GetTableWidget(libcomp::String("%1")
                .Arg(sPair.second).C()));
            i++;
        }
    }

    ui.tableWidget_SpawnGroup->resizeColumnsToContents();

    // Set up the Spawn Location table
    ui.tableWidget_SpawnLocation->clear();
    ui.tableWidget_SpawnLocation->setColumnCount(6);
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(0, GetTableWidget("LGroupID"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(1, GetTableWidget("X"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(2, GetTableWidget("Y"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(3, GetTableWidget("Width"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(4, GetTableWidget("Height"));
    ui.tableWidget_SpawnLocation->setHorizontalHeaderItem(5, GetTableWidget("RespawnTime"));

    uint32_t locKey = static_cast<uint32_t>(-1);
    QString selectedLGroup = ui.comboBox_SpawnEdit->currentText();
    bool allLocs = selectedLGroup == "All";
    if(selectedLGroup.length() > 0 && !allLocs)
    {
        bool success = false;
        locKey = libcomp::String(selectedLGroup.toStdString())
            .ToInteger<uint32_t>(&success);
    }

    auto locGroup = mZone.GetSpawnLocationGroups(locKey);

    int locCount = 0;
    for(auto grpPair : mZone.GetSpawnLocationGroups())
    {
        if(allLocs || grpPair.second == locGroup)
        {
            locCount += (int)grpPair.second->LocationsCount();
        }
    }

    ui.tableWidget_SpawnLocation->setRowCount(locCount);
    i = 0;
    for(auto grpPair : mZone.GetSpawnLocationGroups())
    {
        auto grp = grpPair.second;
        if(allLocs || grp == locGroup)
        {
            for(auto loc : grp->GetLocations())
            {
                ui.tableWidget_SpawnLocation->setItem(i, 0, GetTableWidget(libcomp::String("%1")
                    .Arg(grp->GetID()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 1, GetTableWidget(libcomp::String("%1")
                    .Arg(loc->GetX()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 2, GetTableWidget(libcomp::String("%1")
                    .Arg(loc->GetY()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 3, GetTableWidget(libcomp::String("%1")
                    .Arg(loc->GetWidth()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 4, GetTableWidget(libcomp::String("%1")
                    .Arg(loc->GetHeight()).C()));
                ui.tableWidget_SpawnLocation->setItem(i, 5, GetTableWidget(libcomp::String("%1")
                    .Arg(grp->GetRespawnTime()).C()));
                i++;
            }
        }
    }

    ui.tableWidget_SpawnLocation->resizeColumnsToContents();
}

void MainWindow::BindPoints()
{
    ui.tableWidget_Points->clear();
    ui.tableWidget_Points->setColumnCount(3);
    ui.tableWidget_Points->setHorizontalHeaderItem(0, GetTableWidget("Label"));
    ui.tableWidget_Points->setHorizontalHeaderItem(1, GetTableWidget("Count"));
    ui.tableWidget_Points->setHorizontalHeaderItem(2, GetTableWidget("Show"));

    ui.tableWidget_Points->setRowCount((int)mPoints.size());
    int i = 0;
    for(auto pair : mPoints)
    {
        ui.tableWidget_Points->setItem(i, 0, GetTableWidget(pair.first.c_str()));
        ui.tableWidget_Points->setItem(i, 1, GetTableWidget(libcomp::String("%1")
            .Arg(pair.second.size()).C()));

        QCheckBox* chk = new QCheckBox();
        chk->setCheckState(mHiddenPoints.find(pair.first) == mHiddenPoints.end()
            ? Qt::Checked : Qt::Unchecked);
        connect(chk, SIGNAL(clicked()), this, SLOT(PointGroupClicked()));
        ui.tableWidget_Points->setCellWidget(i, 2, chk);

        i++;
    }

    ui.tableWidget_Points->resizeColumnsToContents();
}

QTableWidgetItem* MainWindow::GetTableWidget(std::string name, bool readOnly)
{
    auto item = new QTableWidgetItem(name.c_str());

    if(readOnly)
    {
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
    }

    return item;
}

void MainWindow::DrawMap()
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
        elems[elem->GetID()] = elem->GetUnknown();
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
                // Lines
                painter.setPen(QPen(Qt::blue));
                painter.setBrush(QBrush(Qt::blue));
                break;
            case 2:
                // Toggleable
                painter.setPen(QPen(Qt::green));
                painter.setBrush(QBrush(Qt::green));
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

    auto spots = mDefinitions->GetSpotData(mZone.GetDynamicMapID());
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
            p.first = (float)(((x - xc) * cos(rot)) - ((y - yc) * sin(rot)) + xc);
            p.second = (float)(((x - xc) * sin(rot)) + ((y - yc) * cos(rot)) + yc);
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
            libcomp::String("[%1] %2").Arg(spotPair.second->GetType()).Arg(spotPair.first).C());
    }

    // Draw the starting point
    painter.setPen(QPen(Qt::magenta));
    painter.setBrush(QBrush(Qt::magenta));

    xVals.insert(mZone.GetStartingX());
    yVals.insert(mZone.GetStartingY());

    painter.drawEllipse(QPoint(Scale(mZone.GetStartingX()), Scale(-mZone.GetStartingY())),
        3, 3);

    // Draw NPCs
    if(ui.checkBox_NPC->isChecked())
    {
        painter.setPen(QPen(Qt::green));
        painter.setBrush(QBrush(Qt::green));

        for(auto npc : mZone.GetNPCs())
        {
            xVals.insert(npc->GetX());
            yVals.insert(npc->GetY());
            painter.drawEllipse(QPoint(Scale(npc->GetX()), Scale(-npc->GetY())),
                3, 3);

            painter.drawText(QPoint(Scale(npc->GetX() + 20.f), Scale(-npc->GetY())),
                libcomp::String("%1").Arg(npc->GetID()).C());
        }
    }

    // Draw Objects
    if(ui.checkBox_Object->isChecked())
    {
        painter.setPen(QPen(Qt::blue));
        painter.setBrush(QBrush(Qt::blue));

        for(auto obj : mZone.GetObjects())
        {
            xVals.insert(obj->GetX());
            yVals.insert(obj->GetY());
            painter.drawEllipse(QPoint(Scale(obj->GetX()), Scale(-obj->GetY())),
                3, 3);

            painter.drawText(QPoint(Scale(obj->GetX() + 20.f), Scale(-obj->GetY())),
                libcomp::String("%1").Arg(obj->GetID()).C());
        }
    }

    // Draw Points
    painter.setPen(QPen(Qt::gray));
    painter.setBrush(QBrush(Qt::gray));

    for(auto pair : mPoints)
    {
        if(mHiddenPoints.find(pair.first) == mHiddenPoints.end())
        {
            for(auto p : pair.second)
            {
                xVals.insert(p.X);
                yVals.insert(p.Y);
                painter.drawEllipse(QPoint(Scale(p.X), Scale(-p.Y)),
                    3, 3);
            }
        }
    }

    // Draw Spawn Locations
    if(ui.checkBox_Spawn->isChecked())
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

        auto locGroup = mZone.GetSpawnLocationGroups(locKey);

        for(auto grpPair : mZone.GetSpawnLocationGroups())
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

    // Take min X and max Y
    mOffsetX = *xVals.begin();
    mOffsetY = *yVals.rbegin();

    painter.end();

    mDrawTarget->setPicture(pic);
    ui.scrollArea->setWidget(mDrawTarget);

    ui.scrollArea->horizontalScrollBar()->setValue(xScroll);
    ui.scrollArea->verticalScrollBar()->setValue(yScroll);
}

int32_t MainWindow::Scale(int32_t point)
{
    return (int32_t)(point / mZoomScale);
}

int32_t MainWindow::Scale(float point)
{
    return (int32_t)(point / mZoomScale);
}
