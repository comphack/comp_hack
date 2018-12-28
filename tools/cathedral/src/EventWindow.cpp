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
#include "ActionList.h"
#include "ActionMap.h"
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
#include <QSpinBox>
#include <PopIgnore.h>

// object Includes
#include <Event.h>
#include <EventBase.h>
#include <EventChoice.h>
#include <EventCondition.h>
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
    bool HasUpdates;
};

class EventFile
{
public:
    libcomp::String Path;
    std::list<std::shared_ptr<FileEvent>> Events;
    std::unordered_map<libcomp::String, int32_t> EventIDMap;
    std::set<libcomp::String> PendingRemovals;
};

EventWindow::EventWindow(MainWindow *pMainWindow, QWidget *pParent) :
    QMainWindow(pParent), mMainWindow(pMainWindow)
{
    ui = new Ui::EventWindow;
    ui->setupUi(this);

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

    ui->removeEvent->hide();

    connect(ui->actionLoadFile, SIGNAL(triggered()), this, SLOT(LoadFile()));
    connect(ui->actionLoadDirectory, SIGNAL(triggered()), this,
        SLOT(LoadDirectory()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(SaveFile()));
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(NewFile()));
    connect(ui->actionRefresh, SIGNAL(triggered()), this, SLOT(Refresh()));
    connect(ui->removeEvent, SIGNAL(clicked()), this, SLOT(RemoveEvent()));
    connect(ui->files, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(FileSelectionChanged()));
    connect(ui->treeWidget, SIGNAL(itemSelectionChanged()), this,
        SLOT(TreeSelectionChanged()));
}

EventWindow::~EventWindow()
{
    delete ui;
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

    libcomp::String currentPath = cs(ui->files->currentText());
    libcomp::String path = iter->second;

    if(currentPath != path)
    {
        // Switch current file
        ui->files->setCurrentText(qs(path));
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
                ui->treeWidget->blockSignals(true);
                ui->treeWidget->clearSelection();
                ui->treeWidget->blockSignals(false);

                // Select new item and display (if not already)
                ui->treeWidget->setItemSelected(treeItem, true);
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

void EventWindow::FileSelectionChanged()
{
    Refresh();
}

void EventWindow::LoadDirectory()
{
    QSettings settings;

    QString qPath = QFileDialog::getExistingDirectory(this,
        tr("Load Event XML folder"), settings.value("datastore").toString());
    if(qPath.isEmpty())
    {
        return;
    }

    ui->files->blockSignals(true);

    QDirIterator it(qPath, QStringList() << "*.xml", QDir::Files,
        QDirIterator::Subdirectories);
    libcomp::String currentPath(ui->files->currentText().toUtf8()
        .constData());
    libcomp::String selectPath = currentPath;
    while(it.hasNext())
    {
        libcomp::String path = cs(it.next());
        if(LoadFileFromPath(path) && selectPath.IsEmpty())
        {
            selectPath = path;
        }
    }

    ui->files->blockSignals(false);

    RebuildGlobalIDMap();
    mMainWindow->ResetEventCount();

    // Refresh selection even if it didnt change
    Refresh();
}

void EventWindow::LoadFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getOpenFileName(this, tr("Load Event XML"),
        settings.value("datastore").toString(), tr("Event XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

    ui->files->blockSignals(true);

    libcomp::String path = cs(qPath);
    if(LoadFileFromPath(path))
    {
        RebuildGlobalIDMap();
        mMainWindow->ResetEventCount();

        ui->files->blockSignals(false);

        if(ui->files->currentText() != qs(path))
        {
            ui->files->setCurrentText(qs(path));
        }
        else
        {
            // Just refresh
            Refresh();
        }
    }
    else
    {
        ui->files->blockSignals(false);
    }
}

void EventWindow::SaveFile()
{
    libcomp::String path = cs(ui->files->currentText());
    if(path.IsEmpty())
    {
        // No file, nothing to do
        return;
    }

    std::list<std::shared_ptr<FileEvent>> updates;

    auto file = mFiles[path];
    for(auto fEvent : file->Events)
    {
        if(fEvent->HasUpdates)
        {
            updates.push_back(fEvent);
        }
    }

    if(updates.size() == 0 && file->PendingRemovals.size() == 0)
    {
        // Nothing to save
        return;
    }

    // Update the current event if we haven't already
    if(mCurrentEvent && mCurrentEvent->HasUpdates)
    {
        for(auto eCtrl : ui->splitter->findChildren<Event*>())
        {
            eCtrl->Save();
        }
    }

    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_NO_ERROR != doc.LoadFile(path.C()))
    {
        LOG_ERROR(libcomp::String("Failed to parse file for saving: %1\n")
            .Arg(path));
        return;
    }

    std::unordered_map<libcomp::String, tinyxml2::XMLNode*> existingEvents;

    auto rootElem = doc.RootElement();
    if(!rootElem)
    {
        // If for whatever reason we don't have a root element, create one now
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
        if(!fEvent->FileEventID.IsEmpty())
        {
            // If the event already existed in the file, move it to the same
            // location and drop the old one
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

    if(updatedNodes.size() > 0)
    {
        SimplifyObjectXML(updatedNodes);
    }

    doc.SaveFile(path.C());

    RebuildGlobalIDMap();
    Refresh();
}

void EventWindow::NewFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getSaveFileName(this,
        tr("Create new Event file"), settings.value("datastore").toString(),
        tr("Event XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

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
    }
}

void EventWindow::RemoveEvent()
{
    auto fIter = mFiles.find(cs(ui->files->currentText()));
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
        mMainWindow->ResetEventCount();
        Refresh();
    }
}

void EventWindow::NewEvent()
{
    QAction *pAction = qobject_cast<QAction*>(sender());
    if(!pAction)
    {
        return;
    }

    auto fIter = mFiles.find(cs(ui->files->currentText()));
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;
    auto eventType = (objects::Event::EventType_t)pAction->data().toUInt();

    // Suggest an ID that is not already taken based off current IDs in
    // the file and cross checked against other loaded files
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

    if(commonPrefix.Length() > 0)
    {
        // Add type abbreviation and increase number until new ID is found
        if(commonPrefix.Right(1) == "_")
        {
            // Remove double underscore
            commonPrefix = commonPrefix.Left((size_t)(
                commonPrefix.Length() - 1));
        }

        switch(eventType)
        {
        case objects::Event::EventType_t::NPC_MESSAGE:
            commonPrefix += "_NM";
            break;
        case objects::Event::EventType_t::EX_NPC_MESSAGE:
            commonPrefix += "_EX";
            break;
        case objects::Event::EventType_t::MULTITALK:
            commonPrefix += "_ML";
            break;
        case objects::Event::EventType_t::PROMPT:
            commonPrefix += "_PR";
            break;
        case objects::Event::EventType_t::PERFORM_ACTIONS:
            commonPrefix += "_PA";
            break;
        case objects::Event::EventType_t::OPEN_MENU:
            commonPrefix += "_ME";
            break;
        case objects::Event::EventType_t::PLAY_SCENE:
            commonPrefix += "_SC";
            break;
        case objects::Event::EventType_t::DIRECTION:
            commonPrefix += "_DR";
            break;
        case objects::Event::EventType_t::ITIME:
            commonPrefix += "_IT";
            break;
        case objects::Event::EventType_t::FORK:
        default:
            commonPrefix += "_";
            break;
        }
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
            return;
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

    // Create and add the event
    auto e = GetNewEvent(eventType);
    e->SetID(eventID);

    file->Events.push_back(std::make_shared<FileEvent>(e, true));

    // Rebuild the global map and update the main window
    RebuildLocalIDMap(file);
    RebuildGlobalIDMap();
    mMainWindow->ResetEventCount();

    // Refresh the file and select the new event
    Refresh();
    GoToEvent(eventID);
}

void EventWindow::Refresh()
{
    libcomp::String path = cs(ui->files->currentText());

    SelectFile(path);
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
    BindSelectedEvent();
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
            file->Events.push_back(std::make_shared<FileEvent>(e));
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

    ui->treeWidget->expandAll();
    ui->treeWidget->resizeColumnToContents(0);

    return true;
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

void EventWindow::BindSelectedEvent()
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
        auto iter = mFiles.find(cs(ui->files->currentText()));
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
        std::shared_ptr<objects::Event> e;
        if(fileIdx != -1 && file &&
            file->Events.size() > (size_t)fileIdx)
        {
            auto eIter2 = file->Events.begin();
            std::advance(eIter2, fileIdx);
            mCurrentEvent = *eIter2;
            e = mCurrentEvent->Event;
            editListen = !mCurrentEvent->HasUpdates;
        }

        if(e)
        {
            switch(e->GetEventType())
            {
            case objects::Event::EventType_t::FORK:
                {
                    auto eUI = new Event(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::NPC_MESSAGE:
                {
                    auto eUI = new EventNPCMessage(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::EX_NPC_MESSAGE:
                {
                    auto eUI = new EventExNPCMessage(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::MULTITALK:
                {
                    auto eUI = new EventMultitalk(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::PROMPT:
                {
                    auto eUI = new EventPrompt(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::PERFORM_ACTIONS:
                {
                    auto eUI = new EventPerformActions(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::OPEN_MENU:
                {
                    auto eUI = new EventOpenMenu(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::PLAY_SCENE:
                {
                    auto eUI = new EventPlayScene(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::DIRECTION:
                {
                    auto eUI = new EventDirection(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            case objects::Event::EventType_t::ITIME:
                {
                    auto eUI = new EventITime(mMainWindow);
                    eUI->Load(e);

                    eNode = eUI;
                }
                break;
            default:
                break;
            }
        }
    }

    // If the previous current event was updated, update the event definition
    // from the current control
    if(previousEvent && previousEvent->HasUpdates)
    {
        for(auto eCtrl : ui->splitter->findChildren<Event*>())
        {
            eCtrl->Save();
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
    }
    else
    {
        ui->removeEvent->hide();
        ui->lblNoCurrent->show();
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

    if(!e->GetNext().IsEmpty())
    {
        // Add directly under the node
        AddEventToTree(e->GetNext(), item, file, seen);
    }

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
            auto multi = std::dynamic_pointer_cast<
                objects::EventMultitalk>(e);

            item->setText(1, "Multitalk");
        }
        break;
    case objects::Event::EventType_t::PROMPT:
        {
            auto prompt = std::dynamic_pointer_cast<
                objects::EventPrompt>(e);

            auto cMessage = mMainWindow->GetEventMessage(prompt
                ->GetMessageID());
            item->setText(1, "Prompt");
            item->setText(2, cMessage ? qs(GetInlineMessageText(
                libcomp::String::Join(cMessage->GetLines(), "  "))) : "");

            for(size_t i = 0; i < prompt->ChoicesCount(); i++)
            {
                auto choice = prompt->GetChoices(i);

                EventTreeItem* cNode = new EventTreeItem(item, id);

                cMessage = mMainWindow->GetEventMessage(choice
                    ->GetMessageID());
                cNode->setText(0, qs(libcomp::String("[%1]").Arg(i + 1)));
                cNode->setText(1, "Prompt Choice");
                cNode->setText(2, cMessage ? qs(GetInlineMessageText(
                    libcomp::String::Join(cMessage->GetLines(), "  "))) : "");

                // Add regardless of next results
                if(!choice->GetNext().IsEmpty())
                {
                    AddEventToTree(choice->GetNext(), cNode, file, seen);
                }

                if(choice->BranchesCount() > 0)
                {
                    EventTreeItem* bNode = new EventTreeItem(cNode, id);

                    bNode->setText(0, "[Branches]");

                    for(auto b : choice->GetBranches())
                    {
                        AddEventToTree(b->GetNext(), bNode, file, seen);
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
        }
        break;
    case objects::Event::EventType_t::OPEN_MENU:
        {
            auto menu = std::dynamic_pointer_cast<
                objects::EventOpenMenu>(e);

            item->setText(1, "Open Menu");
        }
        break;
    case objects::Event::EventType_t::PLAY_SCENE:
        {
            auto scene = std::dynamic_pointer_cast<
                objects::EventPlayScene>(e);

            item->setText(1, "Play Scene");
        }
        break;
    case objects::Event::EventType_t::DIRECTION:
        {
            auto dir = std::dynamic_pointer_cast<
                objects::EventDirection>(e);

            item->setText(1, "Direction");
        }
        break;
    case objects::Event::EventType_t::ITIME:
        {
            auto iTime = std::dynamic_pointer_cast<
                objects::EventITime>(e);

            /// @todo: load I-Time strings
            item->setText(1, "I-Time");
        }
        break;
    default:
        break;
    }

    if(e->BranchesCount() > 0)
    {
        // Add under branches child node
        EventTreeItem* bNode = new EventTreeItem(item, id);

        bNode->setText(0, "[Branches]");

        for(auto b : e->GetBranches())
        {
            AddEventToTree(b->GetNext(), bNode, file, seen);
        }
    }
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

void EventWindow::SimplifyObjectXML(std::list<tinyxml2::XMLNode*> nodes)
{
    // Collect all object nodes and simplify by removing defaulted fields.
    // Also remove CDATA blocks as events are not complicated enough to
    // benefit from it.
    std::set<tinyxml2::XMLNode*> currentNodes;
    std::set<tinyxml2::XMLNode*> objectNodes;
    for(auto node : nodes)
    {
        currentNodes.insert(node);
    }

    while(currentNodes.size() > 0)
    {
        auto node = *currentNodes.begin();
        currentNodes.erase(node);

        if(libcomp::String(node->ToElement()->Name()) == "object")
        {
            objectNodes.insert(node);
        }

        auto child = node->FirstChild();
        while(child)
        {
            auto txt = child->ToText();
            if(txt)
            {
                txt->SetCData(false);
            }
            else
            {
                currentNodes.insert(child);
            }

            child = child->NextSibling();
        }
    }

    if(objectNodes.size() == 0)
    {
        return;
    }

    // Create an empty template object for each type encountered for comparison
    tinyxml2::XMLDocument templateDoc;
    std::unordered_map<libcomp::String, std::unordered_map<libcomp::String,
        tinyxml2::XMLNode*>> templateMembers;
    std::unordered_map<libcomp::String, libcomp::String> templateLowerMembers;

    auto rootElem = templateDoc.NewElement("objects");
    templateDoc.InsertEndChild(rootElem);

    for(auto objNode : objectNodes)
    {
        libcomp::String objType(objNode->ToElement()->Attribute("name"));

        auto templateIter = templateMembers.find(objType);
        if(templateIter == templateMembers.end())
        {
            std::shared_ptr<libcomp::Object> obj;
            if(objType == "EventBase")
            {
                obj = std::make_shared<objects::EventBase>();
                templateLowerMembers[objType] = "popNext";
            }
            else if(objType == "EventChoice")
            {
                obj = std::make_shared<objects::EventChoice>();
                templateLowerMembers[objType] = "branchScriptParams";
            }
            else if(objType.Left(6) == "Action")
            {
                // Action derived
                obj = objects::Action::InheritedConstruction(objType);
                templateLowerMembers[objType] = "transformScriptParams";
            }
            else if(objType.Left(5) == "Event")
            {
                if(objType.Right(9) == "Condition")
                {
                    // EventCondition derived
                    obj = objects::EventCondition::InheritedConstruction(
                        objType);
                }
                else
                {
                    // Event derived
                    obj = objects::Event::InheritedConstruction(objType);
                    templateLowerMembers[objType] = "transformScriptParams";
                }
            }

            if(obj)
            {
                obj->Save(templateDoc, *rootElem);

                auto tNode = rootElem->LastChild();

                std::unordered_map<libcomp::String,
                    tinyxml2::XMLNode*> tMembers;
                auto child = tNode->FirstChild();
                while(child)
                {
                    auto elem = child->ToElement();
                    if(elem && libcomp::String(elem->Name()) == "member")
                    {
                        // Remove CDATA here too
                        auto gChild = child->FirstChild();
                        auto txt = gChild ? gChild->ToText() : 0;
                        if(txt && txt->CData())
                        {
                            txt->SetCData(false);
                        }

                        libcomp::String memberName(elem->Attribute("name"));
                        tMembers[memberName] = child;
                    }

                    child = child->NextSibling();
                }

                templateMembers[objType] = tMembers;
                templateIter = templateMembers.find(objType);
            }
            else
            {
                // No simplification
                continue;
            }
        }

        auto& tMembers = templateIter->second;

        auto lowerIter = templateLowerMembers.find(objType);
        if(lowerIter != templateLowerMembers.end())
        {
            // Move the ID to the top (if it exists) and move less important
            // base properties to the bottom
            std::set<libcomp::String> seen;

            auto child = objNode->FirstChild();
            while(child)
            {
                auto next = child->NextSibling();

                libcomp::String memberName(child->ToElement()
                    ->Attribute("name"));
                bool last = memberName == lowerIter->second || !next ||
                    seen.find(memberName) != seen.end();

                seen.insert(memberName);

                if(memberName == "ID")
                {
                    // Move to the top
                    objNode->InsertFirstChild(child);
                }
                else if(!last &&
                    memberName != "next" && memberName != "queueNext")
                {
                    // Move all others to the bottom
                    objNode->InsertEndChild(child);
                }

                if(last)
                {
                    break;
                }
                else
                {
                    child = next;
                }
            }
        }

        if(objType == "EventBase")
        {
            // EventBase is used for the branch structure which does not need
            // the object identifier and often times these can be very simple
            // so drop it here.
            objNode->ToElement()->DeleteAttribute("name");
        }

        // Drop matching level 1 child nodes (anything further down should not
        // be simplified anyway)
        auto child = objNode->FirstChild();
        while(child)
        {
            auto next = child->NextSibling();

            auto elem = child->ToElement();
            if(elem)
            {
                libcomp::String memberName(elem->Attribute("name"));

                auto iter = tMembers.find(memberName);
                if(iter != tMembers.end())
                {
                    auto child2 = iter->second;

                    auto gc = child->FirstChild();
                    auto gc2 = child2->FirstChild();

                    auto txt = gc ? gc->ToText() : 0;
                    auto txt2 = gc2 ? gc2->ToText() : 0;

                    // If both have no child or both have the same text
                    // representation, the nodes match
                    if((gc == 0 && gc2 == 0) || (txt && txt2 &&
                        libcomp::String(txt->Value()) ==
                        libcomp::String(txt2->Value())))
                    {
                        // Default value matches, drop node
                        objNode->DeleteChild(child);
                    }
                }
            }

            child = next;
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
