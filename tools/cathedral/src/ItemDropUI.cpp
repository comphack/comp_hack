/**
 * @file tools/cathedral/src/ItemDropUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a configured ItemDrop.
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

#include "ItemDropUI.h"

// Cathedral Includes
#include "MainWindow.h"

// objects Includes
#include <ItemDrop.h>

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_ItemDrop.h"
#include <PopIgnore.h>

// libcomp Includes
#include <PacketCodes.h>

ItemDrop::ItemDrop(MainWindow *pMainWindow,
    QWidget *pParent) : QWidget(pParent)
{
    prop = new Ui::ItemDrop;
    prop->setupUi(this);

    prop->itemType->Bind(pMainWindow, "CItemData");
}

ItemDrop::~ItemDrop()
{
    delete prop;
}

void ItemDrop::Load(const std::shared_ptr<objects::ItemDrop>& drop)
{
    prop->itemType->SetValue(drop->GetItemType());
    prop->rate->setValue((double)drop->GetRate());
    prop->minStack->setValue(drop->GetMinStack());
    prop->maxStack->setValue(drop->GetMaxStack());
    prop->type->setCurrentIndex(to_underlying(
        drop->GetType()));
    prop->modifier->setValue((double)drop->GetModifier());
    prop->cooldownRestrict->setValue(drop->GetCooldownRestrict());
}

std::shared_ptr<objects::ItemDrop> ItemDrop::Save() const
{
    auto obj = std::make_shared<objects::ItemDrop>();
    obj->SetItemType(prop->itemType->GetValue());
    obj->SetRate((float)prop->rate->value());
    obj->SetMinStack((uint16_t)prop->minStack->value());
    obj->SetMaxStack((uint16_t)prop->maxStack->value());
    obj->SetType((objects::ItemDrop::Type_t)prop->type->currentIndex());
    obj->SetModifier((float)prop->modifier->value());
    obj->SetCooldownRestrict(prop->cooldownRestrict->value());

    return obj;
}
