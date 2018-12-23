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
#include "EventWindow.h"
#include "NPCListWindow.h"
#include "SpotListWindow.h"
#include "ZoneWindow.h"

// objects Includes
#include <MiCEventMessageData.h>
#include <MiONPCData.h>
#include <ServerNPC.h>
#include <ServerZoneSpot.h>

// Qt Includes
#include <PushIgnore.h>
#include "ui_MainWindow.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

MainWindow::MainWindow(QWidget *pParent) : QWidget(pParent)
{
    // Set these first in case the window wants to query for IDs from another.
    mEventWindow = nullptr;
    mNPCWindow = nullptr;
    mSpotWindow = nullptr;
    mZoneWindow = nullptr;

    mEventWindow = new EventWindow(this);
    mNPCWindow = new NPCListWindow(this);
    mSpotWindow = new SpotListWindow(this);
    mZoneWindow = new ZoneWindow(this);

    ui = new Ui::MainWindow;
    ui->setupUi(this);

    connect(ui->zoneBrowse, SIGNAL(clicked(bool)), this, SLOT(BrowseZone()));

    connect(ui->eventsView, SIGNAL(clicked(bool)), this, SLOT(OpenEvents()));
    connect(ui->openMap, SIGNAL(clicked(bool)), this, SLOT(OpenMap()));
    connect(ui->openNPCs, SIGNAL(clicked(bool)), this, SLOT(OpenNPCs()));
    connect(ui->openSpots, SIGNAL(clicked(bool)), this, SLOT(OpenSpots()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::Init()
{
    libcomp::Log::GetSingletonPtr()->AddLogHook([&](
        libcomp::Log::Level_t level, const libcomp::String& msg)
    {
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

    std::string err;

    if(!mDatastore->AddSearchPath(settingVal.toStdString()))
    {
        err = "Failed to add datastore search path.";
    }
    else if(!mDefinitions->LoadData<objects::MiZoneData>(mDatastore.get()))
    {
        err = "Failed to load zone data.";
    }
    else if(!mDefinitions->LoadData<objects::MiDynamicMapData>(mDatastore.get()))
    {
        err = "Failed to load dynamic map data.";
    }
    else if(!mDefinitions->LoadData<objects::MiDevilData>(mDatastore.get()))
    {
        err = "Failed to load devil data.";
    }
    else if(!mDefinitions->LoadData<objects::MiHNPCData>(mDatastore.get()))
    {
        err = "Failed to load hNPC data.";
    }
    else if(!mDefinitions->LoadData<objects::MiONPCData>(mDatastore.get()))
    {
        err = "Failed to load oNPC data.";
    }

    if(err.length() > 0)
    {
        QMessageBox Msgbox;
        Msgbox.setText(err.c_str());
        Msgbox.exec();

        return false;
    }

    // Load some "nice to haves"
    if(!LoadCMessageData(mCEventMessageData, "CEventMessageData.xml"))
    {
        LOG_ERROR("Failed to load CEventMessageData.xml\n");
    }

    if(!LoadCMessageData(mCEventMessageData2, "CEventMessageData2.xml"))
    {
        LOG_ERROR("Failed to load CEventMessageData2.xml\n");
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

NPCListWindow* MainWindow::GetNPCList() const
{
    return mNPCWindow;
}

SpotListWindow* MainWindow::GetSpotList() const
{
    return mSpotWindow;
}

std::shared_ptr<objects::MiCEventMessageData> MainWindow::GetEventMessage(
    int32_t msgID) const
{
    auto msg = mCEventMessageData->GetObjectByID((uint32_t)msgID);
    if(!msg)
    {
        msg = mCEventMessageData2->GetObjectByID((uint32_t)msgID);
    }

    return std::dynamic_pointer_cast<objects::MiCEventMessageData>(msg);
}

void MainWindow::ResetEventCount()
{
    size_t total = mEventWindow->GetLoadedEventCount();
    ui->eventCount->setText(qs(libcomp::String("%1 event(s) loaded")
        .Arg(total)));
}

void MainWindow::ReloadZoneData()
{
    ui->openMap->setEnabled(mActiveZone != nullptr);

    // NPCs
    {
        std::vector<std::shared_ptr<libcomp::Object>> objs;

        for(auto obj : mActiveZone->GetNPCs())
        {
            objs.push_back(obj);
        }

        mNPCWindow->SetObjectList(objs);
        ui->openNPCs->setEnabled(true);
    }

    // Spots
    {
        std::vector<std::shared_ptr<libcomp::Object>> objs;

        for(auto obj : mActiveZone->GetSpots())
        {
            objs.push_back(obj.second);
        }

        mSpotWindow->SetObjectList(objs);
        ui->openSpots->setEnabled(true);
    }

    // Reset all links in the combo boxes.
    mNPCWindow->ResetSpotList();
}

bool MainWindow::LoadCMessageData(std::shared_ptr<
    libcomp::BinaryDataSet>& dataset, const libcomp::String& file)
{
    dataset = std::make_shared<libcomp::BinaryDataSet>([]()
        {
            return std::make_shared<objects::MiCEventMessageData>();
        },

        [](const std::shared_ptr<libcomp::Object>& obj)
        {
            return std::dynamic_pointer_cast<objects::MiCEventMessageData>(
                obj)->GetID();
        });

    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_NO_ERROR == doc.LoadFile(file.C()) && dataset->LoadXml(doc))
    {
        LOG_DEBUG(libcomp::String("Loaded %1 MiCEventMessageData records from"
            " %2\n").Arg(dataset->GetObjects().size()).Arg(file));
        return true;
    }

    return false;
}

void MainWindow::OpenEvents()
{
    mEventWindow->show();
    mEventWindow->raise();
}

void MainWindow::OpenMap()
{
    if(mZoneWindow->ShowZone(mActiveZone))
    {
        mZoneWindow->raise();
    }
}

void MainWindow::OpenNPCs()
{
    mNPCWindow->show();
    mNPCWindow->raise();
}

void MainWindow::OpenSpots()
{
    mSpotWindow->show();
    mSpotWindow->raise();
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

    // Reload all views now.
    ReloadZoneData();
}
