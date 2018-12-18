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
    EventTreeItem(QTreeWidgetItem* parent, const libcomp::String& eventID = "")
        : QTreeWidgetItem(parent)
    {
        EventID = eventID;
    }

    libcomp::String EventID;
};

class EventFile
{
public:
    libcomp::String Path;
    std::unordered_map<libcomp::String,
        std::shared_ptr<objects::Event>> Events;
    std::list<libcomp::String> EventOrder;
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

    QDirIterator it(qPath, QStringList() << "*.xml", QDir::Files,
        QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        libcomp::String path(it.next().toLocal8Bit().constData());
        if(LoadFileFromPath(path) && ui->files->currentText().isEmpty())
        {
            ui->files->setCurrentText(qs(path));
        }
    }
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

    libcomp::String path(qPath.toLocal8Bit().constData());
    if(LoadFileFromPath(path))
    {
        ui->files->setCurrentText(qs(path));
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

    auto e = file->Events[selected->EventID];

    ui->lblNoCurrent->hide();

    // Clear any existing (should be only one)
    while(ui->layoutView->count() >= 3)
    {
        auto current = ui->layoutView->itemAt(1)->widget();
        ui->layoutView->removeWidget(current);
        current->deleteLater();
    }

    Event* eNode = 0;
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

    if(eNode)
    {
        ui->layoutView->insertWidget(1, eNode);
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

        for(auto e : events)
        {
            file->Events[e->GetID()] = e;
            file->EventOrder.push_back(e->GetID());
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

    std::set<libcomp::String> seen;
    for(auto eventID : file->EventOrder)
    {
        if(seen.find(eventID) == seen.end())
        {
            AddEventToTree(eventID, nullptr, file, seen);
        }
    }

    ui->treeWidget->expandAll();

    return true;
}

void EventWindow::AddEventToTree(const libcomp::String& id,
    EventTreeItem* parent, const std::shared_ptr<EventFile>& file,
    std::set<libcomp::String>& seen)
{
    EventTreeItem* item = 0;
    if(seen.find(id) != seen.end())
    {
        // Add as "goto"
        item = new EventTreeItem(parent, id);

        item->setText(0, qs(libcomp::String("Go to: %1").Arg(id)));
        item->setText(1, "Reference");
    }
    else if(file->Events.find(id) == file->Events.end())
    {
        // Not in the file
        item = new EventTreeItem(parent, id);

        item->setText(0, qs(id));
        item->setText(1, "External");
    }

    if(item)
    {
        if(!parent)
        {
            ui->treeWidget->addTopLevelItem(item);
        }

        return;
    }

    auto e = file->Events[id];
    item = new EventTreeItem(parent, id);
    item->setText(0, qs(id));

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

libcomp::String EventWindow::GetInlineMessageText(const libcomp::String& raw)
{
    return raw.Replace("\n", "  ").Replace("\r", "  ").Left(50);
}
