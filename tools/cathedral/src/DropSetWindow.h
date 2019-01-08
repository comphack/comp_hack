/**
 * @file tools/cathedral/src/DropSetWindow.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a window that handles drop set viewing and modification.
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

#ifndef TOOLS_CATHEDRAL_SRC_DROPSETWINDOW_H
#define TOOLS_CATHEDRAL_SRC_DROPSETWINDOW_H

// Qt Includes
#include <PushIgnore.h>
#include "ui_DropSetWindow.h"
#include <PopIgnore.h>

// object Includes
#include <DropSet.h>

// libcomp Includes
#include <CString.h>

namespace objects
{

class Action;

} // namespace objects

class FileDropSet : public objects::DropSet
{
public:
    FileDropSet() {}
    virtual ~FileDropSet() {}

    libcomp::String Desc;
};

class DropSetFile;
class MainWindow;

class DropSetWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DropSetWindow(MainWindow *pMainWindow, QWidget *pParent = 0);
    virtual ~DropSetWindow();

    void RebuildNamedDataSet();

    size_t GetLoadedDropSetCount();

    void closeEvent(QCloseEvent* event) override;

private slots:
    void FileSelectionChanged();
    void NewDropSet();
    void NewFile();
    void RemoveDropSet();
    void LoadDirectory();
    void LoadFile();
    void SaveFile();
    void SaveAllFiles();
    void Refresh();
    void SelectDropSet();

private:
    bool LoadFileFromPath(const libcomp::String& path);
    bool SelectFile(const libcomp::String& path);

    void SaveFiles(const std::list<libcomp::String>& paths);

    MainWindow *mMainWindow;

    std::unordered_map<libcomp::String, std::shared_ptr<DropSetFile>> mFiles;

    Ui::DropSetWindow *ui;
};

#endif // TOOLS_CATHEDRAL_SRC_DROPSETWINDOW_H
