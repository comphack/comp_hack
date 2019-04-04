/**
 * @file tools/cathedral/src/EventWindow.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a window that handles event viewing and modification.
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

#include "EventWindow.h"

// Cathedral Includes
#include "ActionDelay.h"
#include "ActionDisplayMessage.h"
#include "ActionList.h"
#include "ActionMap.h"
#include "ActionSpawn.h"
#include "ActionStageEffect.h"
#include "ActionStartEvent.h"
#include "ActionZoneInstance.h"
#include "BinaryDataNamedSet.h"
#include "DynamicList.h"
#include "EventUI.h"
#include "EventDirectionUI.h"
#include "EventExNPCMessageUI.h"
#include "EventITimeUI.h"
#include "EventMultitalkUI.h"
#include "EventNPCMessageUI.h"
#include "EventOpenMenuUI.h"
#include "EventPerformActionsUI.h"
#include "EventPlaySceneUI.h"
#include "EventPromptUI.h"
#include "EventRef.h"
#include "MainWindow.h"
#include "XmlHandler.h"
#include "ZoneWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QCheckBox>
#include <QDir>
#include <QDirIterator>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QRadioButton>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QTextEdit>

#include "ui_FindEventAction.h"
#include <PopIgnore.h>

// object Includes
#include <Event.h>
#include <EventChoice.h>
#include <EventDirection.h>
#include <EventExNPCMessage.h>
#include <EventITime.h>
#include <EventMultitalk.h>
#include <EventNPCMessage.h>
#include <EventOpenMenu.h>
#include <EventPerformActions.h>
#include <EventPlayScene.h>
#include <EventPrompt.h>
#include <MiCEventMessageData.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

class EventTreeItem : public QTreeWidgetItem
{
public:
    EventTreeItem(QTreeWidgetItem* parent, const libcomp::String& eventID = "",
        int32_t fileIdx = -1) : QTreeWidgetItem(parent)
    {
        EventID = eventID;
        FileIdx = fileIdx;
    }

    libcomp::String EventID;
    int32_t FileIdx;
    bool FirstFileRef = false;
    bool ExtensionNode = false;
};

class FileEvent
{
public:
    FileEvent(const std::shared_ptr<objects::Event>& e, bool isNew = false)
    {
        Event = e;
        if(isNew)
        {
            HasUpdates = true;
        }
        else
        {
            FileEventID = e->GetID();
            HasUpdates = false;
        }
    }

    std::shared_ptr<objects::Event> Event;
    libcomp::String FileEventID;
    std::list<libcomp::String> Comments;
    bool HasUpdates;
};

class EventFile
{
public:
    libcomp::String Path;
    std::list<std::shared_ptr<FileEvent>> Events;
    std::unordered_map<libcomp::String, int32_t> EventIDMap;
    std::set<libcomp::String> PendingRemovals;
    bool Reordered = false;
};

EventWindow::EventWindow(MainWindow *pMainWindow, QWidget *pParent) :
    QMainWindow(pParent), mMainWindow(pMainWindow)
{
    ui = new Ui::EventWindow;
    ui->setupUi(this);

    findActionWidget = new QWidget;

    findAction = new Ui::FindEventAction;
    findAction->setupUi(findActionWidget);

    QAction *pAction = nullptr;

    QMenu *pMenu = new QMenu(tr("Add Event"));

    pAction = pMenu->addAction("Fork");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::FORK));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("Direction");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::DIRECTION));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("EX NPC Message");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::EX_NPC_MESSAGE));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("I-Time");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::ITIME));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("Multitalk");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::MULTITALK));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("NPC Message");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::NPC_MESSAGE));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("Open Menu");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::OPEN_MENU));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("Perform Actions");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::PERFORM_ACTIONS));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("Play Scene");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::PLAY_SCENE));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    pAction = pMenu->addAction("Prompt");
    pAction->setData(to_underlying(
        objects::Event::EventType_t::PROMPT));
    connect(pAction, SIGNAL(triggered()), this, SLOT(NewEvent()));

    ui->addEvent->setMenu(pMenu);

    for(auto pair : ActionList::GetActions())
    {
        findAction->action->addItem(qs(pair.first), pair.second);
    }

    ui->removeEvent->hide();
    ui->actionMoveUp->setDisabled(true);
    ui->actionMoveDown->setDisabled(true);

    ui->eventBack->hide();

    connect(ui->treeSearch, SIGNAL(textChanged(const QString&)), this,
        SLOT(Search()));

    connect(ui->actionLoadFile, SIGNAL(triggered()), this, SLOT(LoadFile()));
    connect(ui->actionLoadDirectory, SIGNAL(triggered()), this,
        SLOT(LoadDirectory()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(SaveFile()));
    connect(ui->actionSaveAll, SIGNAL(triggered()), this,
        SLOT(SaveAllFiles()));
    connect(ui->actionReloadFile, SIGNAL(triggered()), this,
        SLOT(ReloadFile()));
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(NewFile()));
    connect(ui->eventBack, SIGNAL(clicked()), this, SLOT(Back()));
    connect(ui->removeEvent, SIGNAL(clicked()), this, SLOT(RemoveEvent()));
    connect(ui->files, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(FileSelectionChanged()));
    connect(ui->treeWidget, SIGNAL(itemSelectionChanged()), this,
        SLOT(TreeSelectionChanged()));

    connect(ui->actionRefresh, SIGNAL(triggered()), this, SLOT(Refresh()));
    connect(ui->actionGoto, SIGNAL(triggered()), this, SLOT(GoTo()));
    connect(ui->actionFindAction, SIGNAL(triggered()), this,
        SLOT(FindAction()));
    connect(ui->actionFileView, SIGNAL(toggled(bool)), this,
        SLOT(FileViewChanged()));
    connect(ui->actionCollapseAll, SIGNAL(triggered()), this,
        SLOT(CollapseAll()));
    connect(ui->actionExpandAll, SIGNAL(triggered()), this,
        SLOT(ExpandAll()));

    connect(ui->actionMoveUp, SIGNAL(triggered()), this, SLOT(MoveUp()));
    connect(ui->actionMoveDown, SIGNAL(triggered()), this, SLOT(MoveDown()));
    connect(ui->actionReorganize, SIGNAL(triggered()), this,
        SLOT(Reorganize()));
    connect(ui->actionChangeID, SIGNAL(triggered()), this,
        SLOT(ChangeCurrentEventID()));
    connect(ui->actionChangeFileIDs, SIGNAL(triggered()), this,
        SLOT(ChangeFileEventIDs()));
    connect(ui->actionChangeTreeIDs, SIGNAL(triggered()), this,
        SLOT(ChangeTreeBranchIDs()));

    connect(findAction->next, SIGNAL(clicked()), this,
        SLOT(FindNextAction()));
}

EventWindow::~EventWindow()
{
    delete ui;
    delete findActionWidget;
}

bool EventWindow::GoToEvent(const libcomp::String& eventID)
{
    auto iter = mGlobalIDMap.find(eventID);
    if(iter == mGlobalIDMap.end())
    {
        QMessageBox err;
        err.setText(qs(libcomp::String("Event '%1' is not currently loaded")
            .Arg(eventID)));
        err.exec();

        return false;
    }

    libcomp::String currentPath = mCurrentFileName;
    libcomp::String path = iter->second;

    if(currentPath != path)
    {
        // Switch current file
        ui->files->setCurrentText(qs(path));
        Refresh(false);
    }

    auto file = mFiles[path];
    auto eIter = file->EventIDMap.find(eventID);
    if(eIter != file->EventIDMap.end())
    {
        QList<QTreeWidgetItem*> items = ui->treeWidget->findItems(
            QString("*"), Qt::MatchWrap | Qt::MatchWildcard | Qt::MatchRecursive);
        for(auto item : items)
        {
            auto treeItem = (EventTreeItem*)item;
            if(treeItem->EventID == eventID)
            {
                // Block signals and clear current selection
                bool old = ui->treeWidget->blockSignals(true);
                ui->treeWidget->clearSelection();
                ui->treeWidget->blockSignals(old);

                // Select new item and display (if not already)
                ui->treeWidget->setItemSelected(treeItem, true);

                // Expand everything above the node then scroll to it
                auto parent = treeItem->parent();
                while(parent && !parent->isExpanded())
                {
                    ui->treeWidget->expandItem(parent);
                    parent = parent->parent();
                }

                ui->treeWidget->scrollToItem(item);
                show();
                raise();
                return true;
            }
        }
    }

    return false;
}

size_t EventWindow::GetLoadedEventCount() const
{
    size_t total = 0;
    for(auto& pair : mFiles)
    {
        auto file = pair.second;
        total = (size_t)(total + file->Events.size());
    }

    return total;
}

void EventWindow::ChangeEventID(const libcomp::String& currentID)
{
    auto it = mGlobalIDMap.find(currentID);
    if(it == mGlobalIDMap.end())
    {
        return;
    }

    auto fileIter = mFiles.find(it->second);
    if(fileIter == mFiles.end())
    {
        return;
    }

    std::shared_ptr<FileEvent> fEvent;

    auto file = fileIter->second;
    if(file && file->EventIDMap.find(currentID) != file->EventIDMap.end())
    {
        auto eIter = file->Events.begin();
        std::advance(eIter, file->EventIDMap[currentID]);
        fEvent = *eIter;
    }

    if(fEvent)
    {
        libcomp::String eventID = GetNewEventID(file, fEvent->Event
            ->GetEventType());
        if(eventID.IsEmpty())
        {
            return;
        }

        auto reply = QMessageBox::question(this, "Confirm Rename",
            QString("Event ID '%1' will be changed to '%2' and all currently"
                " loaded event references will be updated automatically"
                " however, no files will be saved at this time. Only the"
                " current zone and loaded zone partials will be updated."
                " Please confirm this action.")
            .arg(currentID.C()).arg(eventID.C()),
            QMessageBox::Yes | QMessageBox::No);
        if(reply != QMessageBox::Yes)
        {
            return;
        }

        // Deselect the tree so everything saves
        ui->treeWidget->clearSelection();

        // Update the event
        fEvent->Event->SetID(eventID);

        std::unordered_map<libcomp::String, libcomp::String> eventIDMap;
        eventIDMap[currentID] = eventID;

        ChangeEventIDs(eventIDMap);

        // Refresh and select the new event
        fEvent->HasUpdates = true;
        RebuildLocalIDMap(file);
        RebuildGlobalIDMap();
        EventRef::RefreshAllEventIDs(mMainWindow);
        Refresh(false);
        GoToEvent(eventID);
    }
    else
    {
        QMessageBox err;
        err.setText(qs(libcomp::String("Event ID '%1' does not exist")
            .Arg(currentID)));
        err.exec();
    }
}

std::list<libcomp::String> EventWindow::GetCurrentEventIDs() const
{
    std::list<libcomp::String> eventIDs;

    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter != mFiles.end())
    {
        for(auto e : fIter->second->Events)
        {
            eventIDs.push_back(e->Event->GetID());
        }
    }

    return eventIDs;
}

libcomp::String EventWindow::GetCurrentFile() const
{
    return mCurrentFileName;
}

std::list<libcomp::String> EventWindow::GetCurrentFiles() const
{
    std::list<libcomp::String> files;
    for(auto& pair : mFiles)
    {
        files.push_back(pair.first);
    }

    return files;
}

std::list<std::shared_ptr<objects::Event>> EventWindow::GetFileEvents(
    const libcomp::String& path) const
{
    std::list<std::shared_ptr<objects::Event>> events;

    auto it = mFiles.find(path);
    if(it != mFiles.end())
    {
        for(auto fEvent : it->second->Events)
        {
            events.push_back(fEvent->Event);
        }
    }

    return events;
}

void EventWindow::closeEvent(QCloseEvent* event)
{
    (void)event;

    mMainWindow->CloseSelectors(this);

    findActionWidget->close();
}

void EventWindow::FileSelectionChanged()
{
    mMainWindow->CloseSelectors(this);

    // Clear the search filter if set
    if(!ui->treeSearch->text().isEmpty())
    {
        ui->treeSearch->blockSignals(true);
        ui->treeSearch->setText("");
        ui->treeSearch->blockSignals(false);
    }

    Refresh(false);
}

void EventWindow::LoadDirectory()
{
    QSettings settings;

    QString qPath = QFileDialog::getExistingDirectory(this,
        tr("Load Event XML folder"), mMainWindow->GetDialogDirectory());
    if(qPath.isEmpty())
    {
        return;
    }

    mMainWindow->SetDialogDirectory(qPath, false);

    ui->files->blockSignals(true);

    QDirIterator it(qPath, QStringList() << "*.xml", QDir::Files,
        QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        libcomp::String path = cs(it.next());
        LoadFileFromPath(path);
    }

    ui->files->blockSignals(false);

    RebuildGlobalIDMap();
    mMainWindow->ResetEventCount();

    // Refresh selection even if it didnt change
    Refresh(false);
}

void EventWindow::LoadFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getOpenFileName(this, tr("Load Event XML"),
        mMainWindow->GetDialogDirectory(), tr("Event XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

    mMainWindow->SetDialogDirectory(qPath, true);

    ui->files->blockSignals(true);

    libcomp::String path = cs(qPath);
    if(LoadFileFromPath(path))
    {
        RebuildGlobalIDMap();
        mMainWindow->ResetEventCount();

        if(ui->files->currentText() != qs(path))
        {
            ui->files->setCurrentText(qs(path));
        }

        ui->files->blockSignals(false);

        Refresh(false);
    }
    else
    {
        ui->files->blockSignals(false);
    }
}

void EventWindow::ReloadFile()
{
    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;

    bool changeExists = file->Reordered ||
        file->PendingRemovals.size() > 0;
    for(auto fEvent : file->Events)
    {
        changeExists |= fEvent->HasUpdates;
    }

    if(changeExists)
    {
        auto reply = QMessageBox::question(this, "Confirm Reload",
            QString("%1 has pending changes that will be lost by reloading."
                " Is this okay?").arg(qs(mCurrentFileName)),
            QMessageBox::Yes | QMessageBox::No);
        if(reply != QMessageBox::Yes)
        {
            return;
        }
    }

    LoadFileFromPath(mCurrentFileName);
}

void EventWindow::SaveFile()
{
    if(mCurrentFileName.IsEmpty())
    {
        // No file, nothing to do
        return;
    }

    std::list<libcomp::String> paths;
    paths.push_back(mCurrentFileName);
    SaveFiles(paths);
}

void EventWindow::SaveAllFiles()
{
    std::list<libcomp::String> paths;
    for(auto& pair : mFiles)
    {
        paths.push_back(pair.first);
    }

    SaveFiles(paths);
}

void EventWindow::NewFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getSaveFileName(this,
        tr("Create new Event file"), mMainWindow->GetDialogDirectory(),
        tr("Event XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

    mMainWindow->SetDialogDirectory(qPath, true);

    QFileInfo fi(qPath);
    if(fi.exists() && fi.isFile())
    {
        LOG_ERROR(libcomp::String("Attempted to overwrite existing file with"
            " new event file: %1").Arg(cs(qPath)));
        return;
    }

    // Save new document with root objects node only
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLElement* pRoot = doc.NewElement("objects");
    doc.InsertEndChild(pRoot);

    doc.SaveFile(cs(qPath).C());

    // Select new file
    if(LoadFileFromPath(cs(qPath)))
    {
        ui->files->setCurrentText(qPath);
        Refresh(false);
    }
}

void EventWindow::RemoveEvent()
{
    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;
    if(mCurrentEvent)
    {
        if(!mCurrentEvent->FileEventID.IsEmpty())
        {
            file->PendingRemovals.insert(mCurrentEvent->FileEventID);
        }

        for(auto it = file->Events.begin(); it != file->Events.end(); it++)
        {
            if(*it == mCurrentEvent)
            {
                file->Events.erase(it);
                break;
            }
        }

        mCurrentEvent = nullptr;

        RebuildLocalIDMap(file);
        RebuildGlobalIDMap();
        EventRef::RefreshAllEventIDs(mMainWindow);
        mMainWindow->ResetEventCount();
        Refresh(false);
    }
}

void EventWindow::Search()
{
    std::set<QTreeWidgetItem*> display;

    QList<QTreeWidgetItem*> items = ui->treeWidget->findItems(
        QString("*"), Qt::MatchWrap | Qt::MatchWildcard | Qt::MatchRecursive);

    QString filter = ui->treeSearch->text();
    if(filter.isEmpty())
    {
        // Display all
        for(auto item : items)
        {
            display.insert(item);
        }
    }
    else
    {
        // Filter on ID and text columns and expand parents
        for(auto item : items)
        {
            if(display.find(item) == display.end() &&
                (item->text(0).contains(filter) ||
                item->text(2).contains(filter)))
            {
                display.insert(item);

                auto parent = item->parent();
                while(parent)
                {
                    display.insert(parent);
                    parent = parent->parent();
                }
            }
        }
    }

    for(auto item : items)
    {
        if(display.find(item) != display.end())
        {
            item->setHidden(false);
            item->setExpanded(true);
        }
        else
        {
            item->setHidden(true);
        }
    }
}

void EventWindow::NewEvent()
{
    QAction *pAction = qobject_cast<QAction*>(sender());
    if(!pAction)
    {
        return;
    }

    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;
    auto eventType = (objects::Event::EventType_t)pAction->data().toUInt();

    libcomp::String eventID = GetNewEventID(file, eventType);
    if(eventID.IsEmpty())
    {
        return;
    }

    // Create and add the event
    auto e = GetNewEvent(eventType);
    e->SetID(eventID);

    file->Events.push_back(std::make_shared<FileEvent>(e, true));

    // Rebuild the global map and update the main window
    RebuildLocalIDMap(file);
    RebuildGlobalIDMap();
    mMainWindow->ResetEventCount();

    // Refresh the file and select the new event
    Refresh(false);
    GoToEvent(eventID);
}

void EventWindow::Refresh(bool reselectEvent)
{
    auto current = mCurrentEvent;

    libcomp::String path = cs(ui->files->currentText());
    if(mFiles.find(path) == mFiles.end())
    {
        // Path is invalid, roll back to last valid one and do nothing
        path = mCurrentFileName;

        bool old = ui->files->blockSignals(true);
        ui->files->setCurrentText(qs(path));
        ui->files->blockSignals(old);

        return;
    }
    else if(mCurrentFileName != path)
    {
        mCurrentFileName = path;
        EventRef::RefreshAllEventIDs(mMainWindow);
    }

    SelectFile(path);

    if(reselectEvent && current)
    {
        GoToEvent(current->Event->GetID());
    }

    if(!ui->treeSearch->text().isEmpty())
    {
        Search();
    }
}

void EventWindow::GoTo()
{
    QString qEventID = QInputDialog::getText(this, "Enter an ID", "Event ID",
        QLineEdit::Normal);
    if(qEventID.isEmpty())
    {
        return;
    }

    GoToEvent(cs(qEventID));
}

void EventWindow::FindAction()
{
    findActionWidget->show();
    findActionWidget->raise();

    findAction->lblError->setText("");
}

void EventWindow::FindNextAction()
{
    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;

    findAction->lblError->setText("");

    auto actionType = (objects::Action::ActionType_t)
        findAction->action->currentData().toInt();

    // Build out the event list, ending with the current event
    std::list<std::shared_ptr<FileEvent>> toEnd;
    std::list<std::shared_ptr<FileEvent>> allEvents;

    auto* currentList = mCurrentEvent ? &toEnd : &allEvents;
    for(auto fEvent : file->Events)
    {
        currentList->push_back(fEvent);

        if(fEvent == mCurrentEvent)
        {
            currentList = &allEvents;
        }
    }

    for(auto fEvent : toEnd)
    {
        allEvents.push_back(fEvent);
    }

    // Go to the first event with the selected action type
    for(auto fEvent : allEvents)
    {
        auto pa = std::dynamic_pointer_cast<objects::EventPerformActions>(
            fEvent->Event);
        if(pa)
        {
            auto actions = pa->GetActions();
            for(auto action : GetAllActions(actions))
            {
                if(action->GetActionType() == actionType)
                {
                    // Match found, go to new event or do nothing if current
                    if(mCurrentEvent == fEvent)
                    {
                        findAction->lblError->setText("Only one event found");
                        return;
                    }
                    else
                    {
                        GoToEvent(pa->GetID());
                        findActionWidget->raise();
                        return;
                    }
                }
            }
        }
    }

    // Nothing found, print error
    findAction->lblError->setText("Action type not found");
}

void EventWindow::Back()
{
    if(mPreviousEventIDs.size() > 0)
    {
        auto previousID = mPreviousEventIDs.back();
        mPreviousEventIDs.pop_back();

        ui->treeWidget->blockSignals(true);
        GoToEvent(previousID);
        ui->treeWidget->blockSignals(false);

        BindSelectedEvent(false);

        if(mPreviousEventIDs.size() == 0)
        {
            ui->eventBack->hide();
        }
        else
        {
            ui->eventBack->setText(QString("Back (%1)")
                .arg(mPreviousEventIDs.size()));
        }
    }
}

void EventWindow::FileViewChanged()
{
    bool flat = ui->actionFileView->isChecked();
    ui->actionMoveUp->setDisabled(!flat);
    ui->actionMoveDown->setDisabled(!flat);
    ui->actionChangeTreeIDs->setDisabled(flat);

    Refresh(false);
}

void EventWindow::CollapseAll()
{
    ui->treeWidget->collapseAll();
}

void EventWindow::ExpandAll()
{
    ui->treeWidget->expandAll();
}

void EventWindow::CurrentEventEdited()
{
    if(!mCurrentEvent || mCurrentEvent->HasUpdates)
    {
        return;
    }

    mCurrentEvent->HasUpdates = true;

    // Update the matching tree node
    QList<QTreeWidgetItem*> items = ui->treeWidget->findItems(
        QString("*"), Qt::MatchWrap | Qt::MatchWildcard | Qt::MatchRecursive);
    for(auto item : items)
    {
        auto treeItem = (EventTreeItem*)item;
        if(treeItem->EventID == mCurrentEvent->FileEventID &&
            cs(treeItem->text(0)) == mCurrentEvent->FileEventID)
        {
            QFont font;
            font.setBold(true);
            font.setItalic(true);
            treeItem->setFont(0, font);
            break;
        }
    }
}

void EventWindow::TreeSelectionChanged()
{
    BindSelectedEvent(true);
}

void EventWindow::MoveUp()
{
    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter == mFiles.end() || !mCurrentEvent)
    {
        // No file or current event
        return;
    }

    auto file = fIter->second;
    if(ObjectList::Move(file->Events, mCurrentEvent, true))
    {
        file->Reordered = true;
        RebuildLocalIDMap(file);
        Refresh(true);
    }
}

void EventWindow::MoveDown()
{
    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter == mFiles.end() || !mCurrentEvent)
    {
        // No file or current event
        return;
    }

    auto file = fIter->second;
    if(ObjectList::Move(file->Events, mCurrentEvent, false))
    {
        file->Reordered = true;
        RebuildLocalIDMap(file);
        Refresh(true);
    }
}

void EventWindow::Reorganize()
{
    auto fIter = mFiles.find(mCurrentFileName);
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;

    std::list<libcomp::String> eventOrder;
    std::set<libcomp::String> seen;
    for(auto fEvent : file->Events)
    {
        auto e = fEvent->Event;

        std::list<libcomp::String> eventSet;
        eventSet.push_back(e->GetID());
        eventSet.push_back(e->GetNext());
        eventSet.push_back(e->GetQueueNext());

        auto prompt = std::dynamic_pointer_cast<objects::EventPrompt>(e);
        if(prompt)
        {
            for(auto c : prompt->GetChoices())
            {
                eventSet.push_back(c->GetNext());
                eventSet.push_back(c->GetQueueNext());

                for(auto b : c->GetBranches())
                {
                    eventSet.push_back(b->GetNext());
                    eventSet.push_back(b->GetQueueNext());
                }
            }
        }

        for(auto b : e->GetBranches())
        {
            eventSet.push_back(b->GetNext());
            eventSet.push_back(b->GetQueueNext());
        }

        for(auto eventID : eventSet)
        {
            if(!eventID.IsEmpty() && seen.find(eventID) == seen.end())
            {
                eventOrder.push_back(eventID);
                seen.insert(eventID);
            }
        }
    }

    std::list<std::shared_ptr<FileEvent>> fEvents;
    std::list<std::shared_ptr<FileEvent>> toEnd;
    for(auto eventID : eventOrder)
    {
        bool added = false;
        for(auto fEvent : file->Events)
        {
            if(fEvent->Event->GetID() == eventID)
            {
                if(added)
                {
                    toEnd.push_back(fEvent);
                }
                else
                {
                    fEvents.push_back(fEvent);
                    added = true;
                }
            }
        }
    }

    file->Events = fEvents;
    for(auto fEvent : toEnd)
    {
        file->Events.push_back(fEvent);
    }

    RebuildLocalIDMap(file);
    file->Reordered = true;
    Refresh(true);
}

void EventWindow::ChangeCurrentEventID()
{
    if(mCurrentEvent)
    {
        ChangeEventID(mCurrentEvent->Event->GetID());
    }
}

void EventWindow::ChangeFileEventIDs()
{
    auto fileIter = mFiles.find(mCurrentFileName);
    if(fileIter == mFiles.end())
    {
        // Nothing to do
        return;
    }

    auto file = fileIter->second;
    if(file->Events.size() == 0)
    {
        // Nothing to do
        return;
    }
    else if(file->Events.size() == 1)
    {
        // Do single rename instead
        ChangeCurrentEventID();
        return;
    }

    auto commonPrefix = GetCommonEventPrefix(file);

    QString qPrefix = QInputDialog::getText(this, "Enter an ID prefix",
        "New ID prefix", QLineEdit::Normal, qs(commonPrefix));
    if(qPrefix.isEmpty())
    {
        return;
    }

    auto prefix = cs(qPrefix);

    std::unordered_map<libcomp::String, libcomp::String> eventIDMap;

    std::unordered_map<int32_t, size_t> sequences;
    for(auto fEvent : file->Events)
    {
        int32_t key = (int32_t)fEvent->Event->GetEventType();
        if(sequences.find(key) == sequences.end())
        {
            sequences[key] = 0;
        }

        auto typePrefix = GetEventTypePrefix(prefix,
            fEvent->Event->GetEventType());

        size_t& seq = sequences[key];
        for(; seq < 999;)
        {
            seq++;

            // Zero pad the number
            auto str = libcomp::String("%1%2").Arg(typePrefix)
                .Arg(libcomp::String("%1").Arg(1000 + seq).Right(3));
            if(mGlobalIDMap.find(str) == mGlobalIDMap.end() ||
                mGlobalIDMap[str] == file->Path)
            {
                eventIDMap[fEvent->Event->GetID()] = str;
                break;
            }
        }

        if(seq > 1000)
        {
            QMessageBox err;
            err.setText("Event sequence exceeded 1000. Please split the file"
                " into fewer events or rename by tree branches.");
            err.exec();

            break;
        }
    }

    auto reply = QMessageBox::question(this, "Confirm Rename",
        QString("%1 event IDs will be changed and all currently loaded event"
            " references will be updated automatically however, no files will"
            " be saved at this time. Only the current zone and loaded zone"
            " partials will be updated. Please confirm this action.")
        .arg(QString::number(eventIDMap.size())),
        QMessageBox::Yes | QMessageBox::No);
    if(reply != QMessageBox::Yes)
    {
        return;
    }

    // Deselect the tree so everything saves
    ui->treeWidget->clearSelection();

    // Update the events
    for(auto fEvent : file->Events)
    {
        fEvent->Event->SetID(eventIDMap[fEvent->Event->GetID()]);
        fEvent->HasUpdates = true;
    }

    ChangeEventIDs(eventIDMap);

    // Rebuild maps and refresh
    RebuildLocalIDMap(file);
    RebuildGlobalIDMap();
    EventRef::RefreshAllEventIDs(mMainWindow);
    Refresh(false);
}

void EventWindow::ChangeTreeBranchIDs()
{
    auto fileIter = mFiles.find(mCurrentFileName);
    if(fileIter == mFiles.end())
    {
        // Nothing to do
        return;
    }

    auto file = fileIter->second;

    std::list<EventTreeItem*> branchNodes;
    for(auto selected : ui->treeWidget->selectedItems())
    {
        auto sNode = (EventTreeItem*)selected;
        if(sNode->ExtensionNode) continue;

        std::list<EventTreeItem*> parents;
        parents.push_back(sNode);
        while(parents.size() > 0)
        {
            auto p = parents.front();
            parents.pop_front();

            branchNodes.push_back(p);

            int childCount = p->childCount();
            for(int i = 0; i < childCount; i++)
            {
                parents.push_back((EventTreeItem*)p->child(i));
            }
        }
    }

    std::list<std::shared_ptr<FileEvent>> renames;
    std::unordered_map<libcomp::String, libcomp::String> eventIDMap;
    for(auto node : branchNodes)
    {
        if(node->FirstFileRef)
        {
            int32_t fileIdx = node->FileIdx;
            if(fileIdx == -1)
            {
                fileIdx = file->EventIDMap[node->EventID];
            }

            auto eIter = file->Events.begin();
            std::advance(eIter, fileIdx);

            auto fEvent = *eIter;
            renames.push_back(fEvent);
            eventIDMap[fEvent->Event->GetID()] = "";
        }
        else if(!node->ExtensionNode)
        {
            QMessageBox err;
            err.setText("The selected tree branch contains at least one event"
                " that is not eligible for branch renaming. Ensure that all"
                " nodes below the current selection are not referenced by"
                " earlier events and are not references to external files.");
            err.exec();
            return;
        }
    }

    if(renames.size() == 0)
    {
        // Nothing to do
        return;
    }
    else if(renames.size() == 1)
    {
        // Do single rename instead
        ChangeCurrentEventID();
        return;
    }

    auto commonPrefix = GetCommonEventPrefix(file);

    QString qPrefix = QInputDialog::getText(this, "Enter an ID prefix",
        "New ID prefix", QLineEdit::Normal, qs(commonPrefix));
    if(qPrefix.isEmpty())
    {
        return;
    }

    auto prefix = cs(qPrefix);

    libcomp::String firstID;
    libcomp::String lastID;

    bool skipped = false;
    size_t seq = 0;
    for(auto fEvent : renames)
    {
        for(; seq < 999;)
        {
            seq++;

            // Zero pad the number
            auto str = libcomp::String("%1%2").Arg(prefix)
                .Arg(libcomp::String("%1").Arg(1000 + seq).Right(3));
            if(mGlobalIDMap.find(str) == mGlobalIDMap.end() ||
                eventIDMap.find(str) != eventIDMap.end())
            {
                lastID = str;

                if(firstID.IsEmpty())
                {
                    firstID = str;
                }

                eventIDMap[fEvent->Event->GetID()] = str;
                break;
            }
            else
            {
                skipped = true;
            }
        }

        if(seq > 1000)
        {
            break;
        }
    }

    if(seq > 1000)
    {
        QMessageBox err;
        err.setText("Event sequence exceeded 1000. Please select fewer"
            " nodes to change.");
        err.exec();

        return;
    }
    else
    {
        auto reply = QMessageBox::question(this, "Confirm Rename",
            QString("%1 event IDs will be changed to '%2' through '%3'%4 and"
                " all currently loaded event references will be updated"
                " automatically however, no files will be saved at this time."
                " Only the current zone and loaded zone partials will be"
                " updated. Please confirm this action.")
            .arg(QString::number(eventIDMap.size()))
            .arg(qs(firstID)).arg(qs(lastID))
            .arg(skipped ? " (with gaps from existing IDs)" : ""),
            QMessageBox::Yes | QMessageBox::No);
        if(reply != QMessageBox::Yes)
        {
            return;
        }

        // Deselect the tree so everything saves
        ui->treeWidget->clearSelection();

        // Update the events
        for(auto fEvent : renames)
        {
            fEvent->Event->SetID(eventIDMap[fEvent->Event->GetID()]);
            fEvent->HasUpdates = true;
        }

        ChangeEventIDs(eventIDMap);

        // Rebuild maps and refresh
        RebuildLocalIDMap(file);
        RebuildGlobalIDMap();
        EventRef::RefreshAllEventIDs(mMainWindow);
        Refresh(false);
    }
}

bool EventWindow::LoadFileFromPath(const libcomp::String& path)
{
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_NO_ERROR != doc.LoadFile(path.C()))
    {
        LOG_ERROR(libcomp::String("Failed to parse file: %1\n").Arg(path));
        return false;
    }
    
    auto rootElem = doc.RootElement();
    if(!rootElem)
    {
        LOG_ERROR(libcomp::String("No root element in file: %1\n").Arg(path));
        return false;
    }

    std::list<std::shared_ptr<objects::Event>> events;
    std::list<std::list<libcomp::String>> commentSets;

    auto objNode = rootElem->FirstChildElement("object");
    while(objNode)
    {
        auto event = objects::Event::InheritedConstruction(objNode
            ->Attribute("name"));
        if(!event || !event->Load(doc, *objNode))
        {
            break;
        }

        if(event->GetID().IsEmpty())
        {
            LOG_ERROR(libcomp::String("Event with no ID encountered in"
                " file: %1\n").Arg(path));
            break;
        }

        events.push_back(event);
        commentSets.push_back(XmlHandler::GetComments(objNode));

        objNode = objNode->NextSiblingElement("object");
    }

    // Add the file if it has events or no child nodes
    if(events.size() > 0 || rootElem->FirstChild() == nullptr)
    {
        if(mFiles.find(path) != mFiles.end())
        {
            LOG_INFO(libcomp::String("Reloaded %1 event(s) from"
                " file: %2\n").Arg(events.size()).Arg(path));
        }
        else
        {
            LOG_INFO(libcomp::String("Loaded %1 event(s) from"
                " file: %2\n").Arg(events.size()).Arg(path));
        }

        auto file = std::make_shared<EventFile>();
        file->Path = path;

        for(auto e : events)
        {
            auto fEvent = std::make_shared<FileEvent>(e);
            fEvent->Comments = commentSets.front();
            file->Events.push_back(fEvent);

            commentSets.pop_front();
        }

        mFiles[path] = file;

        RebuildLocalIDMap(file);

        // Rebuild the context menu
        ui->files->clear();

        std::set<libcomp::String> filenames;
        for(auto& pair : mFiles)
        {
            filenames.insert(pair.first);
        }

        for(auto filename : filenames)
        {
            ui->files->addItem(qs(filename));
        }

        return true;
    }
    else
    {
        LOG_WARNING(libcomp::String("No events found in file: %1\n")
            .Arg(path));
    }

    return false;
}

bool EventWindow::SelectFile(const libcomp::String& path)
{
    auto iter = mFiles.find(path);
    if(iter == mFiles.end())
    {
        return false;
    }

    // Clean up the current tree
    ui->treeWidget->clear();

    // Drop previous events
    mPreviousEventIDs.clear();
    ui->eventBack->hide();

    // Add events to the tree
    auto file = iter->second;

    int32_t fileIdx = 0;
    std::set<libcomp::String> seen;
    std::set<libcomp::String> dupeCheck;
    for(auto lEvent : file->Events)
    {
        auto e = lEvent->Event;
        if(seen.find(e->GetID()) == seen.end())
        {
            AddEventToTree(e->GetID(), nullptr, file, seen);
        }
        else if(dupeCheck.find(e->GetID()) != dupeCheck.end())
        {
            AddEventToTree(e->GetID(), nullptr, file, seen, fileIdx);
        }

        fileIdx++;
        dupeCheck.insert(e->GetID());
    }

    mPreviousEventIDs.clear();
    ui->eventBack->hide();

    ui->treeWidget->expandAll();
    ui->treeWidget->resizeColumnToContents(0);

    findAction->lblError->setText("");

    return true;
}

void EventWindow::SaveFiles(const std::list<libcomp::String>& paths)
{
    // Update the current event if we haven't already
    if(mCurrentEvent && mCurrentEvent->HasUpdates)
    {
        for(auto eCtrl : ui->splitter->findChildren<Event*>())
        {
            if(mCurrentEvent->Event == eCtrl->Save())
            {
                mCurrentEvent->Comments = eCtrl->GetComments();
            }
        }
    }

    for(auto path : paths)
    {
        std::list<std::shared_ptr<FileEvent>> updates;

        // Make sure we don't have dupes
        bool hasDupe = false;
        std::set<libcomp::String> allEventIDs;

        auto file = mFiles[path];
        for(auto fEvent : file->Events)
        {
            if(fEvent->HasUpdates)
            {
                updates.push_back(fEvent);
            }

            if(allEventIDs.find(fEvent->Event->GetID()) == allEventIDs.end())
            {
                allEventIDs.insert(fEvent->Event->GetID());
            }
            else
            {
                hasDupe = true;
            }
        }

        if(updates.size() == 0 && file->PendingRemovals.size() == 0 &&
            !file->Reordered)
        {
            // Nothing to save
            continue;
        }

        if(hasDupe)
        {
            QMessageBox err;
            err.setText(QString("File '%1' cannot be saved because it has at"
                " least one duplicate event ID.").arg(qs(path)));
            err.exec();

            continue;
        }

        tinyxml2::XMLDocument doc;
        if(tinyxml2::XML_NO_ERROR != doc.LoadFile(path.C()))
        {
            LOG_ERROR(libcomp::String("Failed to parse file for saving: %1\n")
                .Arg(path));
            continue;
        }

        std::unordered_map<libcomp::String, tinyxml2::XMLNode*> existingEvents;

        auto rootElem = doc.RootElement();
        if(!rootElem)
        {
            // If for whatever reason we don't have a root element, create
            // one now
            rootElem = doc.NewElement("objects");
            doc.InsertEndChild(rootElem);
        }
        else
        {
            // Load all existing events for replacement
            auto child = rootElem->FirstChild();
            while(child != 0)
            {
                auto member = child->FirstChildElement("member");
                while(member != 0)
                {
                    libcomp::String memberName(member->Attribute("name"));
                    if(memberName == "ID")
                    {
                        auto txtChild = member->FirstChild();
                        auto txt = txtChild ? txtChild->ToText() : 0;
                        if(txt && existingEvents.find(txt->Value()) ==
                            existingEvents.end())
                        {
                            existingEvents[txt->Value()] = child;
                        }
                        break;
                    }

                    member = member->NextSiblingElement("member");
                }

                child = child->NextSibling();
            }
        }

        // Remove events first
        for(auto eventID : file->PendingRemovals)
        {
            auto iter = existingEvents.find(eventID);
            if(iter != existingEvents.end())
            {
                rootElem->DeleteChild(iter->second);
            }
        }

        file->PendingRemovals.clear();

        // Now handle updates
        std::list<tinyxml2::XMLNode*> updatedNodes;
        for(auto fEvent : file->Events)
        {
            if(!fEvent->HasUpdates) continue;

            // Append event to the existing file
            auto e = fEvent->Event;
            e->Save(doc, *rootElem);

            tinyxml2::XMLNode* eNode = rootElem->LastChild();
            if(fEvent->Comments.size())
            {
                tinyxml2::XMLNode* commentNode = 0;
                for(auto comment : fEvent->Comments)
                {
                    auto cNode = doc.NewComment(libcomp::String(" %1 ")
                        .Arg(comment).C());

                    if(commentNode)
                    {
                        eNode->InsertAfterChild(commentNode, cNode);
                    }
                    else
                    {
                        eNode->InsertFirstChild(cNode);
                    }

                    commentNode = cNode;
                }
            }

            if(!fEvent->FileEventID.IsEmpty())
            {
                // If the event already existed in the file, move it to the
                // same location and drop the old one
                auto iter = existingEvents.find(fEvent->FileEventID);
                if(iter != existingEvents.end())
                {
                    if(iter->second->NextSibling() != eNode)
                    {
                        rootElem->InsertAfterChild(iter->second, eNode);
                    }

                    rootElem->DeleteChild(iter->second);
                    existingEvents[fEvent->FileEventID] = eNode;
                }
            }

            updatedNodes.push_back(eNode);

            fEvent->HasUpdates = false;
            fEvent->FileEventID = e->GetID();
        }

        // Reorder if flagged
        if(file->Reordered)
        {
            // Regather events by ID
            existingEvents.clear();

            auto child = rootElem->FirstChild();
            while(child != 0)
            {
                auto member = child->FirstChildElement("member");
                while(member != 0)
                {
                    libcomp::String memberName(member->Attribute("name"));
                    if(memberName == "ID")
                    {
                        auto txtChild = member->FirstChild();
                        auto txt = txtChild ? txtChild->ToText() : 0;
                        if(txt)
                        {
                            existingEvents[txt->Value()] = child;
                        }
                        break;
                    }

                    member = member->NextSiblingElement("member");
                }

                child = child->NextSibling();
            }

            // Now reorganize
            tinyxml2::XMLNode* last = 0;
            for(auto fEvent : file->Events)
            {
                auto id = fEvent->Event->GetID();

                auto iter = existingEvents.find(id);
                if(iter == existingEvents.end()) continue;

                child = existingEvents[id];
                existingEvents.erase(id);

                if(last == 0)
                {
                    if(child->PreviousSiblingElement("object") != 0)
                    {
                        // Move first event to the top
                        rootElem->InsertFirstChild(child);
                    }
                }
                else if(last->NextSiblingElement() != child->ToElement())
                {
                    rootElem->InsertAfterChild(last, child);
                }

                last = child;
            }

            file->Reordered = false;
        }

        if(updatedNodes.size() > 0)
        {
            XmlHandler::SimplifyObjects(updatedNodes);
        }

        doc.SaveFile(path.C());

        LOG_DEBUG(libcomp::String("Updated event file '%1'\n").Arg(path));
    }

    RebuildGlobalIDMap();
    Refresh(true);
}

std::shared_ptr<objects::Event> EventWindow::GetNewEvent(
    objects::Event::EventType_t type) const
{
    switch(type)
    {
    case objects::Event::EventType_t::NPC_MESSAGE:
        return std::make_shared<objects::EventNPCMessage>();
    case objects::Event::EventType_t::EX_NPC_MESSAGE:
        return std::make_shared<objects::EventExNPCMessage>();
    case objects::Event::EventType_t::MULTITALK:
        return std::make_shared<objects::EventMultitalk>();
    case objects::Event::EventType_t::PROMPT:
        return std::make_shared<objects::EventPrompt>();
    case objects::Event::EventType_t::PERFORM_ACTIONS:
        return std::make_shared<objects::EventPerformActions>();
    case objects::Event::EventType_t::OPEN_MENU:
        return std::make_shared<objects::EventOpenMenu>();
    case objects::Event::EventType_t::PLAY_SCENE:
        return std::make_shared<objects::EventPlayScene>();
    case objects::Event::EventType_t::DIRECTION:
        return std::make_shared<objects::EventDirection>();
    case objects::Event::EventType_t::ITIME:
        return std::make_shared<objects::EventITime>();
    case objects::Event::EventType_t::FORK:
    default:
        return std::make_shared<objects::Event>();
    }
}

void EventWindow::BindSelectedEvent(bool storePrevious)
{
    auto previousEvent = mCurrentEvent;
    mCurrentEvent = nullptr;

    EventTreeItem* selected = 0;
    for(auto node : ui->treeWidget->selectedItems())
    {
        selected = (EventTreeItem*)node;
        break;
    }

    std::shared_ptr<EventFile> file;
    if(selected)
    {
        auto iter = mFiles.find(mCurrentFileName);
        if(iter != mFiles.end())
        {
            file = iter->second;
        }
    }

    QWidget* eNode = 0;
    bool editListen = false;

    // Find the event
    int32_t fileIdx = selected ? selected->FileIdx : -1;
    if(fileIdx == -1 && selected)
    {
        if(file && file->EventIDMap.find(selected->EventID) !=
            file->EventIDMap.end())
        {
            fileIdx = file->EventIDMap[selected->EventID];
        }
        else
        {
            // See if it's in a different file
            auto fIter = mGlobalIDMap.find(selected->EventID);
            if(fIter != mGlobalIDMap.end())
            {
                // Just add a manual link to it
                auto ref = new EventRef;
                ref->SetMainWindow(mMainWindow);
                ref->SetEvent(selected->EventID);

                auto line = ref->findChild<QLineEdit*>();
                if(line)
                {
                    line->setDisabled(true);
                }

                eNode = ref;
            }
        }
    }

    if(!eNode)
    {
        if(fileIdx != -1 && file && file->Events.size() >
            (std::list<std::shared_ptr<FileEvent>>::size_type)fileIdx)
        {
            auto eIter2 = file->Events.begin();
            std::advance(eIter2, fileIdx);
            mCurrentEvent = *eIter2;
            editListen = !mCurrentEvent->HasUpdates;
        }

        if(mCurrentEvent)
        {
            switch(mCurrentEvent->Event->GetEventType())
            {
            case objects::Event::EventType_t::FORK:
                {
                    auto eUI = new Event(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::NPC_MESSAGE:
                {
                    auto eUI = new EventNPCMessage(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::EX_NPC_MESSAGE:
                {
                    auto eUI = new EventExNPCMessage(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::MULTITALK:
                {
                    auto eUI = new EventMultitalk(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::PROMPT:
                {
                    auto eUI = new EventPrompt(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::PERFORM_ACTIONS:
                {
                    auto eUI = new EventPerformActions(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::OPEN_MENU:
                {
                    auto eUI = new EventOpenMenu(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::PLAY_SCENE:
                {
                    auto eUI = new EventPlayScene(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::DIRECTION:
                {
                    auto eUI = new EventDirection(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::ITIME:
                {
                    auto eUI = new EventITime(mMainWindow);
                    eUI->Load(mCurrentEvent->Event);

                    eNode = eUI;
                }
                break;
            default:
                break;
            }

            if(eNode)
            {
                ((Event*)eNode)->SetComments(mCurrentEvent->Comments);
            }
        }
    }

    if(previousEvent)
    {
        // If the previous current event was updated, update the event
        // definition from the current control (should only be one)
        if(previousEvent->HasUpdates)
        {
            for(auto eCtrl : ui->splitter->findChildren<Event*>())
            {
                mMainWindow->CloseSelectors(eCtrl);
                if(previousEvent->Event == eCtrl->Save())
                {
                    previousEvent->Comments = eCtrl->GetComments();
                }
            }
        }

        if(storePrevious)
        {
            UpdatePreviousEvents(previousEvent->Event->GetID());
        }
    }

    // Clear any existing controls (should be only one)
    while(ui->layoutView->count() >= 3)
    {
        auto current = ui->layoutView->itemAt(1)->widget();
        ui->layoutView->removeWidget(current);
        current->deleteLater();
    }

    if(eNode)
    {
        ui->lblNoCurrent->hide();

        if(editListen)
        {
            BindEventEditControls(eNode);
        }

        ui->layoutView->insertWidget(1, eNode);

        ui->removeEvent->show();
        ui->actionMoveUp->setDisabled(false);
        ui->actionMoveDown->setDisabled(false);
    }
    else
    {
        ui->lblNoCurrent->show();

        ui->removeEvent->hide();
        ui->actionMoveUp->setDisabled(true);
        ui->actionMoveDown->setDisabled(true);
    }
}

void EventWindow::BindEventEditControls(QWidget* eNode)
{
    // Wire all sub controls to the edit event. This only needs to execute
    // at most once per binding so controls created after this point do not
    // need to be re-bound
    for(auto ctrl : eNode->findChildren<ActionList*>())
    {
        connect(ctrl, SIGNAL(rowEdit()), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<ActionMap*>())
    {
        connect(ctrl, SIGNAL(rowEdit()), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<DynamicList*>())
    {
        connect(ctrl, SIGNAL(rowEdit()), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<QCheckBox*>())
    {
        connect(ctrl, SIGNAL(toggled(bool)), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<QComboBox*>())
    {
        connect(ctrl, SIGNAL(currentIndexChanged(const QString&)), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<QLineEdit*>())
    {
        connect(ctrl, SIGNAL(textChanged(const QString&)), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<QTextEdit*>())
    {
        connect(ctrl, SIGNAL(textChanged()), this, SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<QDoubleSpinBox*>())
    {
        connect(ctrl, SIGNAL(valueChanged(double)), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<QRadioButton*>())
    {
        connect(ctrl, SIGNAL(toggled(bool)), this,
            SLOT(CurrentEventEdited()));
    }

    for(auto ctrl : eNode->findChildren<QSpinBox*>())
    {
        connect(ctrl, SIGNAL(valueChanged(int)), this,
            SLOT(CurrentEventEdited()));
    }
}

void EventWindow::AddEventToTree(const libcomp::String& id,
    EventTreeItem* parent, const std::shared_ptr<EventFile>& file,
    std::set<libcomp::String>& seen, int32_t eventIdx)
{
    if(id.IsEmpty())
    {
        return;
    }

    if(ui->actionFileView->isChecked() && parent)
    {
        return;
    }

    EventTreeItem* item = 0;
    std::shared_ptr<objects::Event> e;
    if(eventIdx == -1)
    {
        // Adding normal node
        if(seen.find(id) != seen.end())
        {
            // Add as "goto"
            item = new EventTreeItem(parent, id);

            item->setText(0, qs(libcomp::String("Go to: %1").Arg(id)));
            item->setText(1, "Reference");
        }
        else if(file->EventIDMap.find(id) == file->EventIDMap.end())
        {
            // Not in the file
            item = new EventTreeItem(parent, id);

            item->setText(0, qs(id));

            auto it = mGlobalIDMap.find(id);
            if(it != mGlobalIDMap.end())
            {
                item->setText(1, qs(libcomp::String("External Reference to %1")
                    .Arg(it->second)));
            }
            else
            {
                item->setText(1, "Event not found");
                item->setTextColor(1, QColor(255, 0, 0));
            }
        }

        if(item)
        {
            if(!parent)
            {
                ui->treeWidget->addTopLevelItem(item);
            }

            return;
        }

        auto eIter = file->Events.begin();
        std::advance(eIter, file->EventIDMap[id]);

        e = (*eIter)->Event;
        item = new EventTreeItem(parent, id);
        item->FirstFileRef = true;

        item->setText(0, qs(id));

        if((*eIter)->HasUpdates)
        {
            QFont font;
            font.setBold(true);
            font.setItalic(true);
            item->setFont(0, font);
        }
    }
    else
    {
        auto eIter = file->Events.begin();
        std::advance(eIter, eventIdx);

        e = (*eIter)->Event;
        item = new EventTreeItem(parent, "", eventIdx);
        item->FirstFileRef = true;

        item->setText(0, qs(libcomp::String("%1 [Duplicate]").Arg(id)));
        item->setTextColor(0, QColor(255, 0, 0));

        if((*eIter)->HasUpdates)
        {
            QFont font;
            font.setBold(true);
            font.setItalic(true);
            item->setFont(0, font);
        }
    }

    seen.insert(id);

    if(!parent)
    {
        ui->treeWidget->addTopLevelItem(item);
    }

    AddEventToTree(e->GetNext(), item, file, seen);
    AddEventToTree(e->GetQueueNext(), item, file, seen);

    switch(e->GetEventType())
    {
    case objects::Event::EventType_t::FORK:
        item->setText(1, "Fork");
        break;
    case objects::Event::EventType_t::NPC_MESSAGE:
        {
            auto msg = std::dynamic_pointer_cast<
                objects::EventNPCMessage>(e);

            auto cMessage = mMainWindow->GetEventMessage(msg
                ->GetMessageIDs(0));
            libcomp::String moreTxt = msg->MessageIDsCount() > 1
                ? libcomp::String(" [+%1 More]")
                .Arg(msg->MessageIDsCount() - 1) : "";
            item->setText(1, "NPC Message");
            item->setText(2, cMessage ? qs(GetInlineMessageText(
                libcomp::String::Join(cMessage->GetLines(), "  ")) +
                moreTxt) : "");
        }
        break;
    case objects::Event::EventType_t::EX_NPC_MESSAGE:
        {
            auto msg = std::dynamic_pointer_cast<
                objects::EventExNPCMessage>(e);

            auto cMessage = mMainWindow->GetEventMessage(msg
                ->GetMessageID());
            item->setText(1, "EX NPC Message");
            item->setText(2, cMessage ? qs(GetInlineMessageText(
                libcomp::String::Join(cMessage->GetLines(), "  "))) : "");
        }
        break;
    case objects::Event::EventType_t::MULTITALK:
        {
            item->setText(1, "Multitalk");
        }
        break;
    case objects::Event::EventType_t::PROMPT:
    case objects::Event::EventType_t::ITIME:
        {
            auto prompt = std::dynamic_pointer_cast<
                objects::EventPrompt>(e);

            std::unordered_map<EventTreeItem*, int32_t> messageNodes;
            messageNodes[item] = prompt->GetMessageID();

            bool iTime = e->GetEventType() ==
                objects::Event::EventType_t::ITIME;
            if(iTime)
            {
                item->setText(1, "I-Time");
            }
            else
            {
                item->setText(1, "Prompt");
            }

            if(!ui->actionFileView->isChecked())
            {
                for(size_t i = 0; i < prompt->ChoicesCount(); i++)
                {
                    auto choice = prompt->GetChoices(i);

                    EventTreeItem* cNode = new EventTreeItem(item, id);
                    cNode->ExtensionNode = true;

                    cNode->setText(0, qs(libcomp::String("[%1]").Arg(i + 1)));
                    cNode->setText(1, iTime
                        ? "I-Time Choice" : "Prompt Choice");

                    messageNodes[cNode] = choice->GetMessageID();

                    // Add regardless of next results
                    AddEventToTree(choice->GetNext(), cNode, file, seen);
                    AddEventToTree(choice->GetQueueNext(), cNode, file, seen);

                    if(choice->BranchesCount() > 0)
                    {
                        EventTreeItem* bNode = new EventTreeItem(cNode, id);
                        bNode->ExtensionNode = true;

                        bNode->setText(0, "[Branches]");

                        for(auto b : choice->GetBranches())
                        {
                            AddEventToTree(b->GetNext(), bNode, file, seen);
                            AddEventToTree(b->GetQueueNext(), bNode, file,
                                seen);
                        }
                    }
                }
            }

            if(iTime)
            {
                auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
                    mMainWindow->GetBinaryDataSet("CHouraiMessageData"));
                for(auto& pair : messageNodes)
                {
                    auto txt = dataset->GetName(dataset
                        ->GetObjectByID((uint32_t)pair.second));
                    if(txt.Length() > 0)
                    {
                        pair.first->setText(2, qs(GetInlineMessageText(txt)));
                    }
                }
            }
            else
            {
                for(auto& pair : messageNodes)
                {
                    auto cMessage = mMainWindow->GetEventMessage(pair.second);
                    if(cMessage)
                    {
                        pair.first->setText(2, qs(GetInlineMessageText(
                            libcomp::String::Join(cMessage->GetLines(),
                                "  "))));
                    }
                }
            }
        }
        break;
    case objects::Event::EventType_t::PERFORM_ACTIONS:
        {
            auto pa = std::dynamic_pointer_cast<
                objects::EventPerformActions>(e);

            item->setText(1, "Perform Actions");

            // Print any messages to the line in order
            std::list<int32_t> messageIDs;
            for(auto action : pa->GetActions())
            {
                switch(action->GetActionType())
                {
                case objects::Action::ActionType_t::DISPLAY_MESSAGE:
                    {
                        auto act = std::dynamic_pointer_cast<
                            objects::ActionDisplayMessage>(action);
                        for(int32_t messageID : act->GetMessageIDs())
                        {
                            messageIDs.push_back(messageID);
                        }
                    }
                    break;
                case objects::Action::ActionType_t::STAGE_EFFECT:
                    {
                        auto act = std::dynamic_pointer_cast<
                            objects::ActionStageEffect>(action);
                        messageIDs.push_back(act->GetMessageID());
                    }
                    break;
                default:
                    break;
                }
            }

            if(messageIDs.size() > 0)
            {
                std::list<libcomp::String> messages;
                for(int32_t messageID : messageIDs)
                {
                    auto cMessage = mMainWindow->GetEventMessage(messageID);
                    if(cMessage && cMessage->LinesCount() > 0)
                    {
                        messages.push_back(GetInlineMessageText(
                            libcomp::String::Join(cMessage->GetLines(),
                                "  ")));
                    }
                }

                item->setText(2, qs(libcomp::String::Join(messages, " => ")));
            }
        }
        break;
    case objects::Event::EventType_t::OPEN_MENU:
        {
            item->setText(1, "Open Menu");
        }
        break;
    case objects::Event::EventType_t::PLAY_SCENE:
        {
            item->setText(1, "Play Scene");
        }
        break;
    case objects::Event::EventType_t::DIRECTION:
        {
            item->setText(1, "Direction");
        }
        break;
    default:
        break;
    }

    if(e->BranchesCount() > 0 && !ui->actionFileView->isChecked())
    {
        // Add under branches child node
        EventTreeItem* bNode = new EventTreeItem(item, id);
        bNode->ExtensionNode = true;

        bNode->setText(0, "[Branches]");

        for(auto b : e->GetBranches())
        {
            AddEventToTree(b->GetNext(), bNode, file, seen);
            AddEventToTree(b->GetQueueNext(), bNode, file, seen);
        }
    }
}

void EventWindow::ChangeEventIDs(const std::unordered_map<libcomp::String,
    libcomp::String>& idMap)
{
    // Update all loaded events and actions within them
    for(auto fPair : mFiles)
    {
        for(auto f : fPair.second->Events)
        {
            bool update = false;

            auto e = f->Event;

            // Pull base class casts for whatever we can since many fields
            // are shared between sections
            std::list<std::shared_ptr<objects::EventBase>> baseParts;
            baseParts.push_back(e);

            for(auto b : e->GetBranches())
            {
                baseParts.push_back(b);
            }

            switch(e->GetEventType())
            {
            case objects::Event::EventType_t::ITIME:
                {
                    auto iTime = std::dynamic_pointer_cast<
                        objects::EventITime>(e);
                    auto iter = idMap.find(iTime->GetStartActions());
                    if(iter != idMap.end())
                    {
                        iTime->SetStartActions(iter->second);
                        update = true;
                    }
                }
                break;
            case objects::Event::EventType_t::PERFORM_ACTIONS:
                {
                    auto pa = std::dynamic_pointer_cast<
                        objects::EventPerformActions>(e);

                    auto actions = pa->GetActions();
                    update |= ChangeActionEventIDs(idMap, actions);
                }
                break;
            case objects::Event::EventType_t::PROMPT:
                {
                    auto prompt = std::dynamic_pointer_cast<
                        objects::EventPrompt>(e);
                    for(auto choice : prompt->GetChoices())
                    {
                        baseParts.push_back(choice);
                        for(auto b : choice->GetBranches())
                        {
                            baseParts.push_back(b);
                        }
                    }
                }
                break;
            default:
                break;
            }

            for(auto eBase : baseParts)
            {
                auto iter = idMap.find(eBase->GetNext());
                if(iter != idMap.end())
                {
                    eBase->SetNext(iter->second);
                    update = true;
                }

                iter = idMap.find(eBase->GetQueueNext());
                if(iter != idMap.end())
                {
                    eBase->SetQueueNext(iter->second);
                    update = true;
                }
            }

            if(update)
            {
                f->HasUpdates = true;
            }
        }
    }

    // Update all loaded zone and partial actions
    auto actions = mMainWindow->GetZones()->GetLoadedActions(true);
    ChangeActionEventIDs(idMap, actions);
}

bool EventWindow::ChangeActionEventIDs(const std::unordered_map<
    libcomp::String, libcomp::String>& idMap,
    const std::list<std::shared_ptr<objects::Action>>& actions)
{
    bool updated = false;
    for(auto action : GetAllActions(actions))
    {
        switch(action->GetActionType())
        {
        case objects::Action::ActionType_t::START_EVENT:
            {
                auto act = std::dynamic_pointer_cast<
                    objects::ActionStartEvent>(action);
                auto iter = idMap.find(act->GetEventID());
                if(iter != idMap.end())
                {
                    act->SetEventID(iter->second);
                    updated = true;
                }
            }
            break;
        case objects::Action::ActionType_t::ZONE_INSTANCE:
            {
                auto act = std::dynamic_pointer_cast<
                    objects::ActionZoneInstance>(action);
                auto iter = idMap.find(act->GetTimerExpirationEventID());
                if(iter != idMap.end())
                {
                    act->SetTimerExpirationEventID(iter->second);
                    updated = true;
                }
            }
            break;
        default:
            break;
        }
    }

    return updated;
}

std::list<std::shared_ptr<objects::Action>> EventWindow::GetAllActions(
    const std::list<std::shared_ptr<objects::Action>>& actions)
{
    std::list<std::shared_ptr<objects::Action>> allActions;

    auto currentActions = actions;

    std::list<std::shared_ptr<objects::Action>> newActions;
    while(currentActions.size() > 0)
    {
        // Actions can't nest forever so loop until we're done
        for(auto action : currentActions)
        {
            allActions.push_back(action);

            switch(action->GetActionType())
            {
            case objects::Action::ActionType_t::DELAY:
                {
                    auto act = std::dynamic_pointer_cast<
                        objects::ActionDelay>(action);
                    for(auto act2 : act->GetActions())
                    {
                        newActions.push_back(act2);
                    }
                }
                break;
            case objects::Action::ActionType_t::SPAWN:
                {
                    auto act = std::dynamic_pointer_cast<
                        objects::ActionSpawn>(action);
                    for(auto act2 : act->GetDefeatActions())
                    {
                        newActions.push_back(act2);
                    }
                }
                break;
            default:
                break;
            }
        }

        currentActions = newActions;
        newActions.clear();
    }

    return allActions;
}

libcomp::String EventWindow::GetCommonEventPrefix(
    const std::shared_ptr<EventFile>& file)
{
    libcomp::String commonPrefix;

    auto eIter = file->Events.begin();
    if(eIter != file->Events.end())
    {
        commonPrefix = (*eIter)->Event->GetID();
        eIter++;
    }

    for(; eIter != file->Events.end(); eIter++)
    {
        auto id = (*eIter)->Event->GetID();
        while(commonPrefix.Length() > 0 &&
            commonPrefix != id.Left(commonPrefix.Length()))
        {
            commonPrefix = commonPrefix.Left((size_t)(
                commonPrefix.Length() - 1));
        }

        if(commonPrefix.Length() == 0)
        {
            // No common prefix
            break;
        }
    }

    return commonPrefix;
}

libcomp::String EventWindow::GetEventTypePrefix(const libcomp::String& prefix,
    objects::Event::EventType_t eventType)
{
    libcomp::String newPrefix = prefix;

    // Add type abbreviation and increase number until new ID is found
    if(newPrefix.Right(1) == "_")
    {
        // Remove double underscore
        newPrefix = newPrefix.Left((size_t)(newPrefix.Length() - 1));
    }

    switch(eventType)
    {
    case objects::Event::EventType_t::NPC_MESSAGE:
        newPrefix += "_NM";
        break;
    case objects::Event::EventType_t::EX_NPC_MESSAGE:
        newPrefix += "_EX";
        break;
    case objects::Event::EventType_t::MULTITALK:
        newPrefix += "_ML";
        break;
    case objects::Event::EventType_t::PROMPT:
        newPrefix += "_PR";
        break;
    case objects::Event::EventType_t::PERFORM_ACTIONS:
        newPrefix += "_PA";
        break;
    case objects::Event::EventType_t::OPEN_MENU:
        newPrefix += "_ME";
        break;
    case objects::Event::EventType_t::PLAY_SCENE:
        newPrefix += "_SC";
        break;
    case objects::Event::EventType_t::DIRECTION:
        newPrefix += "_DR";
        break;
    case objects::Event::EventType_t::ITIME:
        newPrefix += "_IT";
        break;
    case objects::Event::EventType_t::FORK:
    default:
        newPrefix += "_";
        break;
    }

    return newPrefix;
}

libcomp::String EventWindow::GetNewEventID(
    const std::shared_ptr<EventFile>& file,
    objects::Event::EventType_t eventType)
{
    // Suggest an ID that is not already taken based off current IDs in
    // the file and cross checked against other loaded files
    auto commonPrefix = GetCommonEventPrefix(file);
    if(commonPrefix.Length() > 0)
    {
        commonPrefix = GetEventTypePrefix(commonPrefix, eventType);
    }

    libcomp::String suggestedID(commonPrefix);
    if(suggestedID.Length() > 0)
    {
        // Add sequence number to the event and make sure its not already
        // taken
        bool validFound = false;
        for(size_t i = 1; i < 1000; i++)
        {
            // Zero pad the number
            auto str = libcomp::String("%1%2").Arg(suggestedID)
                .Arg(libcomp::String("%1").Arg(1000 + i).Right(3));
            if(mGlobalIDMap.find(str) == mGlobalIDMap.end())
            {
                suggestedID = str;
                validFound = true;
                break;
            }
        }

        if(!validFound)
        {
            // No suggested ID
            suggestedID.Clear();
        }
    }

    libcomp::String eventID;
    while(true)
    {
        QString qEventID = QInputDialog::getText(this, "Enter an ID", "New ID",
            QLineEdit::Normal, qs(suggestedID));
        if(qEventID.isEmpty())
        {
            return "";
        }

        eventID = cs(qEventID);

        auto globalIter = mGlobalIDMap.find(eventID);
        if(globalIter != mGlobalIDMap.end())
        {
            QMessageBox err;
            err.setText(qs(libcomp::String("Event ID '%1' already exists in"
                " file: %2").Arg(eventID).Arg(globalIter->second)));
            err.exec();
        }
        else
        {
            break;
        }
    }

    return eventID;
}

void EventWindow::UpdatePreviousEvents(const libcomp::String& last)
{
    auto oldList = mPreviousEventIDs;

    if(oldList.size() >= 10)
    {
        oldList.pop_front();
    }

    mPreviousEventIDs.clear();
    for(auto eventID : oldList)
    {
        if(eventID != last)
        {
            mPreviousEventIDs.push_back(eventID);
        }
    }

    mPreviousEventIDs.push_back(last);

    ui->eventBack->setText(QString("Back (%1)")
        .arg(mPreviousEventIDs.size()));
    ui->eventBack->show();
}

void EventWindow::RebuildLocalIDMap(const std::shared_ptr<EventFile>& file)
{
    file->EventIDMap.clear();

    int32_t idx = 0;
    for(auto fEvent : file->Events)
    {
        auto e = fEvent->Event;
        if(file->EventIDMap.find(e->GetID()) == file->EventIDMap.end())
        {
            // Don't add it twice
            file->EventIDMap[e->GetID()] = idx;
        }

        idx++;
    }
}

void EventWindow::RebuildGlobalIDMap()
{
    mGlobalIDMap.clear();

    for(auto& pair : mFiles)
    {
        auto file = pair.second;
        for(auto& pair2 : file->EventIDMap)
        {
            auto eventID = pair2.first;
            if(mGlobalIDMap.find(eventID) == mGlobalIDMap.end())
            {
                mGlobalIDMap[eventID] = file->Path;
            }
        }
    }
}

libcomp::String EventWindow::GetInlineMessageText(const libcomp::String& raw,
    size_t limit)
{
    libcomp::String txt = raw.Replace("\n", "  ").Replace("\r", "  ");
    if(limit && txt.Length() > limit)
    {
        return txt.Left(limit) + "...";
    }
    else
    {
        return txt;
    }
}
