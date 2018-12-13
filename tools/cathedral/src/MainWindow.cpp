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
#include "NPCListWindow.h"
#include "SpotListWindow.h"

// objects Includes
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

// tinyxml2 Includes
#include <PushIgnore.h>
#include <tinyxml2.h>
#include <PopIgnore.h>

namespace libcomp
{
class BinaryDataSet
{
public:
    BinaryDataSet(std::function<std::shared_ptr<libcomp::Object>()> allocator,
        std::function<uint32_t(const std::shared_ptr<
            libcomp::Object>&)> mapper);

    bool Load(std::istream& file);
    bool Save(std::ostream& file) const;

    bool LoadXml(tinyxml2::XMLDocument& doc);

    std::string GetXml() const;

    std::list<std::shared_ptr<libcomp::Object>> GetObjects() const;
    std::shared_ptr<libcomp::Object> GetObjectByID(uint32_t id) const;

private:
    std::function<std::shared_ptr<libcomp::Object>()> mObjectAllocator;
    std::function<uint32_t(const std::shared_ptr<
        libcomp::Object>&)> mObjectMapper;

    std::list<std::shared_ptr<libcomp::Object>> mObjects;
    std::map<uint32_t, std::shared_ptr<libcomp::Object>> mObjectMap;
};
}

libcomp::BinaryDataSet::BinaryDataSet(std::function<std::shared_ptr<
    libcomp::Object>()> allocator, std::function<uint32_t(
    const std::shared_ptr<libcomp::Object>&)> mapper) :
    mObjectAllocator(allocator), mObjectMapper(mapper)
{
}


bool libcomp::BinaryDataSet::Load(std::istream& file)
{
    mObjects = libcomp::Object::LoadBinaryData(file,
        mObjectAllocator);

    for(auto obj : mObjects)
    {
        mObjectMap[mObjectMapper(obj)] = obj;
    }

    return !mObjects.empty();
}

bool libcomp::BinaryDataSet::Save(std::ostream& file) const
{
    return libcomp::Object::SaveBinaryData(file, mObjects);
}

bool libcomp::BinaryDataSet::LoadXml(tinyxml2::XMLDocument& doc)
{
    std::list<std::shared_ptr<libcomp::Object>> objs;

    auto pRoot = doc.RootElement();

    if(nullptr == pRoot)
    {
        return false;
    }

    auto objElement = pRoot->FirstChildElement("object");

    while(nullptr != objElement)
    {
        auto obj = mObjectAllocator();

        if(!obj->Load(doc, *objElement))
        {
            return false;
        }

        objs.push_back(obj);

        objElement = objElement->NextSiblingElement("object");
    }

    mObjects = objs;
    mObjectMap.clear();

    for(auto obj : mObjects)
    {
        mObjectMap[mObjectMapper(obj)] = obj;
    }

    return !mObjects.empty();
}

std::string libcomp::BinaryDataSet::GetXml() const
{
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLElement *pRoot = doc.NewElement("objects");
    doc.InsertEndChild(pRoot);

    for(auto obj : mObjects)
    {
        if(!obj->Save(doc, *pRoot))
        {
            return {};
        }
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    return printer.CStr();
}

std::list<std::shared_ptr<libcomp::Object>> libcomp::BinaryDataSet::GetObjects() const
{
    return mObjects;
}

std::shared_ptr<libcomp::Object> libcomp::BinaryDataSet::GetObjectByID(
    uint32_t id) const
{
    auto it = mObjectMap.find(id);

    if(mObjectMap.end() != it)
    {
        return it->second;
    }

    return {};
}

MainWindow::MainWindow(QWidget *pParent) : QWidget(pParent)
{
    // Set these first in case the window wants to query for IDs from another.
    mNPCWindow = nullptr;
    mSpotWindow = nullptr;

    mNPCWindow = new NPCListWindow(this);
    mSpotWindow = new SpotListWindow(this);

    ui = new Ui::MainWindow;
    ui->setupUi(this);

    connect(ui->zoneBrowse, SIGNAL(clicked(bool)), this, SLOT(BrowseZone()));

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

NPCListWindow* MainWindow::GetNPCList() const
{
    return mNPCWindow;
}

SpotListWindow* MainWindow::GetSpotList() const
{
    return mSpotWindow;
}

void MainWindow::ReloadZoneData()
{
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

    libcomp::BinaryDataSet *pSet = new libcomp::BinaryDataSet([]()
        {
            return std::make_shared<objects::ServerZone>();
        },

        [](const std::shared_ptr<libcomp::Object>& obj)
        {
            return std::dynamic_pointer_cast<objects::ServerZone>(
                obj)->GetID();
        }
    );

    if(!pSet->LoadXml(doc))
    {
        LOG_ERROR(libcomp::String("Failed to load file: %1\n").Arg(
            path.toLocal8Bit().constData()));

        return;
    }

    auto objs = pSet->GetObjects();

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
