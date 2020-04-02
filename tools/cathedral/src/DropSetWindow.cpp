/**
 * @file tools/cathedral/src/DropSetWindow.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a window that handles drop set viewing and modification.
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "DropSetWindow.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "DropSetList.h"
#include "FindRefWindow.h"
#include "MainWindow.h"
#include "XmlHandler.h"

// Qt Includes
#include <PushIgnore.h>
#include <QCloseEvent>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <PopIgnore.h>

// object Includes
#include <ItemDrop.h>

// libcomp Includes
#include <Log.h>

class DropSetFile
{
public:
    libcomp::String Path;
    std::list<std::shared_ptr<FileDropSet>> DropSets;
    std::set<uint32_t> PendingRemovals;
};

DropSetWindow::DropSetWindow(MainWindow *pMainWindow, QWidget *pParent) :
    QMainWindow(pParent), mMainWindow(pMainWindow), mFindWindow(0)
{
    ui = new Ui::DropSetWindow;
    ui->setupUi(this);

    ui->dropSetList->SetMainWindow(pMainWindow);

    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(NewFile()));
    connect(ui->actionLoadFile, SIGNAL(triggered()), this, SLOT(LoadFile()));
    connect(ui->actionLoadDirectory, SIGNAL(triggered()), this,
        SLOT(LoadDirectory()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(SaveFile()));
    connect(ui->actionSaveAll, SIGNAL(triggered()), this,
        SLOT(SaveAllFiles()));
    connect(ui->actionFind, SIGNAL(triggered()), this, SLOT(Find()));
    connect(ui->files, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(FileSelectionChanged()));

    connect(ui->actionRefresh, SIGNAL(triggered()), this, SLOT(Refresh()));

    connect(ui->add, SIGNAL(clicked()), this, SLOT(NewDropSet()));
    connect(ui->remove, SIGNAL(clicked()), this, SLOT(RemoveDropSet()));

    connect(ui->dropSetList, SIGNAL(selectedObjectChanged()), this,
        SLOT(SelectDropSet()));
}

DropSetWindow::~DropSetWindow()
{
    delete ui;
}

void DropSetWindow::RebuildNamedDataSet()
{
    auto items = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet("CItemData"));

    std::map<uint32_t, std::shared_ptr<FileDropSet>> sort;
    for(auto& fPair : mFiles)
    {
        for(auto ds : fPair.second->DropSets)
        {
            if(ds->GetType() != objects::DropSet::Type_t::APPEND)
            {
                sort[ds->GetID()] = ds;
            }
        }
    }

    std::vector<libcomp::String> names;
    std::vector<std::shared_ptr<libcomp::Object>> dropSets;
    for(auto& dsPair : sort)
    {
        std::list<libcomp::String> dropStrings;

        auto ds = dsPair.second;
        for(auto drop : ds->GetDrops())
        {
            libcomp::String txt = items->GetName(
                items->GetObjectByID(drop->GetItemType()));

            libcomp::String stack;
            if(drop->GetMinStack() != drop->GetMaxStack())
            {
                stack = libcomp::String("%1~%2").Arg(drop->GetMinStack())
                    .Arg(drop->GetMaxStack());
            }
            else
            {
                stack = libcomp::String("%1").Arg(drop->GetMinStack());
            }

            libcomp::String suffix;
            switch(drop->GetType())
            {
            case objects::ItemDrop::Type_t::LEVEL_MULTIPLY:
                suffix = " [x Lvl]";
                break;
            case objects::ItemDrop::Type_t::RELATIVE_LEVEL_MIN:
                suffix = libcomp::String(" [>= Lvl %1%2]")
                    .Arg(drop->GetModifier() >= 0 ? "+" : "")
                    .Arg(drop->GetModifier());
                break;
            case objects::ItemDrop::Type_t::NORMAL:
            default:
                // No type string
                break;
            }

            if(drop->GetCooldownRestrict())
            {
                suffix += libcomp::String(" [CD: %1]")
                    .Arg(drop->GetCooldownRestrict());
            }

            // Round to 2 decimal places
            dropStrings.push_back(libcomp::String("x%1 %2 %3.%4%%5")
                .Arg(stack).Arg(txt)
                .Arg((int32_t)(drop->GetRate() * 100.f) / 100)
                .Arg((int32_t)(drop->GetRate() * 100.f) % 100)
                .Arg(suffix));
        }

        libcomp::String desc;
        if(!ds->Desc.IsEmpty())
        {
            desc = libcomp::String("%1\n\r    ").Arg(ds->Desc);
        }

        dropSets.push_back(ds);
        names.push_back(libcomp::String("%1%2").Arg(desc)
            .Arg(libcomp::String::Join(dropStrings, ",\n\r    ")));
    }

    auto newData = std::make_shared<BinaryDataNamedSet>(
        [](const std::shared_ptr<libcomp::Object>& obj)->uint32_t
        {
            return std::dynamic_pointer_cast<FileDropSet>(obj)
                ->GetID();
        });
    newData->MapRecords(dropSets, names);
    mMainWindow->RegisterBinaryDataSet("DropSet", newData);
}

size_t DropSetWindow::GetLoadedDropSetCount()
{
    size_t total = 0;
    for(auto& pair : mFiles)
    {
        auto file = pair.second;
        total = (size_t)(total + file->DropSets.size());
    }

    return total;
}

void DropSetWindow::closeEvent(QCloseEvent* event)
{
    if(mFindWindow)
    {
        if(!mFindWindow->close())
        {
            // Close failed (possibly searching) so ignore
            event->ignore();
            return;
        }

        mFindWindow->deleteLater();
        mFindWindow = 0;
    }

    mMainWindow->CloseSelectors(this);
}

void DropSetWindow::FileSelectionChanged()
{
    mMainWindow->CloseSelectors(this);

    SelectFile(cs(ui->files->currentText()));
}

void DropSetWindow::NewDropSet()
{
    auto fIter = mFiles.find(cs(ui->files->currentText()));
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;

    uint32_t dropSetID = 0;

    while(dropSetID == 0)
    {
        dropSetID = (uint32_t)QInputDialog::getInt(this, "Enter an ID",
            "New ID", 0, 0);
        if(!dropSetID)
        {
            return;
        }

        for(auto& fPair : mFiles)
        {
            for(auto ds : fPair.second->DropSets)
            {
                if(ds->GetID() == dropSetID)
                {
                    QMessageBox err;
                    err.setText(QString("Drop set %1 already exists in"
                        " file '%2.'").arg(dropSetID).arg(qs(fPair.first)));
                    err.exec();

                    dropSetID = 0;
                    break;
                }
            }

            if(dropSetID == 0)
            {
                break;
            }
        }
    }

    auto ds = std::make_shared<FileDropSet>();
    ds->SetID(dropSetID);

    file->DropSets.push_back(ds);

    file->DropSets.sort([](const std::shared_ptr<FileDropSet>& a,
        const std::shared_ptr<FileDropSet>& b)
        {
            return a->GetID() < b->GetID();
        });

    Refresh();
}

void DropSetWindow::NewFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getSaveFileName(this,
        tr("Create new Drop Set file"), mMainWindow->GetDialogDirectory(),
        tr("Drop Set XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

    mMainWindow->SetDialogDirectory(qPath, true);

    QFileInfo fi(qPath);
    if(fi.exists() && fi.isFile())
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Attempted to overwrite existing file with"
                " new drop set file: %1").Arg(cs(qPath));
        });

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

void DropSetWindow::RemoveDropSet()
{
    auto fIter = mFiles.find(cs(ui->files->currentText()));
    if(fIter == mFiles.end())
    {
        // No file
        return;
    }

    auto file = fIter->second;
    auto current = std::dynamic_pointer_cast<FileDropSet>(
        ui->dropSetList->GetActiveObject());
    if(current)
    {
        file->PendingRemovals.insert(current->GetID());

        for(auto it = file->DropSets.begin(); it != file->DropSets.end(); it++)
        {
            if(*it == current)
            {
                file->DropSets.erase(it);
                break;
            }
        }

        Refresh();
    }
}

void DropSetWindow::LoadDirectory()
{
    QSettings settings;

    QString qPath = QFileDialog::getExistingDirectory(this,
        tr("Load Drop Set XML folder"), mMainWindow->GetDialogDirectory());
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

    RebuildNamedDataSet();
    mMainWindow->ResetDropSetCount();

    // Refresh selection even if it didnt change
    Refresh();
}

void DropSetWindow::LoadFile()
{
    QSettings settings;

    QString qPath = QFileDialog::getOpenFileName(this, tr("Load Drop Set XML"),
        mMainWindow->GetDialogDirectory(), tr("Drop Set XML (*.xml)"));
    if(qPath.isEmpty())
    {
        return;
    }

    mMainWindow->SetDialogDirectory(qPath, true);

    ui->files->blockSignals(true);

    libcomp::String path = cs(qPath);
    if(LoadFileFromPath(path))
    {
        if(ui->files->currentText() != qs(path))
        {
            ui->files->setCurrentText(qs(path));
        }

        ui->files->blockSignals(false);

        RebuildNamedDataSet();
        mMainWindow->ResetDropSetCount();

        Refresh();
    }
    else
    {
        ui->files->blockSignals(false);
    }
}

void DropSetWindow::SaveFile()
{
    auto filename = cs(ui->files->currentText());
    if(filename.IsEmpty())
    {
        // No file, nothing to do
        return;
    }

    std::list<libcomp::String> paths;
    paths.push_back(filename);
    SaveFiles(paths);
}

void DropSetWindow::SaveAllFiles()
{
    std::list<libcomp::String> paths;
    for(auto& pair : mFiles)
    {
        paths.push_back(pair.first);
    }

    SaveFiles(paths);
}

void DropSetWindow::Refresh()
{
    ui->dropSetList->SaveActiveProperties();

    auto selected = ui->dropSetList->GetActiveObject();

    RebuildNamedDataSet();

    SelectFile(cs(ui->files->currentText()));

    ui->dropSetList->Select(selected);
}

void DropSetWindow::SelectDropSet()
{
    ui->remove->setDisabled(ui->dropSetList->GetActiveObject() == nullptr);
}

void DropSetWindow::Find()
{
    if(!mFindWindow)
    {
        mFindWindow = new FindRefWindow(mMainWindow);
    }

    auto selected = std::dynamic_pointer_cast<objects::DropSet>(
        ui->dropSetList->GetActiveObject());

    mFindWindow->Open("DropSet", selected ? selected->GetID() : 0);
}

bool DropSetWindow::LoadFileFromPath(const libcomp::String& path)
{
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS != doc.LoadFile(path.C()))
    {
        LogGeneralError([&]()
        {
            return libcomp::String("Failed to parse file: %1\n").Arg(path);
        });

        return false;
    }

    auto rootElem = doc.RootElement();
    if(!rootElem)
    {
        LogGeneralError([&]()
        {
            return libcomp::String("No root element in file: %1\n").Arg(path);
        });

        return false;
    }

    std::list<std::shared_ptr<FileDropSet>> dropSets;

    auto objNode = rootElem->FirstChildElement("object");
    while(objNode)
    {
        auto ds = std::make_shared<FileDropSet>();
        if(!ds->Load(doc, *objNode))
        {
            break;
        }

        if(!ds->GetID())
        {
            LogGeneralError([&]()
            {
                return libcomp::String("Drop set with no ID encountered in"
                    " file: %1\n").Arg(path);
            });

            break;
        }

        libcomp::String desc;
        auto descNode = objNode->FirstChildElement("desc");
        if(descNode)
        {
            auto txtNode = descNode->FirstChild();
            if(txtNode && txtNode->ToText())
            {
                desc = libcomp::String(txtNode->ToText()->Value());
            }
        }

        ds->Desc = desc;
        dropSets.push_back(ds);

        objNode = objNode->NextSiblingElement("object");
    }

    // Add the file if it has drop sets or no child nodes
    if(dropSets.size() > 0 || rootElem->FirstChild() == nullptr)
    {
        if(mFiles.find(path) != mFiles.end())
        {
            LogGeneralInfo([&]()
            {
                return libcomp::String("Reloaded %1 drop sets(s) from"
                    " file: %2\n").Arg(dropSets.size()).Arg(path);
            });
        }
        else
        {
            LogGeneralInfo([&]()
            {
                return libcomp::String("Loaded %1 drop set(s) from"
                    " file: %2\n").Arg(dropSets.size()).Arg(path);
            });
        }

        auto file = std::make_shared<DropSetFile>();
        file->Path = path;
        file->DropSets = dropSets;

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
        LogGeneralWarning([&]()
        {
            return libcomp::String("No drop sets found in file: %1\n")
                .Arg(path);
        });
    }

    return false;
}

bool DropSetWindow::SelectFile(const libcomp::String& path)
{
    auto iter = mFiles.find(path);
    if(iter == mFiles.end())
    {
        return false;
    }

    ui->add->setDisabled(false);

    std::vector<std::shared_ptr<libcomp::Object>> dropSets;
    for(auto ds : iter->second->DropSets)
    {
        dropSets.push_back(ds);
    }

    ui->dropSetList->SetObjectList(dropSets);

    return true;
}

void DropSetWindow::SaveFiles(const std::list<libcomp::String>& paths)
{
    // Update the current drop set if we haven't already
    ui->dropSetList->SaveActiveProperties();

    for(auto path : paths)
    {
        auto file = mFiles[path];

        tinyxml2::XMLDocument doc;
        if(tinyxml2::XML_SUCCESS != doc.LoadFile(path.C()))
        {
            LogGeneralError([&]()
            {
                return libcomp::String("Failed to parse file for saving: %1\n")
                    .Arg(path);
            });

            continue;
        }

        std::unordered_map<uint32_t, tinyxml2::XMLNode*> existing;

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
            // Load all existing drop sets for replacement
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
                            existing[libcomp::String(txt->Value())
                                .ToInteger<uint32_t>()] = child;
                        }
                        break;
                    }

                    member = member->NextSiblingElement("member");
                }

                child = child->NextSibling();
            }
        }

        // Remove events first
        for(uint32_t dropSetID : file->PendingRemovals)
        {
            auto iter = existing.find(dropSetID);
            if(iter != existing.end())
            {
                rootElem->DeleteChild(iter->second);
            }
        }

        file->PendingRemovals.clear();

        // Now handle updates
        std::list<tinyxml2::XMLNode*> updatedNodes;
        for(auto ds : file->DropSets)
        {
            // Append drop set to the existing file
            ds->Save(doc, *rootElem);

            tinyxml2::XMLNode* dsNode = rootElem->LastChild();

            if(!ds->Desc.IsEmpty())
            {
                auto descElem = doc.NewElement("desc");
                auto txtElem = doc.NewText(ds->Desc.C());
                descElem->InsertFirstChild(txtElem);
                dsNode->InsertAfterChild(dsNode->FirstChildElement(),
                    descElem);
            }

            // If the drop set already existed in the file, move it to the
            // same location and drop the old one
            auto iter = existing.find(ds->GetID());
            if(iter != existing.end())
            {
                if(iter->second->NextSibling() != dsNode)
                {
                    rootElem->InsertAfterChild(iter->second, dsNode);
                }

                rootElem->DeleteChild(iter->second);
            }

            existing[ds->GetID()] = dsNode;

            updatedNodes.push_back(dsNode);
        }

        // Now reorganize
        tinyxml2::XMLNode* last = 0;
        for(auto ds : file->DropSets)
        {
            auto id = ds->GetID();

            auto iter = existing.find(id);
            if(iter == existing.end()) continue;

            auto child = iter->second;
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

        if(updatedNodes.size() > 0)
        {
            XmlHandler::SimplifyObjects(updatedNodes);
        }

        doc.SaveFile(path.C());

        LogGeneralDebug([&]()
        {
            return libcomp::String("Updated drop set file '%1'\n").Arg(path);
        });
    }
}
