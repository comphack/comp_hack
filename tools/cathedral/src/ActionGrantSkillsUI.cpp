/**
 * @file tools/cathedral/src/ActionGrantSkillsUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a grant skills action.
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

#include "ActionGrantSkillsUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionGrantSkills.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionGrantSkills::ActionGrantSkills(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionGrantSkills;
    prop->setupUi(pWidget);

    prop->skillIDs->Setup(DynamicItemType_t::PRIMITIVE_UINT, pMainWindow);

    ui->actionTitle->setText(tr("<b>Grant Skills</b>"));
    ui->layoutMain->addWidget(pWidget);
}

ActionGrantSkills::~ActionGrantSkills()
{
    delete prop;
}

void ActionGrantSkills::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionGrantSkills>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->targetType->setCurrentIndex(to_underlying(
        mAction->GetTargetType()));
    prop->skillPoints->setValue(mAction->GetSkillPoints());
    
    for(uint32_t skillID : mAction->GetSkillIDs())
    {
        prop->skillIDs->AddUnsignedInteger(skillID);
    }

    prop->expertiseMax->setValue(mAction->GetExpertiseMax());
    prop->expertiseSet->setChecked(mAction->GetExpertiseSet());

    std::unordered_map<uint32_t, int32_t> points;

    for(auto pts : mAction->GetExpertisePoints())
    {
        points[pts.first] = pts.second;
    }

    prop->expertisePoints->Load(points);
}

std::shared_ptr<objects::Action> ActionGrantSkills::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetTargetType((objects::ActionGrantSkills::TargetType_t)
        prop->targetType->currentIndex());
    mAction->SetSkillPoints((uint16_t)prop->skillPoints->value());

    mAction->ClearSkillIDs();
    for(uint32_t skillID : prop->skillIDs->GetUnsignedIntegerList())
    {
        mAction->InsertSkillIDs(skillID);
    }

    mAction->SetExpertiseMax((uint8_t)prop->expertiseMax->value());
    mAction->SetExpertiseSet(prop->expertiseSet->isChecked());

    mAction->ClearExpertisePoints();
    for(auto pair : prop->expertisePoints->Save())
    {
        mAction->SetExpertisePoints((uint8_t)pair.first, pair.second);
    }

    return mAction;
}
