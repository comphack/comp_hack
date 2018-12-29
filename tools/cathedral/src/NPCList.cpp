/**
 * @file tools/cathedral/src/NPCList.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a control that holds a list of NPCs.
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

#include "NPCList.h"

// Cathedral Includes
#include "MainWindow.h"
#include "SpotList.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_NPCProperties.h"
#include "ui_ObjectList.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

// BinaryData Includes
#include <MiHNPCData.h>
#include <MiHNPCBasicData.h>
#include <MiONPCData.h>

// objects Includes
#include <ServerNPC.h>
#include <ServerObject.h>

NPCList::NPCList(QWidget *pParent) : ObjectList(pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::NPCProperties;
    prop->setupUi(pWidget);

    ui->splitter->addWidget(pWidget);
}

NPCList::~NPCList()
{
    delete prop;
}

void NPCList::Bind(MainWindow *pMainWindow, bool isHNPC)
{
    SetMainWindow(pMainWindow);

    prop->type->Bind(pMainWindow, isHNPC ? "hNPCData" : "oNPCData");
    prop->actions->SetMainWindow(pMainWindow);
}

QString NPCList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto sObj = std::dynamic_pointer_cast<objects::ServerObject>(obj);

    if(!sObj)
    {
        return {};
    }

    return QString::number(sObj->GetID());
}

QString NPCList::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto sObj = std::dynamic_pointer_cast<objects::ServerObject>(obj);
    if(!sObj)
    {
        return {};
    }

    libcomp::String name;

    auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(sObj);
    if(npc)
    {
        auto dataset = mMainWindow->GetBinaryDataSet("hNPCData");
        auto hNPC = std::dynamic_pointer_cast<objects::MiHNPCData>(
            dataset->GetObjectByID(sObj->GetID()));
        if(hNPC)
        {
            name = hNPC->GetBasic()->GetName();
        }
    }
    else
    {
        auto dataset = mMainWindow->GetBinaryDataSet("oNPCData");
        auto oNPC = std::dynamic_pointer_cast<objects::MiONPCData>(
            dataset->GetObjectByID(sObj->GetID()));
        if(oNPC)
        {
            name = oNPC->GetName();
        }
    }

    if(sObj->GetActorID())
    {
        name = libcomp::String("%1 [Actor %2]").Arg(name)
            .Arg(sObj->GetActorID()).Trimmed();
    }

    return qs(name);
}

void NPCList::LoadProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto sObj = std::dynamic_pointer_cast<objects::ServerObject>(obj);

    auto parentWidget = prop->layoutMain->itemAt(0)->widget();
    if(!sObj)
    {
        parentWidget->hide();
    }
    else
    {
        if(parentWidget->isHidden())
        {
            parentWidget->show();
        }

        prop->type->SetValue(sObj->GetID());
        prop->position->Load(sObj);
        prop->state->setValue((uint8_t)sObj->GetState());
        prop->actorID->setValue(sObj->GetActorID());

        auto actions = sObj->GetActions();
        prop->actions->Load(actions);
    }
}

void NPCList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto sObj = std::dynamic_pointer_cast<objects::ServerObject>(obj);
    if(sObj)
    {
        sObj->SetID(prop->type->GetValue());

        auto pos = prop->position->Save();
        sObj->SetSpotID(pos->GetSpotID());
        sObj->SetX(pos->GetX());
        sObj->SetY(pos->GetY());
        sObj->SetRotation(pos->GetRotation());

        sObj->SetState((uint8_t)prop->state->value());
        sObj->SetActorID(prop->actorID->value());

        auto actions = prop->actions->Save();
        sObj->SetActions(actions);
    }
}
