/**
 * @file tools/cathedral/src/EventConditionUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event condition.
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

#include "EventConditionUI.h"

// cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_EventCondition.h"
#include <PopIgnore.h>

// object Includes
#include <EventCondition.h>
#include <EventFlagCondition.h>
#include <EventScriptCondition.h>

// Standard C++11 Includes
#include <array>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventCondition::EventCondition(MainWindow *pMainWindow, QWidget *pParent)
    : QWidget(pParent), mMainWindow(pMainWindow)
{
    ui = new Ui::EventCondition;
    ui->setupUi(this);

    ui->typeNormal->addItem("Bethel",
        (int)objects::EventCondition::Type_t::BETHEL);
    ui->typeNormal->addItem("Clan Home",
        (int)objects::EventCondition::Type_t::CLAN_HOME);
    ui->typeNormal->addItem("COMP Demon",
        (int)objects::EventCondition::Type_t::COMP_DEMON);
    ui->typeNormal->addItem("COMP Free",
        (int)objects::EventCondition::Type_t::COMP_FREE);
    ui->typeNormal->addItem("Cowrie",
        (int)objects::EventCondition::Type_t::COWRIE);
    ui->typeNormal->addItem("Demon Book",
        (int)objects::EventCondition::Type_t::DEMON_BOOK);
    ui->typeNormal->addItem("DESTINY Box",
        (int)objects::EventCondition::Type_t::DESTINY_BOX);
    ui->typeNormal->addItem("Diaspora Base",
        (int)objects::EventCondition::Type_t::DIASPORA_BASE);
    ui->typeNormal->addItem("Equipped",
        (int)objects::EventCondition::Type_t::EQUIPPED);
    ui->typeNormal->addItem("Event Counter",
        (int)objects::EventCondition::Type_t::EVENT_COUNTER);
    ui->typeNormal->addItem("Event World Counter",
        (int)objects::EventCondition::Type_t::EVENT_WORLD_COUNTER);
    ui->typeNormal->addItem("Expertise",
        (int)objects::EventCondition::Type_t::EXPERTISE);
    ui->typeNormal->addItem("Expertise Active",
        (int)objects::EventCondition::Type_t::EXPERTISE_ACTIVE);
    ui->typeNormal->addItem("Expertise Class Obtainable",
        (int)objects::EventCondition::Type_t::EXPERTISE_CLASS_OBTAINABLE);
    ui->typeNormal->addItem("Expertise Points Obtainable",
        (int)objects::EventCondition::Type_t::EXPERTISE_POINTS_OBTAINABLE);
    ui->typeNormal->addItem("Expertise Points Remaining",
        (int)objects::EventCondition::Type_t::EXPERTISE_POINTS_REMAINING);
    ui->typeNormal->addItem("Faction Group",
        (int)objects::EventCondition::Type_t::FACTION_GROUP);
    ui->typeNormal->addItem("Gender",
        (int)objects::EventCondition::Type_t::GENDER);
    ui->typeNormal->addItem("Instance Access",
        (int)objects::EventCondition::Type_t::INSTANCE_ACCESS);
    ui->typeNormal->addItem("Item",
        (int)objects::EventCondition::Type_t::ITEM);
    ui->typeNormal->addItem("Inventory Free",
        (int)objects::EventCondition::Type_t::INVENTORY_FREE);
    ui->typeNormal->addItem("Level",
        (int)objects::EventCondition::Type_t::LEVEL);
    ui->typeNormal->addItem("LNC",
        (int)objects::EventCondition::Type_t::LNC);
    ui->typeNormal->addItem("LNC Type",
        (int)objects::EventCondition::Type_t::LNC_TYPE);
    ui->typeNormal->addItem("Map",
        (int)objects::EventCondition::Type_t::MAP);
    ui->typeNormal->addItem("Material",
        (int)objects::EventCondition::Type_t::MATERIAL);
    ui->typeNormal->addItem("Moon Phase",
        (int)objects::EventCondition::Type_t::MOON_PHASE);
    ui->typeNormal->addItem("NPC State",
        (int)objects::EventCondition::Type_t::NPC_STATE);
    ui->typeNormal->addItem("Partner Alive",
        (int)objects::EventCondition::Type_t::PARTNER_ALIVE);
    ui->typeNormal->addItem("Partner Familiarity",
        (int)objects::EventCondition::Type_t::PARTNER_FAMILIARITY);
    ui->typeNormal->addItem("Partner Level",
        (int)objects::EventCondition::Type_t::PARTNER_LEVEL);
    ui->typeNormal->addItem("Partner Locked",
        (int)objects::EventCondition::Type_t::PARTNER_LOCKED);
    ui->typeNormal->addItem("Partner Skill Learned",
        (int)objects::EventCondition::Type_t::PARTNER_SKILL_LEARNED);
    ui->typeNormal->addItem("Partner Stat Value",
        (int)objects::EventCondition::Type_t::PARTNER_STAT_VALUE);
    ui->typeNormal->addItem("Party Size",
        (int)objects::EventCondition::Type_t::PARTY_SIZE);
    ui->typeNormal->addItem("Pentalpha Team",
        (int)objects::EventCondition::Type_t::PENTALPHA_TEAM);
    ui->typeNormal->addItem("Plugin",
        (int)objects::EventCondition::Type_t::PLUGIN);
    ui->typeNormal->addItem("Quest Active",
        (int)objects::EventCondition::Type_t::QUEST_ACTIVE);
    ui->typeNormal->addItem("Quest Available",
        (int)objects::EventCondition::Type_t::QUEST_AVAILABLE);
    ui->typeNormal->addItem("Quest Complete",
        (int)objects::EventCondition::Type_t::QUEST_COMPLETE);
    ui->typeNormal->addItem("Quest Phase",
        (int)objects::EventCondition::Type_t::QUEST_PHASE);
    ui->typeNormal->addItem("Quest Phase Requirements",
        (int)objects::EventCondition::Type_t::QUEST_PHASE_REQUIREMENTS);
    ui->typeNormal->addItem("Quest Sequence",
        (int)objects::EventCondition::Type_t::QUEST_SEQUENCE);
    ui->typeNormal->addItem("Quests Active",
        (int)objects::EventCondition::Type_t::QUESTS_ACTIVE);
    ui->typeNormal->addItem("SI Equipped",
        (int)objects::EventCondition::Type_t::SI_EQUIPPED);
    ui->typeNormal->addItem("Skill Learned",
        (int)objects::EventCondition::Type_t::SKILL_LEARNED);
    ui->typeNormal->addItem("Soul Points",
        (int)objects::EventCondition::Type_t::SOUL_POINTS);
    ui->typeNormal->addItem("Stat Value",
        (int)objects::EventCondition::Type_t::STAT_VALUE);
    ui->typeNormal->addItem("Status Active",
        (int)objects::EventCondition::Type_t::STATUS_ACTIVE);
    ui->typeNormal->addItem("Summoned",
        (int)objects::EventCondition::Type_t::SUMMONED);
    ui->typeNormal->addItem("Team Category",
        (int)objects::EventCondition::Type_t::TEAM_CATEGORY);
    ui->typeNormal->addItem("Team Leader",
        (int)objects::EventCondition::Type_t::TEAM_LEADER);
    ui->typeNormal->addItem("Team Size",
        (int)objects::EventCondition::Type_t::TEAM_SIZE);
    ui->typeNormal->addItem("Team Type",
        (int)objects::EventCondition::Type_t::TEAM_TYPE);
    ui->typeNormal->addItem("Timespan",
        (int)objects::EventCondition::Type_t::TIMESPAN);
    ui->typeNormal->addItem("Timespan (Date/Time)",
        (int)objects::EventCondition::Type_t::TIMESPAN_DATETIME);
    ui->typeNormal->addItem("Timespan (Week)",
        (int)objects::EventCondition::Type_t::TIMESPAN_WEEK);
    ui->typeNormal->addItem("Valuable",
        (int)objects::EventCondition::Type_t::VALUABLE);
    ui->typeNormal->addItem("Ziotite (Large)",
        (int)objects::EventCondition::Type_t::ZIOTITE_LARGE);
    ui->typeNormal->addItem("Ziotite (Small)",
        (int)objects::EventCondition::Type_t::ZIOTITE_SMALL);

    // Default to first real option
    ui->typeNormal->setCurrentText("Level");

    ui->typeFlags->addItem("Zone Flags",
        (int)objects::EventCondition::Type_t::ZONE_FLAGS);
    ui->typeFlags->addItem("Zone Flags (Character)",
        (int)objects::EventCondition::Type_t::ZONE_CHARACTER_FLAGS);
    ui->typeFlags->addItem("Zone Flags (Instance)",
        (int)objects::EventCondition::Type_t::ZONE_INSTANCE_FLAGS);
    ui->typeFlags->addItem("Zone Flags (Instance Character)",
        (int)objects::EventCondition::Type_t::ZONE_INSTANCE_CHARACTER_FLAGS);
    ui->typeFlags->addItem("Quest Flags",
        (int)objects::EventCondition::Type_t::QUEST_FLAGS);

    RefreshAvailableOptions();

    connect(ui->radNormal, SIGNAL(clicked(bool)), this, SLOT(RadioToggle()));
    connect(ui->radFlags, SIGNAL(clicked(bool)), this, SLOT(RadioToggle()));
    connect(ui->radScript, SIGNAL(clicked(bool)), this, SLOT(RadioToggle()));
    connect(ui->compareMode, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(CompareModeSelectionChanged()));
    connect(ui->typeNormal, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(TypeSelectionChanged()));
    connect(ui->typeFlags, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(TypeSelectionChanged()));
}

EventCondition::~EventCondition()
{
    delete ui;
}

void EventCondition::Load(const std::shared_ptr<objects::EventCondition>& e)
{
    if(!e || e->GetType() == objects::EventCondition::Type_t::NONE)
    {
        return;
    }

    // Reset min/max while setting the values so they are not adjusted
    ui->value1Number->setMinimum(-2147483647);
    ui->value1Number->setMaximum(2147483647);
    ui->value2->setMinimum(-2147483647);
    ui->value2->setMaximum(2147483647);

    ui->value1Number->setValue(e->GetValue1());
    ui->value1Selector->SetValue(e->GetValue1() >= 0
        ? (uint32_t)e->GetValue1() : 0);
    ui->value2->setValue(e->GetValue2());

    ui->compareMode->setCurrentIndex((int)e->GetCompareMode());
    ui->negate->setChecked(e->GetNegate());

    ui->radNormal->setChecked(false);
    ui->radFlags->setChecked(false);
    ui->radScript->setChecked(false);

    auto flagCondition = std::dynamic_pointer_cast<
        objects::EventFlagCondition>(e);
    auto scriptCondition = std::dynamic_pointer_cast<
        objects::EventScriptCondition>(e);
    if(!flagCondition && !scriptCondition)
    {
        int idx = ui->typeNormal->findData((int)e->GetType());
        ui->typeNormal->setCurrentIndex(idx != -1 ? idx : 0);
        ui->typeFlags->setCurrentIndex(0);

        ui->radNormal->setChecked(true);
    }
    else
    {
        // Reset to first real option
        ui->typeNormal->setCurrentText("Level");

        if(flagCondition)
        {
            int idx = ui->typeFlags->findData((int)e->GetType());
            ui->typeFlags->setCurrentIndex(idx != -1 ? idx : 0);

            auto flags = flagCondition->GetFlagStates();
            ui->flagStates->Load(flags);

            ui->radFlags->setChecked(true);
        }
        else
        {
            ui->typeFlags->setCurrentIndex(0);

            ui->script->SetScriptID(scriptCondition->GetScriptID());

            auto params = scriptCondition->GetParams();
            ui->script->SetParams(params);

            ui->radScript->setChecked(true);
        }
    }

    RefreshAvailableOptions();
}

std::shared_ptr<objects::EventCondition> EventCondition::Save() const
{
    std::shared_ptr<objects::EventCondition> condition;
    if(ui->radFlags->isChecked())
    {
        // Flag condition
        auto fCondition = std::make_shared<objects::EventFlagCondition>();

        auto states = ui->flagStates->SaveSigned();
        fCondition->SetFlagStates(states);

        condition = fCondition;
    }
    else if(ui->radScript->isChecked())
    {
        // Script condition
        auto sCondition = std::make_shared<objects::EventScriptCondition>();

        sCondition->SetScriptID(ui->script->GetScriptID());
        if(!sCondition->GetScriptID().IsEmpty())
        {
            // Ignore params if no script is set
            auto params = ui->script->GetParams();
            sCondition->SetParams(params);
        }

        condition = sCondition;
    }
    else
    {
        // Normal condition
        condition = std::make_shared<objects::EventCondition>();
    }

    condition->SetType(GetCurrentType());

    if(ui->value1Selector->isVisible())
    {
        condition->SetValue1((int32_t)ui->value1Selector->GetValue());
    }
    else
    {
        condition->SetValue1(ui->value1Number->value());
    }

    condition->SetValue2(ui->value2->value());
    condition->SetCompareMode((objects::EventCondition::CompareMode_t)ui
        ->compareMode->currentData().value<int>());
    condition->SetNegate(ui->negate->isChecked());

    return condition;
}

void EventCondition::RadioToggle()
{
    // Reset values if switching from flags mode
    if(ui->typeFlags->isEnabled() &&
        !ui->radFlags->isChecked())
    {
        ui->value1Number->setMinimum(0);
        ui->value1Number->setMaximum(0);
        ui->value1Number->setValue(0);

        ui->value2->setMinimum(0);
        ui->value2->setMaximum(0);
        ui->value2->setValue(0);
    }

    RefreshAvailableOptions();
}

void EventCondition::CompareModeSelectionChanged()
{
    // Only certain compare modes affect type context
    auto type = GetCurrentType();
    if(type == objects::EventCondition::Type_t::DEMON_BOOK ||
        type == objects::EventCondition::Type_t::DESTINY_BOX ||
        type == objects::EventCondition::Type_t::INSTANCE_ACCESS ||
        type == objects::EventCondition::Type_t::MOON_PHASE)
    {
        RefreshTypeContext();
    }
}

void EventCondition::TypeSelectionChanged()
{
    RefreshTypeContext();
}

objects::EventCondition::Type_t EventCondition::GetCurrentType() const
{
    if(ui->radFlags->isChecked())
    {
        return (objects::EventCondition::Type_t)
            ui->typeFlags->currentData().value<int>();
    }
    else if(ui->radScript->isChecked())
    {
        return objects::EventCondition::Type_t::SCRIPT;
    }
    else
    {
        return (objects::EventCondition::Type_t)
            ui->typeNormal->currentData().value<int>();
    }
}

void EventCondition::RefreshAvailableOptions()
{
    if(ui->radNormal->isChecked())
    {
        ui->typeNormal->setEnabled(true);
    }
    else
    {
        ui->typeNormal->setEnabled(false);
    }

    if(ui->radFlags->isChecked())
    {
        ui->typeFlags->setEnabled(true);
        ui->flagStates->setEnabled(true);
    }
    else
    {
        ui->typeFlags->setEnabled(false);
        ui->flagStates->setEnabled(false);
    }

    if(ui->radScript->isChecked())
    {
        ui->script->setEnabled(true);
    }
    else
    {
        ui->script->setEnabled(false);
    }

    RefreshTypeContext();
}

void EventCondition::RefreshTypeContext()
{
    // Turn off control signals
    ui->radNormal->blockSignals(true);
    ui->radFlags->blockSignals(true);
    ui->radScript->blockSignals(true);
    ui->compareMode->blockSignals(true);
    ui->typeNormal->blockSignals(true);
    ui->typeFlags->blockSignals(true);

    // Rebuild valid compare modes
    QVariant currentData = ui->compareMode->currentData();

    ui->compareMode->clear();

    bool cmpNumeric = true;
    bool cmpBetween = true;
    bool cmpEqual = true;
    bool cmpExists = true;

    // Values 1 and 2 contextually change
    bool value1Ignored = false;
    bool value2Ignored = false;

    ui->lblValue1->setText("Value 1:");
    ui->lblValue2->setText("Value 2:");

    libcomp::String defaultCompareTxt;
    libcomp::String selectorObjectType;
    std::array<int, 2> minValues = { { -2147483647, -2147483647 } };
    std::array<int, 2> maxValues = { { 2147483647, 2147483647 } };

    switch(GetCurrentType())
    {
    case objects::EventCondition::Type_t::BETHEL:
        ui->lblValue1->setText("Bethel Type:");
        ui->lblValue2->setText("Amount:");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = 4;
        cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::CLAN_HOME:
        ui->lblValue1->setText("Zone:");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        selectorObjectType = "ZoneData";
        minValues[0] = 0;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::COMP_DEMON:
        ui->lblValue1->setText("Demon Type:");
        value2Ignored = true;
        defaultCompareTxt = "Exists";
        selectorObjectType = "DevilData";
        minValues[0] = 0;
        cmpNumeric = cmpBetween = cmpEqual = false;
        break;
    case objects::EventCondition::Type_t::COMP_FREE:
        ui->lblValue1->setText("(Min) Count:");
        ui->lblValue2->setText("(Optional) Max Count:");
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = maxValues[1] = 10;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::COWRIE:
        ui->lblValue1->setText("(Min) Count:");
        ui->lblValue2->setText("(Optional) Max Count:");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::DEMON_BOOK:
        if(objects::EventCondition::CompareMode_t::EXISTS ==
            (objects::EventCondition::CompareMode_t)
            currentData.value<int>())
        {
            ui->lblValue1->setText("Demon Type:");
            ui->lblValue2->setText("Base Demon?:");
            selectorObjectType = "DevilData";
            minValues[0] = minValues[1] = 0;
            maxValues[1] = 1;
        }
        else
        {
            ui->lblValue1->setText("(Min) Count:");
            ui->lblValue2->setText("(Optional) Max Count:");
            minValues[0] = minValues[1] = 0;
        }
        defaultCompareTxt = "GTE";
        break;
    case objects::EventCondition::Type_t::DESTINY_BOX:
        if(objects::EventCondition::CompareMode_t::EXISTS ==
            (objects::EventCondition::CompareMode_t)
            currentData.value<int>())
        {
            value1Ignored = value2Ignored = true;
        }
        else
        {
            ui->lblValue1->setText("(Min) Count:");
            ui->lblValue2->setText("(Optional) Max Count:");
            minValues[0] = minValues[1] = 0;
        }
        defaultCompareTxt = "GTE";
        break;
    case objects::EventCondition::Type_t::DIASPORA_BASE:
        ui->lblValue1->setText("Base ID:");
        ui->lblValue2->setText("Captured?:");
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::EQUIPPED:
        ui->lblValue1->setText("Item Type:");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        selectorObjectType = "CItemData";
        minValues[0] = 0;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::EVENT_COUNTER:
    case objects::EventCondition::Type_t::EVENT_WORLD_COUNTER:
        ui->lblValue1->setText("Counter Type:");
        ui->lblValue2->setText("Value:");
        defaultCompareTxt = "GTE";
        cmpBetween = false;
        break;
    case objects::EventCondition::Type_t::EXPERTISE:
        ui->lblValue1->setText("Expertise Index:");
        ui->lblValue2->setText("Points or Class (<= 10):");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = 58;
        cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::EXPERTISE_ACTIVE:
        ui->lblValue1->setText("Expertise Index:");
        ui->lblValue2->setText("Locked?:");
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = 58;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::EXPERTISE_CLASS_OBTAINABLE:
        ui->lblValue1->setText("Expertise Index (No Chain):");
        ui->lblValue2->setText("Class Obtainable:");
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = 38;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::EXPERTISE_POINTS_OBTAINABLE:
        ui->lblValue1->setText("Expertise Index (No Chain):");
        ui->lblValue2->setText("Points Obtainable:");
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = 38;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::EXPERTISE_POINTS_REMAINING:
        ui->lblValue1->setText("Expertise Index (-1 = all):");
        ui->lblValue2->setText("Points Remaining:");
        defaultCompareTxt = "GTE";
        minValues[0] = -1;
        minValues[0] = 0;
        maxValues[0] = 58;
        cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::FACTION_GROUP:
        ui->lblValue1->setText("(Min) Value:");
        ui->lblValue2->setText("(Optional) Max Value:");
        defaultCompareTxt = "Equal";
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::GENDER:
        ui->lblValue1->setText("Gender:");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        minValues[0] = 0;
        maxValues[0] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::INSTANCE_ACCESS:
        if(objects::EventCondition::CompareMode_t::EXISTS ==
            (objects::EventCondition::CompareMode_t)
            currentData.value<int>())
        {
            ui->lblValue1->setText("Instance Type:");
            ui->lblValue2->setText("Special Mode:");
        }
        else
        {
            ui->lblValue1->setText("Value:");
            ui->lblValue2->setText("(Optional) Max Value:");
        }
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        break;
    case objects::EventCondition::Type_t::INVENTORY_FREE:
        ui->lblValue1->setText("(Min) Count:");
        ui->lblValue2->setText("(Optional) Max Count:");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = maxValues[1] = 50;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::ITEM:
        ui->lblValue1->setText("Item Type:");
        ui->lblValue2->setText("Amount:");
        defaultCompareTxt = "GTE";
        selectorObjectType = "CItemData";
        minValues[0] = minValues[1] = 0;
        cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::LEVEL:
    case objects::EventCondition::Type_t::PARTNER_LEVEL:
        ui->lblValue1->setText("(Min) Level:");
        ui->lblValue2->setText("(Optional) Max Level:");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = maxValues[1] = 99;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::LNC:
        ui->lblValue1->setText("(Min) Value:");
        ui->lblValue2->setText("(Optional) Max Value:");
        defaultCompareTxt = "Between";
        minValues[0] = minValues[1] = -10000;
        maxValues[0] = maxValues[1] = 10000;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::LNC_TYPE:
        ui->lblValue1->setText("L/N/C (0/2/4):");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        minValues[0] = 0;
        maxValues[0] = 5;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::MAP:
        ui->lblValue1->setText("Map ID:");
        ui->lblValue2->setText("Obtained?:");
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::MATERIAL:
        ui->lblValue1->setText("Material Type:");
        ui->lblValue2->setText("Amount:");
        defaultCompareTxt = "GTE";
        selectorObjectType = "CItemData";
        minValues[0] = minValues[1] = 0;
        cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::MOON_PHASE:
        if(objects::EventCondition::CompareMode_t::BETWEEN ==
            (objects::EventCondition::CompareMode_t)
            currentData.value<int>())
        {
            ui->lblValue1->setText("Start Phase:");
            ui->lblValue2->setText("End Phase:");
            minValues[0] = minValues[1] = 1;
            maxValues[0] = maxValues[1] = 16;
        }
        else if(objects::EventCondition::CompareMode_t::EXISTS ==
            (objects::EventCondition::CompareMode_t)
            currentData.value<int>())
        {
            ui->lblValue1->setText("Phase Mask:");
            value2Ignored = true;
            minValues[0] = 0x0000;
            maxValues[0] = 0xFFFF;
        }
        else
        {
            ui->lblValue1->setText("Phase:");
            value2Ignored = true;
            minValues[0] = 1;
            maxValues[0] = 16;
        }
        defaultCompareTxt = "Equal";
        break;
    case objects::EventCondition::Type_t::NPC_STATE:
        ui->lblValue1->setText("Actor ID:");
        ui->lblValue2->setText("State:");
        defaultCompareTxt = "Equal";
        selectorObjectType = "Actor";
        minValues[1] = 0;
        maxValues[1] = 255;
        cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::PARTNER_ALIVE:
    case objects::EventCondition::Type_t::PARTNER_LOCKED:
        value1Ignored = value2Ignored = true;
        defaultCompareTxt = "Equal";
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::PARTNER_FAMILIARITY:
        ui->lblValue1->setText("(Min) Points:");
        ui->lblValue2->setText("(Optional) Max Points:");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = maxValues[1] = 10000;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::PARTNER_SKILL_LEARNED:
    case objects::EventCondition::Type_t::SKILL_LEARNED:
        ui->lblValue1->setText("Skill ID:");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        minValues[0] = 0;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::PARTNER_STAT_VALUE:
    case objects::EventCondition::Type_t::STAT_VALUE:
        ui->lblValue1->setText("Correct Table Index:");
        ui->lblValue2->setText("Value:");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = 125;
        cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::PARTY_SIZE:
        ui->lblValue1->setText("(Min) Size:");
        ui->lblValue2->setText("(Optional) Max Size:");
        defaultCompareTxt = "Between";
        minValues[0] = minValues[1] = 0;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::PENTALPHA_TEAM:
        ui->lblValue1->setText("(Min) Team Type:");
        ui->lblValue2->setText("(Optional) Max Team Type:");
        defaultCompareTxt = "Between";
        minValues[0] = minValues[1] = 0;
        maxValues[0] = maxValues[1] = 4;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::PLUGIN:
        ui->lblValue1->setText("Plugin ID:");
        ui->lblValue2->setText("Obtained?:");
        defaultCompareTxt = "Equal";
        selectorObjectType = "CKeyItemData";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::QUEST_ACTIVE:
        ui->lblValue1->setText("Quest:");
        ui->lblValue2->setText("Active?:");
        defaultCompareTxt = "Equal";
        selectorObjectType = "CQuestData";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::QUEST_AVAILABLE:
        ui->lblValue1->setText("Quest:");
        value2Ignored = true;
        selectorObjectType = "CQuestData";
        minValues[0] = 0;
        cmpNumeric = cmpBetween = cmpEqual = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::QUEST_COMPLETE:
        ui->lblValue1->setText("Quest:");
        ui->lblValue2->setText("Completed?:");
        defaultCompareTxt = "Equal";
        selectorObjectType = "CQuestData";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::QUEST_FLAGS:
        ui->lblValue1->setText("Quest:");
        value2Ignored = true;
        selectorObjectType = "CQuestData";
        minValues[0] = 0;
        cmpBetween = false;
        break;
    case objects::EventCondition::Type_t::QUEST_PHASE:
        ui->lblValue1->setText("Quest:");
        ui->lblValue2->setText("Phase:");
        defaultCompareTxt = "Equal";
        selectorObjectType = "CQuestData";
        minValues[0] = 0;
        minValues[1] = -2;
        break;
    case objects::EventCondition::Type_t::QUEST_PHASE_REQUIREMENTS:
        ui->lblValue1->setText("Quest:");
        value2Ignored = true;
        selectorObjectType = "CQuestData";
        minValues[0] = 0;
        cmpNumeric = cmpBetween = cmpEqual = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::QUEST_SEQUENCE:
        ui->lblValue1->setText("Quest:");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        selectorObjectType = "CQuestData";
        minValues[0] = 0;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::QUESTS_ACTIVE:
        ui->lblValue1->setText("(Min) Active Count:");
        ui->lblValue2->setText("(Optional) Max Active Count:");
        defaultCompareTxt = "Equal";
        minValues[0] = minValues[1] = 0;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::SI_EQUIPPED:
        value1Ignored = value2Ignored = true;
        defaultCompareTxt = "Equal";
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::SOUL_POINTS:
        ui->lblValue1->setText("(Min) Points:");
        ui->lblValue2->setText("(Optional) Max Points:");
        defaultCompareTxt = "GTE";
        minValues[0] = minValues[1] = 0;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::STATUS_ACTIVE:
        ui->lblValue1->setText("Status Effect:");
        ui->lblValue2->setText("Parter Demon?:");
        defaultCompareTxt = "Exists";
        selectorObjectType = "StatusData";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpEqual = false;
        break;
    case objects::EventCondition::Type_t::SUMMONED:
        ui->lblValue1->setText("Demon Type:");
        ui->lblValue2->setText("Base Demon?:");
        defaultCompareTxt = "Equal";
        selectorObjectType = "DevilData";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = false;
        break;
    case objects::EventCondition::Type_t::TEAM_CATEGORY:
        ui->lblValue1->setText("Team Category:");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        minValues[0] = 0;
        maxValues[0] = 10;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::TEAM_LEADER:
        value1Ignored = value2Ignored = true;
        defaultCompareTxt = "Equal";
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::TEAM_SIZE:
        ui->lblValue1->setText("(Min) Size:");
        ui->lblValue2->setText("(Optional) Max Size:");
        defaultCompareTxt = "Between";
        minValues[0] = minValues[1] = 0;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::TEAM_TYPE:
        ui->lblValue1->setText("Team Type:");
        value2Ignored = true;
        defaultCompareTxt = "Equal";
        minValues[0] = 0;
        maxValues[0] = 12;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::TIMESPAN:
    case objects::EventCondition::Type_t::TIMESPAN_DATETIME:
    case objects::EventCondition::Type_t::TIMESPAN_WEEK:
        ui->lblValue1->setText("Start Time:");
        ui->lblValue2->setText("End Time:");
        defaultCompareTxt = "Between";
        minValues[0] = minValues[1] = 0;
        cmpNumeric = cmpEqual = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::VALUABLE:
        ui->lblValue1->setText("Valuable ID:");
        ui->lblValue2->setText("Obtained?:");
        defaultCompareTxt = "Equal";
        selectorObjectType = "CValuablesData";
        minValues[0] = minValues[1] = 0;
        maxValues[1] = 1;
        cmpNumeric = cmpBetween = cmpExists = false;
        break;
    case objects::EventCondition::Type_t::ZIOTITE_LARGE:
    case objects::EventCondition::Type_t::ZIOTITE_SMALL:
        ui->lblValue1->setText("(Min) Amount:");
        ui->lblValue2->setText("(Optional) Max Amount:");
        defaultCompareTxt = "Between";
        minValues[0] = minValues[1] = 0;
        cmpExists = false;
        break;
    case objects::EventCondition::Type_t::ZONE_FLAGS:
    case objects::EventCondition::Type_t::ZONE_CHARACTER_FLAGS:
    case objects::EventCondition::Type_t::ZONE_INSTANCE_FLAGS:
    case objects::EventCondition::Type_t::ZONE_INSTANCE_CHARACTER_FLAGS:
        value1Ignored = value2Ignored = true;
        defaultCompareTxt = "Equal";
        cmpBetween = false;
        break;
    case objects::EventCondition::Type_t::SCRIPT:
        // Values 1 and 2 have no defined meaning
        cmpNumeric = cmpBetween = cmpEqual = cmpExists = false;
        break;
    default:
        break;
    }

    ui->compareMode->addItem(qs(defaultCompareTxt.IsEmpty()
        ? "Default" : libcomp::String("Default (%1)").Arg(defaultCompareTxt)),
        (int)objects::EventCondition::CompareMode_t::DEFAULT_COMPARE);
    ui->compareMode->addItem("Equal",
        (int)objects::EventCondition::CompareMode_t::EQUAL);
    ui->compareMode->addItem("Exists",
        (int)objects::EventCondition::CompareMode_t::EXISTS);
    ui->compareMode->addItem("LT (or NaN)",
        (int)objects::EventCondition::CompareMode_t::LT_OR_NAN);
    ui->compareMode->addItem("LT",
        (int)objects::EventCondition::CompareMode_t::LT);
    ui->compareMode->addItem("GTE",
        (int)objects::EventCondition::CompareMode_t::GTE);
    ui->compareMode->addItem("Between",
        (int)objects::EventCondition::CompareMode_t::BETWEEN);

    if(!cmpNumeric)
    {
        ui->compareMode->removeItem(ui->compareMode->findData(
            (int)objects::EventCondition::CompareMode_t::LT_OR_NAN));
        ui->compareMode->removeItem(ui->compareMode->findData(
            (int)objects::EventCondition::CompareMode_t::LT));
        ui->compareMode->removeItem(ui->compareMode->findData(
            (int)objects::EventCondition::CompareMode_t::GTE));
    }

    if(!cmpBetween)
    {
        ui->compareMode->removeItem(ui->compareMode->findData(
            (int)objects::EventCondition::CompareMode_t::BETWEEN));
    }

    if(!cmpEqual && !cmpNumeric)
    {
        ui->compareMode->removeItem(ui->compareMode->findData(
            (int)objects::EventCondition::CompareMode_t::EQUAL));
    }

    if(!cmpExists)
    {
        ui->compareMode->removeItem(ui->compareMode->findData(
            (int)objects::EventCondition::CompareMode_t::EXISTS));
    }

    // If the current compare mode value still exists, select it again
    int idx = ui->compareMode->findData(currentData);
    if(idx != -1)
    {
        ui->compareMode->setCurrentIndex(idx);
    }

    if(value1Ignored)
    {
        int lockValue = ui->radFlags->isChecked() ? -1 : 0;
        ui->value1Number->setValue(lockValue);
        ui->lblValue1->setText("Not Used:");
        minValues[0] = lockValue;
        maxValues[0] = lockValue;
        ui->value1Number->setEnabled(false);
    }
    else
    {
        ui->value1Number->setEnabled(true);
    }

    if(value2Ignored)
    {
        int lockValue = ui->radFlags->isChecked() ? -1 : 0;
        ui->value2->setValue(lockValue);
        ui->lblValue2->setText("Not Used:");
        minValues[1] = lockValue;
        maxValues[1] = lockValue;
        ui->value2->setEnabled(false);
    }
    else
    {
        ui->value2->setEnabled(true);
    }

    ui->value1Number->setMinimum(minValues[0]);
    ui->value2->setMinimum(minValues[1]);
    ui->value1Number->setMaximum(maxValues[0]);
    ui->value2->setMaximum(maxValues[1]);

    // Min/max automatically fix existing values

    // Swap value1 control if needed
    if(!selectorObjectType.IsEmpty())
    {
        ui->value1Number->hide();
        ui->value1Selector->show();

        if(ui->value1Selector->BindSelector(mMainWindow, selectorObjectType,
            false))
        {
            // If the binding changed, reset the value
            ui->value1Selector->SetValue(0);
        }
    }
    else
    {
        ui->value1Number->show();
        ui->value1Selector->hide();
    }

    // Turn on control signals
    ui->radNormal->blockSignals(false);
    ui->radFlags->blockSignals(false);
    ui->radScript->blockSignals(false);
    ui->compareMode->blockSignals(false);
    ui->typeNormal->blockSignals(false);
    ui->typeFlags->blockSignals(false);
}
