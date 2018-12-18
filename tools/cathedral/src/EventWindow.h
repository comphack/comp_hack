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
#include <PopIgnore.h>

 // objects Includes
#include <CString.h>
#include <Object.h>

// Standard C++11 Includes
#include <set>

namespace Ui
{

class EventWindow;

} // namespace Ui

class EventFile;
class EventTreeItem;
class MainWindow;
class QTreeWidgetItem;

class EventWindow : public QWidget
{
    Q_OBJECT

public:
    explicit EventWindow(MainWindow *pMainWindow, QWidget *pParent = 0);
    virtual ~EventWindow();

private slots:
    void FileSelectionChanged();
    void LoadDirectory();
    void LoadFile();
    void TreeSelectionChanged();

private:
    void LoadFilesFromPaths(const QStringList& inPaths);
    bool LoadFileFromPath(const libcomp::String& path);
    bool SelectFile(const libcomp::String& path);
    void AddEventToTree(const libcomp::String& id, EventTreeItem* parent,
        const std::shared_ptr<EventFile>& file,
        std::set<libcomp::String>& seen);

    libcomp::String GetInlineMessageText(const libcomp::String& raw);

    MainWindow *mMainWindow;

    std::unordered_map<libcomp::String, std::shared_ptr<EventFile>> mFiles;

    std::weak_ptr<libcomp::Object> mActiveObject;

    Ui::EventWindow *ui;
};

#endif // TOOLS_CATHEDRAL_SRC_EVENTWINDOW_H
