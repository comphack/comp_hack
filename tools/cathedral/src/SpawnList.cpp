/**
 * @file tools/cathedral/src/SpawnList.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a control that holds a list of spawns.
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

#include "SpawnList.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "MainWindow.h"
#include "ZoneWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_ObjectList.h"
#include "ui_Spawn.h"
#include <PopIgnore.h>

// objects Includes
#include <Spawn.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

SpawnList::SpawnList(QWidget *pParent) : ObjectList(pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::Spawn;
    prop->setupUi(pWidget);

    ui->splitter->addWidget(pWidget);
}

SpawnList::~SpawnList()
{
    delete prop;
}

void SpawnList::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;

    prop->type->Bind(pMainWindow, "DevilData");
    prop->drops->Setup(DynamicItemType_t::OBJ_ITEM_DROP, pMainWindow);
    prop->dropSetIDs->Setup(DynamicItemType_t::PRIMITIVE_UINT, pMainWindow);
    prop->gifts->Setup(DynamicItemType_t::OBJ_ITEM_DROP, pMainWindow);
    prop->giftSetIDs->Setup(DynamicItemType_t::PRIMITIVE_UINT, pMainWindow);
}

QString SpawnList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto spawn = std::dynamic_pointer_cast<objects::Spawn>(obj);

    if(!spawn)
    {
        return {};
    }

    return QString::number(spawn->GetID());
}

QString SpawnList::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    auto spawn = std::dynamic_pointer_cast<objects::Spawn>(obj);
    if(mMainWindow && spawn)
    {
        auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
            mMainWindow->GetBinaryDataSet("Spawn"));
        return dataset ? qs(dataset->GetName(spawn)) : "";
    }

    return {};
}

void SpawnList::LoadProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto parentWidget = prop->layoutMain->itemAt(0)->widget();
    if(!obj)
    {
        parentWidget->hide();
    }
    else if(parentWidget->isHidden())
    {
        parentWidget->show();
    }

    prop->drops->Clear();
    prop->dropSetIDs->Clear();
    prop->gifts->Clear();
    prop->giftSetIDs->Clear();

    auto spawn = std::dynamic_pointer_cast<objects::Spawn>(obj);
    if(spawn)
    {
        prop->spawnID->setText(QString::number(spawn->GetID()));
        prop->type->SetValue(spawn->GetEnemyType());
        prop->variant->setValue((int32_t)spawn->GetVariantType());
        prop->category->setCurrentIndex(to_underlying(spawn->GetCategory()));
        prop->level->setValue(spawn->GetLevel());
        prop->xp->setValue((int32_t)spawn->GetXP());

        for(auto drop : spawn->GetDrops())
        {
            prop->drops->AddObject(drop);
        }

        for(uint32_t dropSetID : spawn->GetDropSetIDs())
        {
            prop->dropSetIDs->AddUnsignedInteger(dropSetID);
        }

        prop->inheritDrops->setChecked(spawn->GetInheritDrops());

        prop->talkResist->setValue((int32_t)spawn->GetTalkResist());
        prop->canJoin->setChecked((spawn->GetTalkResults() & 0x01) != 0);
        prop->canGift->setChecked((spawn->GetTalkResults() & 0x02) != 0);

        if(prop->canGift->isChecked())
        {
            prop->grpGifts->show();
        }
        else
        {
            prop->grpGifts->hide();
        }
        
        for(auto gift : spawn->GetGifts())
        {
            prop->gifts->AddObject(gift);
        }

        for(uint32_t giftSetID : spawn->GetGiftSetIDs())
        {
            prop->giftSetIDs->AddUnsignedInteger(giftSetID);
        }

        prop->baseAIType->setValue((int32_t)spawn->GetBaseAIType());
        prop->aiScript->setText(qs(spawn->GetAIScriptID()));
        prop->logicGroupID->setValue((int32_t)spawn->GetLogicGroupID());

        prop->killValue->setValue(spawn->GetKillValue());
        prop->killValueType->setCurrentIndex(to_underlying(spawn
            ->GetKillValueType()));
        prop->bossGroup->setValue((int32_t)spawn->GetBossGroup());
        prop->factionGroup->setValue((int32_t)spawn->GetFactionGroup());
    }
    else
    {
        prop->spawnID->setText("");
    }
}

void SpawnList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    (void)obj;
}
