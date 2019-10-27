/**
 * @file tools/cathedral/src/EventWindow.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a window that handles event viewing and modification.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTWINDOW_H
#define TOOLS_CATHEDRAL_SRC_EVENTWINDOW_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>

#include "ui_EventWindow.h"
#include <PopIgnore.h>

// objects Includes
#include <CString.h>
#include <Event.h>
#include <Object.h>

// Standard C++11 Includes
#include <set>

namespace objects
{

class Action;

} // namespace objects

namespace Ui
{

class FindEventAction;

} // namespace Ui

class EventFile;
class EventTreeItem;
class FileEvent;
class MainWindow;
class QTreeWidgetItem;

class EventWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EventWindow(MainWindow *pMainWindow, QWidget *pParent = 0);
    virtual ~EventWindow();

    bool GoToEvent(const libcomp::String& eventID);

    size_t GetLoadedEventCount() const;

    void ChangeEventID(const libcomp::String& currentID);

    std::list<libcomp::String> GetCurrentEventIDs() const;

    libcomp::String GetCurrentFile() const;

    std::list<libcomp::String> GetCurrentFiles() const;

    std::list<std::shared_ptr<objects::Event>> GetFileEvents(
        const libcomp::String& path) const;

    void closeEvent(QCloseEvent* event) override;

private slots:
    void FileSelectionChanged();
    void LoadDirectory();
    void LoadFile();
    void ReloadFile();
    void SaveFile();
    void SaveAllFiles();
    void NewFile();
    void NewEvent();
    void Convert();
    void RemoveEvent();
    void Search();
    void Refresh(bool reselectEvent = true);
    void GoTo();
    void FindAction();
    void FindNextAction();
    void Back();
    void FileViewChanged();
    void CollapseAll();
    void ExpandAll();
    void CurrentEventEdited();
    void TreeSelectionChanged();

    void MoveUp();
    void MoveDown();
    void Reorganize();
    void ChangeCurrentEventID();
    void ChangeFileEventIDs();
    void ChangeTreeBranchIDs();

private:
    bool LoadFileFromPath(const libcomp::String& path);
    bool SelectFile(const libcomp::String& path);

    void SaveFiles(const std::list<libcomp::String>& paths);

    std::shared_ptr<objects::Event> GetNewEvent(
        objects::Event::EventType_t type) const;

    void BindSelectedEvent(bool storePrevious);
    void BindEventEditControls(QWidget* eNode);

    void AddEventToTree(const libcomp::String& id, EventTreeItem* parent,
        const std::shared_ptr<EventFile>& file,
        std::set<libcomp::String>& seen, int32_t eventIdx = -1);

    void ChangeEventIDs(const std::unordered_map<libcomp::String,
        libcomp::String>& idMap);
    bool ChangeActionEventIDs(const std::unordered_map<libcomp::String,
        libcomp::String>& idMap, const std::list<std::shared_ptr<
        objects::Action>>& actions);

    std::list<std::shared_ptr<objects::Action>> GetAllActions(
        const std::list<std::shared_ptr<objects::Action>>& actions);

    libcomp::String GetCommonEventPrefix(
        const std::shared_ptr<EventFile>& file);
    libcomp::String GetEventTypePrefix(const libcomp::String& prefix,
        objects::Event::EventType_t eventType);
    libcomp::String GetNewEventID(const std::shared_ptr<EventFile>& file,
        objects::Event::EventType_t eventType);

    void UpdatePreviousEvents(const libcomp::String& last);

    void RebuildLocalIDMap(const std::shared_ptr<EventFile>& file);
    void RebuildGlobalIDMap();

    libcomp::String GetInlineMessageText(const libcomp::String& raw,
        size_t limit = 0);

    MainWindow *mMainWindow;

    std::unordered_map<libcomp::String, std::shared_ptr<EventFile>> mFiles;
    std::unordered_map<libcomp::String, libcomp::String> mGlobalIDMap;

    libcomp::String mCurrentFileName;
    std::shared_ptr<FileEvent> mCurrentEvent;

    std::list<libcomp::String> mPreviousEventIDs;

    Ui::EventWindow *ui;

    QWidget *findActionWidget;
    Ui::FindEventAction *findAction;
};

#endif // TOOLS_CATHEDRAL_SRC_EVENTWINDOW_H
