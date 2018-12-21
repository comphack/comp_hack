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
#include "ui_EventWindow.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
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

class EventFile
{
public:
    libcomp::String Path;
    std::list<std::shared_ptr<objects::Event>> Events;
    std::unordered_map<libcomp::String, int32_t> EventIDMap;
};

EventWindow::EventWindow(MainWindow *pMainWindow, QWidget *pParent) :
    QWidget(pParent), mMainWindow(pMainWindow)
{
    ui = new Ui::EventWindow;
    ui->setupUi(this);

    connect(ui->loadDirectory, SIGNAL(clicked(bool)), this,
        SLOT(LoadDirectory()));
    connect(ui->loadFile, SIGNAL(clicked(bool)), this, SLOT(LoadFile()));
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

    libcomp::String currentPath(ui->files
        ->currentText().toLocal8Bit().constData());
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
    libcomp::String path(ui->files->currentText().toLocal8Bit().constData());

    SelectFile(path);
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
    libcomp::String currentPath(ui->files->currentText().toLocal8Bit()
        .constData());
    libcomp::String selectPath = currentPath;
    while(it.hasNext())
    {
        libcomp::String path(it.next().toLocal8Bit().constData());
        if(LoadFileFromPath(path) && selectPath.IsEmpty())
        {
            selectPath = path;
        }
    }

    ui->files->blockSignals(false);

    RebuildGlobalIDMap();
    mMainWindow->ResetEventCount();

    // Refresh selection even if it didnt change
    FileSelectionChanged();
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

    libcomp::String path(qPath.toLocal8Bit().constData());
    if(LoadFileFromPath(path))
    {
        RebuildGlobalIDMap();
        mMainWindow->ResetEventCount();

        ui->files->blockSignals(false);

        ui->files->setCurrentText(qs(path));
    }
    else
    {
        ui->files->blockSignals(false);
    }
}

void EventWindow::TreeSelectionChanged()
{
    EventTreeItem* selected = 0;
    for(auto node : ui->treeWidget->selectedItems())
    {
        selected = (EventTreeItem*)node;
    }

    if(selected == nullptr)
    {
        // Nothing selected
        return;
    }
    
    auto file = mFiles[libcomp::String(ui->files
        ->currentText().toUtf8().constData())];

    QWidget* eNode = 0;

    // Find the event
    int32_t fileIdx = selected->FileIdx;
    if(fileIdx == -1)
    {
        auto eIter = file->EventIDMap.find(selected->EventID);
        if(eIter != file->EventIDMap.end())
        {
            fileIdx = eIter->second;
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
        if(fileIdx != -1 && file->Events.size() > (size_t)fileIdx)
        {
            auto eIter2 = file->Events.begin();
            std::advance(eIter2, fileIdx);
            e = *eIter2;
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
    
    // Clear any existing (should be only one)
    while(ui->layoutView->count() >= 3)
    {
        auto current = ui->layoutView->itemAt(1)->widget();
        ui->layoutView->removeWidget(current);
        current->deleteLater();
    }

    if(eNode)
    {
        ui->lblNoCurrent->hide();

        ui->layoutView->insertWidget(1, eNode);
    }
    else
    {
        ui->lblNoCurrent->show();
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

    if(events.size() > 0)
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
            if(file->EventIDMap.find(e->GetID()) == file->EventIDMap.end())
            {
                // Don't add it twice
                file->EventIDMap[e->GetID()] = (int32_t)file->Events.size();
            }
            else
            {
                LOG_ERROR(libcomp::String("Duplicate event ID %1 encountered"
                    " in file: %2\n").Arg(e->GetID()).Arg(path));
            }

            file->Events.push_back(e);
        }

        mFiles[path] = file;

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
    for(auto e : file->Events)
    {
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

    return true;
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
                item->setText(1, "Invalid");
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

        e = *eIter;
        item = new EventTreeItem(parent, id);
        item->setText(0, qs(id));
    }
    else
    {
        auto eIter = file->Events.begin();
        std::advance(eIter, eventIdx);

        e = *eIter;
        item = new EventTreeItem(parent, "", eventIdx);
        item->setText(0, qs(libcomp::String("%1 [Duplicate]").Arg(id)));
        item->setTextColor(0, QColor(255, 0, 0));
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
            item->setText(1, "NPC Message");
            item->setText(2, cMessage
                ? qs(GetInlineMessageText(cMessage->GetLines(0))) : "");
        }
        break;
    case objects::Event::EventType_t::EX_NPC_MESSAGE:
        {
            auto msg = std::dynamic_pointer_cast<
                objects::EventExNPCMessage>(e);

            auto cMessage = mMainWindow->GetEventMessage(msg
                ->GetMessageID());
            item->setText(1, "EX NPC Message");
            item->setText(2, cMessage
                ? qs(GetInlineMessageText(cMessage->GetLines(0))) : "");
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
            item->setText(2, cMessage
                ? qs(GetInlineMessageText(cMessage->GetLines(0))) : "");

            for(size_t i = 0; i < prompt->ChoicesCount(); i++)
            {
                auto choice = prompt->GetChoices(i);

                EventTreeItem* cNode = new EventTreeItem(item, id);

                cMessage = mMainWindow->GetEventMessage(choice
                    ->GetMessageID());
                cNode->setText(0, qs(libcomp::String("[%1]").Arg(i + 1)));
                cNode->setText(1, "Prompt Choice");
                cNode->setText(2, cMessage
                    ? qs(GetInlineMessageText(cMessage->GetLines(0))) : "");

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

libcomp::String EventWindow::GetInlineMessageText(const libcomp::String& raw)
{
    return raw.Replace("\n", "  ").Replace("\r", "  ").Left(50);
}
