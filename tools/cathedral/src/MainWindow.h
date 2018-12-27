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
#include <QMainWindow>
#include <PopIgnore.h>

// libcomp Includes
#include <BinaryDataSet.h>
#include <DataStore.h>
#include <DefinitionManager.h>

// objects Includes
#include <ServerZone.h>

namespace objects
{

class MiCEventMessageData;

} // namespace objects

namespace Ui
{

class MainWindow;

} // namespace Ui

class EventWindow;
class ObjectSelectorWindow;
class ZoneWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *pParent = 0);
    ~MainWindow();

    bool Init();

    std::shared_ptr<libcomp::DataStore> GetDatastore() const;
    std::shared_ptr<libcomp::DefinitionManager> GetDefinitions() const;

    EventWindow* GetEvents() const;
    ZoneWindow* GetZones() const;

    std::shared_ptr<objects::MiCEventMessageData> GetEventMessage(
        int32_t msgID) const;

    std::shared_ptr<libcomp::BinaryDataSet> GetBinaryDataSet(
        const libcomp::String& objType) const;

    ObjectSelectorWindow* GetObjectSelector(
        const libcomp::String& objType) const;

    void ResetEventCount();

protected slots:
    void OpenEvents();
    void OpenZone();
    void ViewObjectList();

protected:
    bool LoadBinaryData(const libcomp::String& binaryFile,
        const libcomp::String& objName, bool decrypt, bool addSelector = false,
        bool selectorAllowBlanks = false);

    void CloseAllWindows();

    void closeEvent(QCloseEvent* event) override;

private slots:
    void BrowseZone();

protected:
    EventWindow *mEventWindow;
    ZoneWindow *mZoneWindow;

private:
    Ui::MainWindow *ui;

    std::shared_ptr<libcomp::DataStore> mDatastore;
    std::shared_ptr<libcomp::DefinitionManager> mDefinitions;
    
    std::unordered_map<libcomp::String,
        std::shared_ptr<libcomp::BinaryDataSet>> mBinaryDataSets;

    std::unordered_map<libcomp::String,
        ObjectSelectorWindow*> mObjectSelectors;

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
