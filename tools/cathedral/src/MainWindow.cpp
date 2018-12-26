/**
 * @file tools/cathedral/src/MainWindow.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main window implementation.
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

#include "MainWindow.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "EventWindow.h"
#include "NPCList.h"
#include "ObjectSelectorList.h"
#include "ObjectSelectorWindow.h"
#include "SpotList.h"
#include "ZoneWindow.h"

// objects Includes
#include <MiCEventMessageData.h>
#include <MiCItemBaseData.h>
#include <MiCItemData.h>
#include <MiCSoundData.h>
#include <MiCQuestData.h>
#include <MiDevilData.h>
#include <MiDynamicMapData.h>
#include <MiHNPCBasicData.h>
#include <MiHNPCData.h>
#include <MiNPCBasicData.h>
#include <MiONPCData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <ServerNPC.h>
#include <ServerZoneSpot.h>

// Qt Includes
#include <PushIgnore.h>
#include "ui_MainWindow.h"

#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

#define BDSET(objname, getid, getname) \
    std::make_shared<BinaryDataNamedSet>( \
        []() \
        { \
            return std::make_shared<objects::objname>(); \
        }, \
        [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t \
        { \
            return static_cast<uint32_t>(std::dynamic_pointer_cast< \
                objects::objname>(obj)->getid); \
        }, \
        [](const std::shared_ptr<libcomp::Object>& obj) -> libcomp::String \
        { \
            return std::dynamic_pointer_cast< \
                objects::objname>(obj)->getname; \
        }); \

MainWindow::MainWindow(QWidget *pParent) : QMainWindow(pParent)
{
    // Set these first in case the window wants to query for IDs from another.
    mEventWindow = nullptr;
    mZoneWindow = nullptr;

    mEventWindow = new EventWindow(this);
    mZoneWindow = new ZoneWindow(this);

    ui = new Ui::MainWindow;
    ui->setupUi(this);

    connect(ui->zoneBrowse, SIGNAL(clicked(bool)), this, SLOT(BrowseZone()));

    connect(ui->eventsView, SIGNAL(clicked(bool)), this, SLOT(OpenEvents()));
    connect(ui->zoneView, SIGNAL(clicked(bool)), this, SLOT(OpenZone()));

    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
}

MainWindow::~MainWindow()
{
    delete ui;

    for(auto& pair : mObjectSelectors)
    {
        delete pair.second;
    }

    mObjectSelectors.clear();
}

bool MainWindow::Init()
{
    libcomp::Log::GetSingletonPtr()->AddLogHook([&](
        libcomp::Log::Level_t level, const libcomp::String& msg)
    {
        ui->log->moveCursor(QTextCursor::End);
        ui->log->setFontWeight(QFont::Normal);

        switch(level)
        {
            case libcomp::Log::Level_t::LOG_LEVEL_DEBUG:
                ui->log->setTextColor(QColor(Qt::darkGreen));
                break;
            case libcomp::Log::Level_t::LOG_LEVEL_INFO:
                ui->log->setTextColor(QColor(Qt::black));
                break;
                case libcomp::Log::Level_t::LOG_LEVEL_WARNING:
                ui->log->setTextColor(QColor(Qt::darkYellow));
                break;
            case libcomp::Log::Level_t::LOG_LEVEL_ERROR:
                ui->log->setTextColor(QColor(Qt::red));
                break;
            case libcomp::Log::Level_t::LOG_LEVEL_CRITICAL:
                ui->log->setTextColor(QColor(Qt::red));
                ui->log->setFontWeight(QFont::Bold);
                break;
            default:
                break;
        }

        ui->log->insertPlainText(msg.C());
        ui->log->moveCursor(QTextCursor::End);
    });

    mDatastore = std::make_shared<libcomp::DataStore>("comp_cathedral");
    mDefinitions = std::make_shared<libcomp::DefinitionManager>();

    QSettings settings;

    QString settingVal = settings.value("datastore", "error").toString();

    bool settingPath = false;

    if(settingVal == "error")
    {
        settingVal = QFileDialog::getExistingDirectory(nullptr,
            "Datastore path", "", QFileDialog::ShowDirsOnly);

        if(settingVal.isEmpty())
        {
            // Cancelling
            return false;
        }
        settingPath = true;
    }

    mBinaryDataSets["CEventMessageData"] = std::make_shared<
        BinaryDataNamedSet>([]()
        {
            return std::make_shared<objects::MiCEventMessageData>();
        },
        [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
        {
            return std::dynamic_pointer_cast<
                objects::MiCEventMessageData>(obj)->GetID();
        },
        [](const std::shared_ptr<libcomp::Object>& obj)->libcomp::String
        {
            // Combine lines so they all display
            auto msg = std::dynamic_pointer_cast<objects::MiCEventMessageData>(
                obj);
            return libcomp::String::Join(msg->GetLines(), "\n\r");
        });

    mBinaryDataSets["CItemData"] = BDSET(MiCItemData, GetBaseData()->GetID(),
        GetBaseData()->GetName2());
    mBinaryDataSets["CSoundData"] = BDSET(MiCSoundData, GetID(), GetPath());
    mBinaryDataSets["CSoundData"] = BDSET(MiCSoundData, GetID(), GetPath());
    mBinaryDataSets["CQuestData"] = BDSET(MiCQuestData, GetID(), GetTitle());
    mBinaryDataSets["DevilData"] = BDSET(MiDevilData, GetBasic()->GetID(),
        GetBasic()->GetName());
    mBinaryDataSets["hNPCData"] = BDSET(MiHNPCData, GetBasic()->GetID(),
        GetBasic()->GetName());
    mBinaryDataSets["oNPCData"] = BDSET(MiONPCData, GetID(), GetName());
    mBinaryDataSets["ZoneData"] = BDSET(MiZoneData, GetBasic()->GetID(),
        GetBasic()->GetName());

    std::string err;

    if(!mDatastore->AddSearchPath(settingVal.toStdString()))
    {
        err = "Failed to add datastore search path.";
    }
    else if(!LoadBinaryData("Shield/CEventMessageData.sbin",
        "CEventMessageData", true, true, false) ||
       !LoadBinaryData("Shield/CEventMessageData2.sbin", "CEventMessageData",
            true, true, false))
    {
        err = "Failed to load event message data.";
    }
    else if(!LoadBinaryData("Shield/CItemData.sbin", "CItemData", true, true))
    {
        err = "Failed to load c-item data.";
    }
    else if(!LoadBinaryData("Client/CSoundData.bin", "CSoundData", false, true))
    {
        err = "Failed to load c-sound data.";
    }
    else if(!LoadBinaryData("Shield/CQuestData.sbin", "CQuestData", true, true))
    {
        err = "Failed to load c-quest data.";
    }
    else if(!LoadBinaryData("Shield/DevilData.sbin", "DevilData", true, true))
    {
        err = "Failed to load devil data.";
    }
    else if(!mDefinitions->LoadData<objects::MiDynamicMapData>(mDatastore.get()))
    {
        // Dynamic map data uses the definition manager as it handles spot
        // data loading as well and these records do not need to be referenced
        err = "Failed to load dynamic map data.";
    }
    else if(!LoadBinaryData("Shield/hNPCData.sbin", "hNPCData", true, true))
    {
        err = "Failed to load hNPC data.";
    }
    else if(!LoadBinaryData("Shield/oNPCData.sbin", "oNPCData", true, true))
    {
        err = "Failed to load oNPC data.";
    }
    else if(!LoadBinaryData("Shield/ZoneData.sbin", "ZoneData", true, true))
    {
        err = "Failed to load zone data.";
    }

    if(err.length() > 0)
    {
        QMessageBox Msgbox;
        Msgbox.setText(err.c_str());
        Msgbox.exec();

        return false;
    }

    if(settingPath)
    {
        // Save the new ini now that we know its valid
        settings.setValue("datastore", settingVal);
        settings.sync();
    }

    return true;
}

std::shared_ptr<libcomp::DataStore> MainWindow::GetDatastore() const
{
    return mDatastore;
}

std::shared_ptr<libcomp::DefinitionManager> MainWindow::GetDefinitions() const
{
    return mDefinitions;
}

EventWindow* MainWindow::GetEvents() const
{
    return mEventWindow;
}

std::shared_ptr<objects::MiCEventMessageData> MainWindow::GetEventMessage(
    int32_t msgID) const
{
    auto ds = GetBinaryDataSet("CEventMessageData");
    auto msg = ds->GetObjectByID((uint32_t)msgID);

    return std::dynamic_pointer_cast<objects::MiCEventMessageData>(msg);
}

std::shared_ptr<libcomp::BinaryDataSet> MainWindow::GetBinaryDataSet(
    const libcomp::String& objType) const
{
    auto iter = mBinaryDataSets.find(objType);
    return iter != mBinaryDataSets.end() ? iter->second : nullptr;
}

ObjectSelectorWindow* MainWindow::GetObjectSelector(
    const libcomp::String& objType) const
{
    auto iter = mObjectSelectors.find(objType);
    return iter != mObjectSelectors.end() ? iter->second : nullptr;
}

void MainWindow::ResetEventCount()
{
    size_t total = mEventWindow->GetLoadedEventCount();
    ui->eventCount->setText(qs(libcomp::String("%1 event(s) loaded")
        .Arg(total)));
}

bool MainWindow::LoadBinaryData(const libcomp::String& binaryFile,
    const libcomp::String& objName, bool decrypt, bool addSelector,
    bool selectorAllowBlanks)
{
    auto dataset = GetBinaryDataSet(objName);
    if(!dataset)
    {
        return false;
    }

    auto path = libcomp::String("/BinaryData/") + binaryFile;

    std::vector<char> data;
    if(decrypt)
    {
        data = mDatastore->DecryptFile(path);
    }
    else
    {
        data = mDatastore->ReadFile(path);
    }

    if(data.empty())
    {
        return false;
    }
    else
    {
        LOG_DEBUG(libcomp::String("Loading records from %1\n")
            .Arg(binaryFile));
    }

    std::stringstream ss(std::string(data.begin(), data.end()));
    if(dataset->Load(ss, true))
    {
        auto namedSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            dataset);
        if(addSelector && namedSet &&
            mObjectSelectors.find(objName) == mObjectSelectors.end())
        {
            ObjectSelectorWindow* selector = new ObjectSelectorWindow();
            selector->Bind(new ObjectSelectorList(namedSet,
                selectorAllowBlanks));
            mObjectSelectors[objName] = selector;

            // Build a menu action for viewing without selection
            QAction *pAction = ui->menuResourceList->addAction(qs(objName));
            connect(pAction, SIGNAL(triggered()), this,
                SLOT(ViewObjectList()));
        }

        return true;
    }

    return false;
}


void MainWindow::CloseAllWindows() 
{
    for(auto& pair : mObjectSelectors)
    {
        pair.second->close();
    }

    if(mEventWindow)
    {
        mEventWindow->close();
    }

    if(mZoneWindow)
    {
        mZoneWindow->close();
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    CloseAllWindows();
}

void MainWindow::OpenEvents()
{
    mEventWindow->show();
    mEventWindow->raise();
}

void MainWindow::OpenZone()
{
    if(mZoneWindow->ShowZone(mActiveZone))
    {
        mZoneWindow->raise();
    }
}

void MainWindow::ViewObjectList()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
    {
        auto objType = cs(action->text());
        auto selector = GetObjectSelector(objType);
        if(selector)
        {
            selector->Open(nullptr);
        }
    }
}

void MainWindow::BrowseZone()
{
    QSettings settings;

    QString path = QFileDialog::getOpenFileName(this, tr("Open Zone XML"),
        settings.value("datastore").toString(), tr("Zone XML (*.xml)"));

    if(path.isEmpty())
    {
        return;
    }

    tinyxml2::XMLDocument doc;

    if(tinyxml2::XML_NO_ERROR != doc.LoadFile(path.toLocal8Bit().constData()))
    {
        LOG_ERROR(libcomp::String("Failed to parse file: %1\n").Arg(
            path.toLocal8Bit().constData()));

        return;
    }

    libcomp::BinaryDataSet pSet([]()
        {
            return std::make_shared<objects::ServerZone>();
        },

        [](const std::shared_ptr<libcomp::Object>& obj)
        {
            return std::dynamic_pointer_cast<objects::ServerZone>(
                obj)->GetID();
        }
    );

    if(!pSet.LoadXml(doc))
    {
        LOG_ERROR(libcomp::String("Failed to load file: %1\n").Arg(
            path.toLocal8Bit().constData()));

        return;
    }

    auto objs = pSet.GetObjects();

    if(1 != objs.size())
    {
        LOG_ERROR("There must be exactly 1 zone in the XML file.\n");

        return;
    }

    auto zone = std::dynamic_pointer_cast<objects::ServerZone>(objs.front());

    if(!zone)
    {
        LOG_ERROR("Internal error loading zone.\n");

        return;
    }

    mActiveZonePath = path;
    mActiveZone = zone;

    ui->zonePath->setText(QDir::toNativeSeparators(path));

    LOG_INFO(libcomp::String("Loaded: %1\n").Arg(
        path.toLocal8Bit().constData()));

    ui->zoneView->setEnabled(mActiveZone != nullptr);
}
