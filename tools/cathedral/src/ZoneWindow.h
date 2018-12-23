/**
 * @file tools/cathedral/src/ZoneWindow.h
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

#ifndef TOOLS_CATHEDRAL_SRC_ZONEWINDOW_H
#define TOOLS_CATHEDRAL_SRC_ZONEWINDOW_H

#include <PushIgnore.h>
#include "ui_ZoneWindow.h"
#include <PopIgnore.h>

#include <QLabel>
#include <QMouseEvent>

#include <MiZoneData.h>
#include <QmpFile.h>
#include <ServerZone.h>

class MainWindow;

class GenericPoint
{
public:
    float X = 0.f;
    float Y = 0.f;
};

class ZoneWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ZoneWindow(MainWindow *pMainWindow, QWidget *parent = 0);
    ~ZoneWindow();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

protected slots:
    void ShowOpenDialog();
    void ShowSaveDialog();
    void Zoom200();
    void Zoom100();
    void Zoom50();
    void Zoom25();
    void PlotPoints();
    void ClearPoints();
    void ShowToggled(bool checked);
    void Refresh();

    void ComboBox_SpawnEdit_IndexChanged(const QString& str);
    void SpawnLocationRemoveSelected();

    void PointGroupClicked();

private:
    bool LoadMapFromZone(QString path);

    bool GetSpotPosition(uint32_t dynamicMapID,
        uint32_t spotID, float& x, float& y, float& rot) const;

    void BindNPCs();
    void BindObjects();
    void BindSpawns();
    void BindPoints();

    QTableWidgetItem* GetTableWidget(std::string name,
        bool readOnly = true);

    void DrawMap();

    int32_t Scale(int32_t point);
    int32_t Scale(float point);

    MainWindow *mMainWindow;

    Ui::ZoneWindow ui;
    QLabel* mDrawTarget;
    QRubberBand* mRubberBand;
    QPoint mOrigin;

    float mOffsetX;
    float mOffsetY;

    objects::ServerZone mZone;
    std::shared_ptr<objects::MiZoneData> mZoneData;
    std::shared_ptr<objects::QmpFile> mQmpFile;
    std::map<std::string, std::list<GenericPoint>> mPoints;
    std::set<std::string> mHiddenPoints;

    uint8_t mZoomScale;
};

#endif // TOOLS_CATHEDRAL_SRC_ZONEWINDOW_H
