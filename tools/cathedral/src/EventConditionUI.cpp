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

// Qt Includes
#include <PushIgnore.h>
#include "ui_EventCondition.h"
#include <PopIgnore.h>

// object Includes
#include <EventCondition.h>
#include <EventFlagCondition.h>
#include <EventScriptCondition.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventCondition::EventCondition(QWidget *pParent) : QWidget(pParent)
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
    ui->typeNormal->addItem("Expertise Not Max",
        (int)objects::EventCondition::Type_t::EXPERTISE_NOT_MAX);
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
}

EventCondition::~EventCondition()
{
    delete ui;
}

void EventCondition::Load(const std::shared_ptr<objects::EventCondition>& e)
{
    ui->value1->setValue(e->GetValue1());
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
    if(flagCondition)
    {
        int idx = ui->typeFlags->findData((int)e->GetType());
        ui->typeNormal->setCurrentIndex(0);
        ui->typeFlags->setCurrentIndex(idx != -1 ? idx : 0);

        /// @todo: fix this
        std::unordered_map<uint32_t, int32_t> flags;
        for(auto& pair : flagCondition->GetFlagStates())
        {
            flags[(uint32_t)pair.first] = pair.second;
        }

        ui->flagStates->Load(flags);

        ui->radFlags->setChecked(true);
    }
    else if(scriptCondition)
    {
        ui->typeNormal->setCurrentIndex(0);
        ui->typeFlags->setCurrentIndex(0);

        ui->script->SetScriptID(scriptCondition->GetScriptID());
        ui->script->SetParams(scriptCondition->GetParams());

        ui->radScript->setChecked(true);
    }
    else
    {
        int idx = ui->typeNormal->findData((int)e->GetType());
        ui->typeNormal->setCurrentIndex(idx != -1 ? idx : 0);
        ui->typeFlags->setCurrentIndex(0);

        ui->radNormal->setChecked(true);
    }

    RefreshAvailableOptions();
}

std::shared_ptr<objects::EventCondition> EventCondition::Save() const
{
    return nullptr;
}

void EventCondition::RadioToggle()
{
    // Reset values if switching to/from flags mode
    if(ui->typeFlags->isEnabled() &&
        !ui->radFlags->isChecked())
    {
        // Flags unchecked
        ui->value1->setValue(0);
        ui->value2->setValue(0);
    }
    else if(!ui->typeFlags->isEnabled() &&
        ui->radFlags->isChecked())
    {
        // Flags checked
        ui->value1->setValue(-1);
        ui->value2->setValue(-1);
    }

    RefreshAvailableOptions();
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
        ui->value1->setEnabled(false);
        ui->value2->setEnabled(false);
    }
    else
    {
        ui->typeFlags->setEnabled(false);
        ui->flagStates->setEnabled(false);
        ui->value1->setEnabled(true);
        ui->value2->setEnabled(true);
    }

    if(ui->radScript->isChecked())
    {
        ui->script->setEnabled(true);
    }
    else
    {
        ui->script->setEnabled(false);
    }
}
