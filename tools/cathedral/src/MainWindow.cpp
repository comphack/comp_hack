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
#include "DropSetWindow.h"
#include "EventWindow.h"
#include "ObjectSelectorList.h"
#include "ObjectSelectorWindow.h"
#include "SettingsWindow.h"
#include "ZoneWindow.h"

// objects Includes
#include <MiAIData.h>
#include <MiCancelData.h>
#include <MiCEventMessageData.h>
#include <MiCHouraiData.h>
#include <MiCHouraiMessageData.h>
#include <MiCItemBaseData.h>
#include <MiCItemData.h>
#include <MiCKeyItemData.h>
#include <MiCQuestData.h>
#include <MiCSoundData.h>
#include <MiCStatusData.h>
#include <MiCTitleData.h>
#include <MiCValuablesData.h>
#include <MiDevilData.h>
#include <MiDynamicMapData.h>
#include <MiHNPCBasicData.h>
#include <MiHNPCData.h>
#include <MiKeyItemData.h>
#include <MiNPCBasicData.h>
#include <MiONPCData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiShopProductData.h>
#include <MiStatusData.h>
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
#include <QTextStream>
#include <PopIgnore.h>

// libcomp Includes
#include <Exception.h>
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
    mDropSetWindow = nullptr;
    mEventWindow = nullptr;
    mZoneWindow = nullptr;

    mDropSetWindow = new DropSetWindow(this);
    mEventWindow = new EventWindow(this);
    mZoneWindow = new ZoneWindow(this);

    ui = new Ui::MainWindow;
    ui->setupUi(this);

    connect(ui->zoneBrowse, SIGNAL(clicked(bool)), this, SLOT(BrowseZone()));

    connect(ui->dropSetView, SIGNAL(clicked(bool)), this, SLOT(OpenDropSets()));
    connect(ui->eventsView, SIGNAL(clicked(bool)), this, SLOT(OpenEvents()));
    connect(ui->zoneView, SIGNAL(clicked(bool)), this, SLOT(OpenZone()));

    connect(ui->actionSettings, SIGNAL(triggered()), this,
        SLOT(OpenSettings()));
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

        bool logCrash = false;
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
                logCrash = true;
                break;
            default:
                break;
        }

        ui->log->insertPlainText(msg.C());
        ui->log->moveCursor(QTextCursor::End);

        if(logCrash)
        {
            QSettings settings;
            auto dumpFile = settings.value("crashDump").toString();
            if(!dumpFile.isEmpty())
            {
                QFile file(dumpFile);
                file.open(QIODevice::WriteOnly | QIODevice::Append);
                if(file.isOpen())
                {
                    QTextStream oStream(&file);
                    oStream << qs(msg);
                    file.close();
                }
            }
        }
    });

    libcomp::Exception::RegisterSignalHandler();

    mDatastore = std::make_shared<libcomp::DataStore>("comp_cathedral");
    mDefinitions = std::make_shared<libcomp::DefinitionManager>();

    QSettings settings;

    QString settingVal = settings.value("datastore", "error").toString();

    if(settingVal == "error" || !QDir(settingVal).exists())
    {
        SettingsWindow* settingWindow = new SettingsWindow(this, true, this);
        settingWindow->setWindowModality(Qt::ApplicationModal);

        settingWindow->exec();

        delete settingWindow;

        settingVal = settings.value("datastore", "").toString();

        if(settingVal.isEmpty())
        {
            // Cancelling
            return false;
        }
    }

    mBinaryDataSets["AIData"] = std::make_shared<
        BinaryDataNamedSet>([]()
        {
            return std::make_shared<objects::MiAIData>();
        },
        [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
        {
            return std::dynamic_pointer_cast<
                objects::MiAIData>(obj)->GetID();
        },
        [](const std::shared_ptr<libcomp::Object>& obj)->libcomp::String
        {
            return libcomp::String("AI %1").Arg(std::dynamic_pointer_cast<
                objects::MiAIData>(obj)->GetID());
        });

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

    mBinaryDataSets["CHouraiData"] = BDSET(MiCHouraiData, GetID(), GetName());
    mBinaryDataSets["CHouraiMessageData"] = BDSET(MiCHouraiMessageData,
        GetID(), GetMessage());
    mBinaryDataSets["CItemData"] = BDSET(MiCItemData, GetBaseData()->GetID(),
        GetBaseData()->GetName2());
    mBinaryDataSets["CKeyItemData"] = BDSET(MiCKeyItemData, GetItemData()->GetID(),
        GetItemData()->GetName());
    mBinaryDataSets["CQuestData"] = BDSET(MiCQuestData, GetID(), GetTitle());
    mBinaryDataSets["CSoundData"] = BDSET(MiCSoundData, GetID(), GetPath());
    mBinaryDataSets["CStatusData"] = BDSET(MiCStatusData, GetID(), GetName());
    mBinaryDataSets["CTitleData"] = BDSET(MiCTitleData, GetID(), GetTitle());
    mBinaryDataSets["CValuablesData"] = BDSET(MiCValuablesData, GetID(),
        GetName());
    mBinaryDataSets["DevilData"] = BDSET(MiDevilData, GetBasic()->GetID(),
        GetBasic()->GetName());
    mBinaryDataSets["hNPCData"] = BDSET(MiHNPCData, GetBasic()->GetID(),
        GetBasic()->GetName());
    mBinaryDataSets["oNPCData"] = BDSET(MiONPCData, GetID(), GetName());
    mBinaryDataSets["ZoneData"] = BDSET(MiZoneData, GetBasic()->GetID(),
        GetBasic()->GetName());

    // Special data sets that will be modified later
    mBinaryDataSets["ShopProductData"] = std::make_shared<
        BinaryDataNamedSet>([]()
        {
            return std::make_shared<objects::MiShopProductData>();
        },
        [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
        {
            return std::dynamic_pointer_cast<
                objects::MiShopProductData>(obj)->GetID();
        },
        // Names will be mapped below
        nullptr);
    mBinaryDataSets["StatusData"] = std::make_shared<
        BinaryDataNamedSet>([]()
        {
            return std::make_shared<objects::MiStatusData>();
        },
        [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
        {
            return std::dynamic_pointer_cast<
                objects::MiStatusData>(obj)->GetCommon()->GetID();
        },
        // Names will be mapped below
        nullptr);

    std::string err;

    if(!mDatastore->AddSearchPath(settingVal.toStdString()))
    {
        err = "Failed to add datastore search path.";
    }
    else if(!LoadBinaryData("Shield/AIData.sbin", "AIData", true, false))
    {
        err = "Failed to load AI data.";
    }
    else if(!LoadBinaryData("Shield/CEventMessageData.sbin",
        "CEventMessageData", true, true, false) ||
       !LoadBinaryData("Shield/CEventMessageData2.sbin", "CEventMessageData",
            true, true, false))
    {
        err = "Failed to load event message data.";
    }
    else if(!LoadBinaryData("Shield/CHouraiData.sbin", "CHouraiData", true,
        true))
    {
        err = "Failed to load hourai data.";
    }
    else if(!LoadBinaryData("Shield/CHouraiMessageData.sbin",
        "CHouraiMessageData", true, true))
    {
        err = "Failed to load hourai message data.";
    }
    else if(!LoadBinaryData("Shield/CItemData.sbin", "CItemData", true, true))
    {
        err = "Failed to load c-item data.";
    }
    else if(!LoadBinaryData("Shield/CKeyItemData.sbin", "CKeyItemData",
        true, true))
    {
        err = "Failed to load c-key item data.";
    }
    else if(!LoadBinaryData("Shield/CQuestData.sbin", "CQuestData", true,
        true))
    {
        err = "Failed to load c-quest data.";
    }
    else if(!LoadBinaryData("Client/CSoundData.bin", "CSoundData", false,
        true))
    {
        err = "Failed to load c-sound data.";
    }
    else if(!LoadBinaryData("Shield/CStatusData.sbin", "CStatusData", true,
        false))
    {
        err = "Failed to load c-status data.";
    }
    else if(!LoadBinaryData("Shield/CTitleData.sbin", "CTitleData", true,
        true))
    {
        err = "Failed to load c-title data.";
    }
    else if(!LoadBinaryData("Shield/CValuablesData.sbin", "CValuablesData",
        true, true))
    {
        err = "Failed to load c-valuables data.";
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
    else if(!LoadBinaryData("Shield/ShopProductData.sbin", "ShopProductData",
        true, true))
    {
        err = "Failed to load shop product data.";
    }
    else if(!LoadBinaryData("Shield/StatusData.sbin", "StatusData", true,
        true))
    {
        err = "Failed to load status data.";
    }
    else if(!LoadBinaryData("Shield/ZoneData.sbin", "ZoneData", true, true))
    {
        err = "Failed to load zone data.";
    }

    if(err.length() == 0)
    {
        // Build complex named types

        // Build Status
        auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mBinaryDataSets["StatusData"]);
        auto cStatusSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mBinaryDataSets["CStatusData"]);

        std::vector<libcomp::String> names;
        std::vector<std::shared_ptr<libcomp::Object>> objs;
        for(auto obj : dataset->GetObjects())
        {
            auto status = std::dynamic_pointer_cast<objects::MiStatusData>(
                obj);
            auto cStatus = std::dynamic_pointer_cast<objects::MiCStatusData>(
                cStatusSet->GetObjectByID(status->GetCommon()->GetID()));

            libcomp::String name = cStatus ? cStatus->GetName() : "[Unnamed]";
            if(status)
            {
                auto cancel = status->GetCancel();
                uint32_t duration = cancel->GetDuration();
                switch(cancel->GetDurationType())
                {
                case objects::MiCancelData::DurationType_t::MS:
                    name = libcomp::String("%1 (%2ms)").Arg(name)
                        .Arg(duration);
                    break;
                case objects::MiCancelData::DurationType_t::DAY:
                    name = libcomp::String("%1 (%2 day%3)").Arg(name)
                        .Arg(duration).Arg(duration != 1 ? "s" : "");
                    break;
                case objects::MiCancelData::DurationType_t::HOUR:
                    name = libcomp::String("%1 (%2 hour%3)").Arg(name)
                        .Arg(duration).Arg(duration != 1 ? "s" : "");
                    break;
                case objects::MiCancelData::DurationType_t::DAY_SET:
                    name = libcomp::String("%1 (%2 day%3 [set])").Arg(name)
                        .Arg(duration).Arg(duration != 1 ? "s" : "");
                    break;
                case objects::MiCancelData::DurationType_t::MS_SET:
                    name = libcomp::String("%1 (%2ms [set])").Arg(name)
                        .Arg(duration);
                    break;
                case objects::MiCancelData::DurationType_t::NONE:
                default:
                    if(duration)
                    {
                        name = libcomp::String("%1 (%2ms?)").Arg(name)
                            .Arg(duration);
                    }
                    break;
                }
            }

            names.push_back(name);
            objs.push_back(obj);
        }

        dataset->MapRecords(objs, names);

        // Build Shop Product
        auto items = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mBinaryDataSets["CItemData"]);
        dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mBinaryDataSets["ShopProductData"]);

        names.clear();
        objs.clear();
        for(auto obj : dataset->GetObjects())
        {
            auto product = std::dynamic_pointer_cast<
                objects::MiShopProductData>(obj);
            libcomp::String name = libcomp::String("%1 x%2").Arg(items
                ->GetName(items->GetObjectByID(product->GetItem())))
                .Arg(product->GetStack());

            names.push_back(name);
            objs.push_back(obj);
        }

        dataset->MapRecords(objs, names);
    }

    if(err.length() > 0)
    {
        QMessageBox Msgbox;
        Msgbox.setText(err.c_str());
        Msgbox.exec();

        auto reply = QMessageBox::question(this, "Load Failed",
            "Loading BinaryData failed. Do you want to update the settings"
            " path for the next restart?", QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            SettingsWindow* settingWindow = new SettingsWindow(this, true,
                this);
            settingWindow->setWindowModality(Qt::ApplicationModal);

            settingWindow->exec();

            delete settingWindow;
        }

        return false;
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

DropSetWindow* MainWindow::GetDropSets() const
{
    return mDropSetWindow;
}

EventWindow* MainWindow::GetEvents() const
{
    return mEventWindow;
}

ZoneWindow* MainWindow::GetZones() const
{
    return mZoneWindow;
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

void MainWindow::RegisterBinaryDataSet(const libcomp::String& objType,
    const std::shared_ptr<libcomp::BinaryDataSet>& dataset,
    bool createSelector)
{
    mBinaryDataSets[objType] = dataset;

    auto namedSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        dataset);
    if(namedSet)
    {
        auto iter = mObjectSelectors.find(objType);
        if(iter == mObjectSelectors.end())
        {
            if(createSelector)
            {
                mObjectSelectors[objType] = new ObjectSelectorWindow(this);
                iter = mObjectSelectors.find(objType);
            }
            else
            {
                // Do not create the selector
                return;
            }
        }

        iter->second->Bind(new ObjectSelectorList(namedSet, objType, false),
            false);
    }
}

ObjectSelectorWindow* MainWindow::GetObjectSelector(
    const libcomp::String& objType) const
{
    auto iter = mObjectSelectors.find(objType);
    return iter != mObjectSelectors.end() ? iter->second : nullptr;
}

void MainWindow::UpdateActiveZone(const libcomp::String& path)
{
    mActiveZonePath = path;

    ui->zonePath->setText(qs(path));

    LOG_INFO(libcomp::String("Loaded: %1\n").Arg(path));

    ui->zoneView->setEnabled(true);
}

void MainWindow::ResetDropSetCount()
{
    size_t total = mDropSetWindow->GetLoadedDropSetCount();
    ui->dropSetCount->setText(qs(libcomp::String("%1 drop set(s) loaded")
        .Arg(total)));
}

void MainWindow::ResetEventCount()
{
    size_t total = mEventWindow->GetLoadedEventCount();
    ui->eventCount->setText(qs(libcomp::String("%1 event(s) loaded")
        .Arg(total)));
}

QString MainWindow::GetDialogDirectory()
{
    QSettings settings;
    QString path = settings.value("dialogDirectory").toString();
    if(path.isEmpty())
    {
        path = settings.value("datastore").toString();
    }

    return path;
}

void MainWindow::SetDialogDirectory(QString path, bool isFile)
{
    QSettings settings;
    if(isFile)
    {
        QFileInfo f(path);
        path = f.absoluteDir().path();
    }

    settings.setValue("dialogDirectory", path);
    settings.sync();
}

void MainWindow::CloseSelectors(QWidget* topLevel)
{
    for(auto& pair : mObjectSelectors)
    {
        pair.second->CloseIfConnected(topLevel);
    }
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

    std::vector<char> bytes;
    if(decrypt)
    {
        bytes = mDatastore->DecryptFile(path);
    }
    else
    {
        bytes = mDatastore->ReadFile(path);
    }

    if(bytes.empty())
    {
        return false;
    }
    else
    {
        LOG_DEBUG(libcomp::String("Loading records from %1\n")
            .Arg(binaryFile));
    }

    std::stringstream ss(std::string(bytes.begin(), bytes.end()));
    if(dataset->Load(ss, true))
    {
        auto namedSet = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            dataset);
        if(addSelector && namedSet &&
            mObjectSelectors.find(objName) == mObjectSelectors.end())
        {
            ObjectSelectorWindow* selector = new ObjectSelectorWindow(this);
            selector->Bind(new ObjectSelectorList(namedSet, objName,
                selectorAllowBlanks), true);
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

    if(mDropSetWindow)
    {
        mDropSetWindow->close();
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
    (void)event;

    CloseAllWindows();
}

void MainWindow::OpenDropSets()
{
    mDropSetWindow->show();
    mDropSetWindow->raise();
}

void MainWindow::OpenEvents()
{
    mEventWindow->show();
    mEventWindow->raise();
}

void MainWindow::OpenSettings()
{
    SettingsWindow* settings = new SettingsWindow(this, false, this);
    settings->setWindowModality(Qt::ApplicationModal);

    settings->exec();

    delete settings;
}

void MainWindow::OpenZone()
{
    if(mZoneWindow->ShowZone())
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
    mZoneWindow->LoadZoneFile();
}
