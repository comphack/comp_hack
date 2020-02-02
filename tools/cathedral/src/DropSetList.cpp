/**
 * @file tools/cathedral/src/DropSetList.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a control that holds a list of Drop Sets.
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

#include "DropSetList.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "DropSetWindow.h"
#include "DynamicList.h"
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_DropSetProperties.h"
#include "ui_ObjectList.h"
#include <PopIgnore.h>

// objects Includes
#include <ItemDrop.h>

// libcomp Includes
#include <PacketCodes.h>

DropSetList::DropSetList(QWidget *pParent) : ObjectList(pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::DropSetProperties;
    prop->setupUi(pWidget);

    ui->splitter->setOrientation(Qt::Orientation::Horizontal);

    // Hide the details panel to start
    prop->layoutMain->itemAt(0)->widget()->hide();

    ui->splitter->addWidget(pWidget);
}

DropSetList::~DropSetList()
{
    delete prop;
}

void DropSetList::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;

    prop->drops->Setup(DynamicItemType_t::OBJ_ITEM_DROP, pMainWindow);
    prop->drops->SetAddText("Add Drop");

    prop->conditions->Setup(DynamicItemType_t::OBJ_EVENT_CONDITION, pMainWindow);
    prop->conditions->SetAddText("Add Condition");
}

QString DropSetList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto ds = std::dynamic_pointer_cast<FileDropSet>(obj);
    if(!ds)
    {
        return {};
    }

    return QString::number(ds->GetID());
}

QString DropSetList::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto ds = std::dynamic_pointer_cast<FileDropSet>(obj);
    if(mMainWindow && ds)
    {
        auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("DropSet"));
        return dataset ? qs(dataset->GetName(ds)) : "";
    }

    return {};
}

void DropSetList::LoadProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto ds = std::dynamic_pointer_cast<FileDropSet>(obj);

    auto parentWidget = prop->layoutMain->itemAt(0)->widget();
    if(!ds)
    {
        parentWidget->hide();
    }
    else
    {
        if(parentWidget->isHidden())
        {
            parentWidget->show();
        }

        prop->id->setText(QString::number(ds->GetID()));
        prop->desc->setText(qs(ds->Desc));
        prop->type->setCurrentIndex(to_underlying(
            ds->GetType()));
        prop->mutexID->setValue((int32_t)ds->GetMutexID());
        prop->giftBoxID->setValue((int32_t)ds->GetGiftBoxID());

        prop->drops->Clear();
        for(auto drop : ds->GetDrops())
        {
            prop->drops->AddObject(drop);
        }

        prop->conditions->Clear();
        for(auto condition : ds->GetConditions())
        {
            prop->conditions->AddObject(condition);
        }
    }
}

void DropSetList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto ds = std::dynamic_pointer_cast<FileDropSet>(obj);
    if(ds)
    {
        ds->Desc = cs(prop->desc->text());
        ds->SetType((objects::DropSet::Type_t)prop->type->currentIndex());
        ds->SetMutexID((uint32_t)prop->mutexID->value());
        ds->SetGiftBoxID((uint32_t)prop->giftBoxID->value());

        auto drops = prop->drops->GetObjectList<objects::ItemDrop>();
        ds->SetDrops(drops);

        auto conditions = prop->conditions->GetObjectList<
            objects::EventCondition>();
        ds->SetConditions(conditions);
    }
}
