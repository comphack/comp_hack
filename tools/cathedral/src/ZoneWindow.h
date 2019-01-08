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

// Qt Includes
#include <PushIgnore.h>
#include "ui_ZoneWindow.h"
#include <PopIgnore.h>

// C++11 Standard Includes
#include <memory>
#include <set>

namespace objects
{

class Action;
class MiSpotData;
class MiZoneData;
class QmpFile;
class ServerNPC;
class ServerObject;
class ServerZone;
class ServerZonePartial;
class SpawnLocation;

} // namespace objects

class MainWindow;

class MergedZone
{
public:
    std::shared_ptr<objects::ServerZone> Definition;
    std::shared_ptr<objects::ServerZone> CurrentZone;
    std::shared_ptr<objects::ServerZonePartial> CurrentPartial;
};

class ZoneWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ZoneWindow(MainWindow *pMainWindow, QWidget *parent = 0);
    ~ZoneWindow();

    std::shared_ptr<MergedZone> GetMergedZone() const;
    std::map<uint32_t,
        std::shared_ptr<objects::ServerZonePartial>> GetLoadedPartials() const;
    std::set<uint32_t> GetSelectedPartials() const;

    bool ShowZone();

    void RebuildNamedDataSet(const libcomp::String& objType);

    std::list<std::shared_ptr<objects::Action>> GetLoadedActions(
        bool forUpdate);

    bool ShowSpot(uint32_t spotID);

    void closeEvent(QCloseEvent* event) override;

public slots:
    void LoadZoneFile();

protected:
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    bool eventFilter(QObject* o, QEvent* e);

protected slots:
    void LoadPartialDirectory();
    void LoadPartialFile();
    void SaveFile();
    void SaveAllFiles();
    void ApplyPartials();

    void AddNPC();
    void AddObject();
    void AddSpawn();
    void RemoveNPC();
    void RemoveObject();
    void RemoveSpawn();

    void ZoneViewUpdated();
    void SelectListObject();
    void MainTabChanged();
    void SpawnTabChanged();

    void NPCMoved(std::shared_ptr<libcomp::Object> obj,
        bool up);
    void ObjectMoved(std::shared_ptr<libcomp::Object> obj,
        bool up);

    void Zoom();
    void ShowToggled(bool checked);
    void Refresh();

private:
    bool LoadZonePartials(const libcomp::String& path);

    void SaveZone();
    void SavePartials(const std::set<uint32_t>& partialIDs);

    void ResetAppliedPartials(std::set<uint32_t> newPartials = {});
    void RebuildCurrentZoneDisplay();
    void UpdateMergedZone(bool redraw);

    bool LoadMapFromZone();

    void LoadProperties();
    void SaveProperties();

    bool GetSpotPosition(uint32_t dynamicMapID,
        uint32_t spotID, float& x, float& y, float& rot) const;

    void BindNPCs();
    void BindObjects();
    void BindSpawns();
    void BindSpots();

    void DrawMap();

    void DrawNPC(const std::shared_ptr<objects::ServerNPC>& npc,
        bool selected, QPainter& painter);
    void DrawObject(const std::shared_ptr<objects::ServerObject>& obj,
        bool selected, QPainter& painter);
    void DrawSpawnLocation(const std::shared_ptr<objects::SpawnLocation>& loc,
        QPainter& painter);
    void DrawSpot(const std::shared_ptr<objects::MiSpotData>& spotDef,
        bool selected, QPainter& painter);

    int32_t Scale(int32_t point);
    int32_t Scale(float point);

    MainWindow *mMainWindow;

    Ui::ZoneWindow ui;

    int32_t mOffsetX;
    int32_t mOffsetY;

    QPoint mLastMousePos;
    bool mDragging;

    libcomp::String mZonePath;
    std::shared_ptr<MergedZone> mMergedZone;
    std::shared_ptr<objects::MiZoneData> mZoneData;
    std::shared_ptr<objects::QmpFile> mQmpFile;
    std::set<std::string> mHiddenPoints;

    std::set<uint32_t> mSelectedPartials;
    std::map<uint32_t,
        std::shared_ptr<objects::ServerZonePartial>> mZonePartials;
    std::map<uint32_t, libcomp::String> mZonePartialFiles;
};

#endif // TOOLS_CATHEDRAL_SRC_ZONEWINDOW_H
