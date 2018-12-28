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

#include "EventWindow.h"

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

private slots:
    void FileSelectionChanged();
    void LoadDirectory();
    void LoadFile();
    void SaveFile();
    void NewFile();
    void NewEvent();
    void RemoveEvent();
    void Refresh();
    void CurrentEventEdited();
    void TreeSelectionChanged();

private:
    bool LoadFileFromPath(const libcomp::String& path);
    bool SelectFile(const libcomp::String& path);

    std::shared_ptr<objects::Event> GetNewEvent(
        objects::Event::EventType_t type) const;

    void BindSelectedEvent();
    void BindEventEditControls(QWidget* eNode);

    void AddEventToTree(const libcomp::String& id, EventTreeItem* parent,
        const std::shared_ptr<EventFile>& file,
        std::set<libcomp::String>& seen, int32_t eventIdx = -1);

    void RebuildLocalIDMap(const std::shared_ptr<EventFile>& file);
    void RebuildGlobalIDMap();

    void SimplifyObjectXML(std::list<tinyxml2::XMLNode*> nodes);

    libcomp::String GetInlineMessageText(const libcomp::String& raw,
        size_t limit = 0);

    MainWindow *mMainWindow;

    std::unordered_map<libcomp::String, std::shared_ptr<EventFile>> mFiles;
    std::unordered_map<libcomp::String, libcomp::String> mGlobalIDMap;

    std::shared_ptr<FileEvent> mCurrentEvent;

    Ui::EventWindow *ui;
};

#endif // TOOLS_CATHEDRAL_SRC_EVENTWINDOW_H
