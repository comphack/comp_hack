/**
 * @file tools/cathedral/src/SpawnList.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a control that holds a list of spawns.
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
#include <MiAIData.h>
#include <MiAIRelationData.h>
#include <MiDevilData.h>
#include <MiFindInfo.h>
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

    connect(prop->grpBaseAIType, SIGNAL(toggled(bool)), this,
        SLOT(BaseAITypeToggled(bool)));
    connect(prop->baseAIType, SIGNAL(valueChanged(int)), this,
        SLOT(UpdateAIDisplay()));
}

SpawnList::~SpawnList()
{
    delete prop;
}

void SpawnList::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;

    prop->type->BindSelector(pMainWindow, "DevilData");
    prop->variant->BindSelector(pMainWindow, "CTitleData");

    prop->drops->Setup(DynamicItemType_t::OBJ_ITEM_DROP, pMainWindow);
    prop->drops->SetAddText("Add Drop");

    prop->dropSetIDs->Setup(DynamicItemType_t::COMPLEX_OBJECT_SELECTOR,
        pMainWindow, "DropSet", true);
    prop->dropSetIDs->SetAddText("Add Drop Set");

    prop->gifts->Setup(DynamicItemType_t::OBJ_ITEM_DROP, pMainWindow);
    prop->gifts->SetAddText("Add Gift");

    prop->giftSetIDs->Setup(DynamicItemType_t::COMPLEX_OBJECT_SELECTOR,
        pMainWindow, "DropSet", true);
    prop->giftSetIDs->SetAddText("Add Gift Drop Set");
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
        prop->variant->SetValue(spawn->GetVariantType());
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

        prop->grpBaseAIType->setChecked(spawn->GetBaseAIType() != 0);

        prop->killValue->setValue(spawn->GetKillValue());
        prop->killValueType->setCurrentIndex(to_underlying(spawn
            ->GetKillValueType()));
        prop->bossGroup->setValue((int32_t)spawn->GetBossGroup());
        prop->factionGroup->setValue(spawn->GetFactionGroup());
        prop->chkValidDQuestTarget->setChecked(spawn
            ->GetValidDemonQuestTarget());
    }
    else
    {
        prop->spawnID->setText("");
    }

    UpdateAIDisplay();
}

void SpawnList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj)
{
    auto spawn = std::dynamic_pointer_cast<objects::Spawn>(obj);
    if(spawn)
    {
        spawn->SetEnemyType(prop->type->GetValue());
        spawn->SetVariantType(prop->variant->GetValue());
        spawn->SetCategory((objects::Spawn::Category_t)prop->category
            ->currentIndex());
        spawn->SetLevel((int8_t)prop->level->value());
        spawn->SetXP(prop->xp->value());

        auto drops = prop->drops->GetObjectList<objects::ItemDrop>();
        spawn->SetDrops(drops);

        auto dropSetIDs = prop->dropSetIDs->GetUnsignedIntegerList();
        spawn->SetDropSetIDs(dropSetIDs);

        spawn->SetInheritDrops(prop->inheritDrops->isChecked());

        spawn->SetTalkResist((uint8_t)prop->talkResist->value());

        spawn->SetTalkResults((uint8_t)(
            prop->canJoin->isChecked() ? 0x01 : 0x00 |
            prop->canGift->isChecked() ? 0x02 : 0x00));

        auto gifts = prop->gifts->GetObjectList<objects::ItemDrop>();
        spawn->SetGifts(gifts);

        auto giftSetIDs = prop->giftSetIDs->GetUnsignedIntegerList();
        spawn->SetGiftSetIDs(giftSetIDs);

        if(prop->grpBaseAIType->isChecked())
        {
            spawn->SetBaseAIType((uint16_t)prop->baseAIType->value());
        }
        else
        {
            spawn->SetBaseAIType(0);
        }

        spawn->SetAIScriptID(cs(prop->aiScript->text()));
        spawn->SetLogicGroupID((uint16_t)prop->logicGroupID->value());

        spawn->SetKillValue(prop->killValue->value());
        spawn->SetKillValueType((objects::Spawn::KillValueType_t)
            prop->killValueType->currentIndex());
        spawn->SetBossGroup((uint8_t)prop->bossGroup->value());
        spawn->SetFactionGroup(prop->factionGroup->value());
        spawn->SetValidDemonQuestTarget(prop->chkValidDQuestTarget
            ->isChecked());
    }
}

void SpawnList::BaseAITypeToggled(bool checked)
{
    if(!checked)
    {
        // Reset to zero
        prop->baseAIType->setValue(0);
    }
    else
    {
        // These should never be enabled
        prop->chkAINormalSkillUse->setEnabled(false);
        prop->chkAIStrikeFirst->setEnabled(false);
    }
}

void SpawnList::UpdateAIDisplay()
{
    prop->baseAIType->blockSignals(true);

    bool reset = false;

    uint32_t demonType = prop->type->GetValue();
    if(demonType && mMainWindow)
    {
        auto devilData = std::dynamic_pointer_cast<objects::MiDevilData>(
            mMainWindow->GetBinaryDataSet("DevilData")
            ->GetObjectByID(demonType));
        if(devilData && prop->baseAIType->value() == 0)
        {
            prop->baseAIType->setValue(devilData->GetAI()->GetType());
        }

        auto aiData = std::dynamic_pointer_cast<objects::MiAIData>(
            mMainWindow->GetBinaryDataSet("AIData")->GetObjectByID(
                (uint32_t)prop->baseAIType->value()));
        if(!aiData && devilData)
        {
            // Reset to default and try to retrieve again
            prop->baseAIType->setValue(devilData->GetAI()->GetType());
            aiData = std::dynamic_pointer_cast<objects::MiAIData>(
                mMainWindow->GetBinaryDataSet("AIData")->GetObjectByID(
                    (uint32_t)prop->baseAIType->value()));
        }

        if(aiData)
        {
            if(aiData->GetAggroLimit())
            {
                size_t aggroMax = 0;
                for(uint8_t i = 0; i < 8; i++)
                {
                    if((aiData->GetAggroLimit() >> i) & 0x01)
                    {
                        aggroMax = (size_t)(aggroMax + i + 1);
                    }
                }

                prop->aiAggroLimit->setText(qs(libcomp::String("%1 (Rank %2)")
                    .Arg(aggroMax).Arg(aiData->GetAggroLimit())));
            }
            else
            {
                prop->aiAggroLimit->setText("1 (Rank 0)");
            }

            prop->aiLevelLimit->setText(qs(libcomp::String("+%1")
                .Arg(aiData->GetAggroLevelLimit())));
            prop->aiAggroDay->setText(qs(libcomp::String("%1 (%2)")
                .Arg(aiData->GetAggroNormal()->GetDistance())
                .Arg(aiData->GetAggroNormal()->GetFOV())));
            prop->aiAggroNight->setText(qs(libcomp::String("%1 (%2)")
                .Arg(aiData->GetAggroNight()->GetDistance())
                .Arg(aiData->GetAggroNight()->GetFOV())));
            prop->aiAggroCast->setText(qs(libcomp::String("%1 (%2)")
                .Arg(aiData->GetAggroCast()->GetDistance())
                .Arg(aiData->GetAggroCast()->GetFOV())));
            prop->aiDeaggroScale->setText(qs(libcomp::String("x%1")
                .Arg(aiData->GetDeaggroScale())));
            prop->aiThinkSpeed->setText(qs(libcomp::String("%1 ms")
                .Arg(aiData->GetThinkSpeed())));
            prop->chkAINormalSkillUse->setChecked(
                aiData->GetNormalSkillUse());
            prop->chkAIStrikeFirst->setChecked(
                aiData->GetStrikeFirst());
        }
        else
        {
            reset = true;
        }
    }
    else
    {
        reset = true;
    }

    if(reset)
    {
        prop->baseAIType->setValue(0);
        prop->aiAggroLimit->setText("");
        prop->aiLevelLimit->setText("");
        prop->aiAggroDay->setText("");
        prop->aiAggroNight->setText("");
        prop->aiAggroCast->setText("");
        prop->aiDeaggroScale->setText("");
        prop->aiThinkSpeed->setText("");
        prop->chkAINormalSkillUse->setChecked(false);
        prop->chkAIStrikeFirst->setChecked(false);
    }

    prop->baseAIType->blockSignals(false);
}
