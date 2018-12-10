/**
 * @file tools/cathedral/src/NPCListWindow.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a window that holds a list of NPCs.
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

#include "NPCListWindow.h"

// Cathedral Includes
#include "MainWindow.h"
#include "SpotListWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_NPCProperties.h"
#include "ui_ObjectListWindow.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

// BinaryData Includes
#include <MiHNPCData.h>
#include <MiHNPCBasicData.h>
#include <MiONPCData.h>

// objects Includes
#include <ServerNPC.h>

NPCListWindow::NPCListWindow(MainWindow *pMainWindow, QWidget *pParent) :
    ObjectListWindow(pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::NPCProperties;
    prop->setupUi(pWidget);

    ui->splitter->addWidget(pWidget);

    ResetSpotList();
}

NPCListWindow::~NPCListWindow()
{
    delete prop;
}

QString NPCListWindow::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(obj);

    if(!npc)
    {
        return {};
    }

    return QString::number(npc->GetID());
}

QString NPCListWindow::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(obj);

    if(!npc)
    {
        return {};
    }

    auto definitions = mMainWindow->GetDefinitions();
    auto id = npc->GetID();
    auto hNPC = definitions->GetHNPCData(id);
    auto oNPC = definitions->GetONPCData(id);

    if(hNPC)
    {
        return qs(hNPC->GetBasic()->GetName());
    }

    if(oNPC)
    {
        return qs(oNPC->GetName());
    }

    return {};
}

void NPCListWindow::LoadProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(obj);

    if(!npc)
    {
        return;
    }

    prop->spot->lineEdit()->setText(QString::number(npc->GetSpotID()));
    prop->x->setValue(npc->GetX());
    prop->y->setValue(npc->GetY());
    prop->rot->setValue(npc->GetRotation());
}

void NPCListWindow::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    (void)obj;
}

void NPCListWindow::ResetSpotList()
{
    prop->spot->clear();
    prop->spot->addItem(QString("0 (None)"), QVariant(0));

    auto spotList = mMainWindow->GetSpotList();

    if(spotList)
    {
        // Get the list of all the spots IDs.
        auto mapping = spotList->GetObjectMapping();

        for(auto it : mapping)
        {
            prop->spot->addItem(it.second, QVariant(it.first));
        }
    }
}
