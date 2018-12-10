/**
 * @file tools/cathedral/src/MainWindow.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main window definition.
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

#ifndef TOOLS_CATHEDRAL_SRC_MAINWINDOW_H
#define TOOLS_CATHEDRAL_SRC_MAINWINDOW_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// libcomp Includes
#include <DataStore.h>
#include <DefinitionManager.h>

// objects Includes
#include <ServerZone.h>

namespace Ui
{

class MainWindow;

} // namespace Ui

class NPCListWindow;
class SpotListWindow;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *pParent = 0);
    ~MainWindow();

    bool Init();

    std::shared_ptr<libcomp::DataStore> GetDatastore() const;
    std::shared_ptr<libcomp::DefinitionManager> GetDefinitions() const;

    NPCListWindow* GetNPCList() const;
    SpotListWindow* GetSpotList() const;

protected slots:
    void OpenNPCs();
    void OpenSpots();

protected:
    void ReloadZoneData();

private slots:
    void BrowseZone();

protected:
    NPCListWindow *mNPCWindow;
    SpotListWindow *mSpotWindow;

private:
    Ui::MainWindow *ui;

    std::shared_ptr<libcomp::DataStore> mDatastore;
    std::shared_ptr<libcomp::DefinitionManager> mDefinitions;

    QString mActiveZonePath;
    std::shared_ptr<objects::ServerZone> mActiveZone;
};

static inline QString qs(const libcomp::String& s)
{
    return QString::fromUtf8(s.C());
}

static inline libcomp::String cs(const QString& s)
{
    return libcomp::String(s.toUtf8().constData());
}

#endif // TOOLS_CATHEDRAL_SRC_MAINWINDOW_H
