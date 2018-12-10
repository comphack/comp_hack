/**
 * @file tools/cathedral/src/ActionUpdateZoneFlagsUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an update zone flags action.
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

#include "ActionUpdateZoneFlagsUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionUpdateZoneFlags.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionUpdateZoneFlags::ActionUpdateZoneFlags(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionUpdateZoneFlags;
    prop->setupUi(pWidget);
    prop->flagStates->SetValueName(tr("State:"));

    ui->actionTitle->setText(tr("<b>Update Zone Flags</b>"));
    ui->actionLayout->insertWidget(2, pWidget);
}

ActionUpdateZoneFlags::~ActionUpdateZoneFlags()
{
    delete prop;
}

void ActionUpdateZoneFlags::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionUpdateZoneFlags>(act);

    if(!mAction)
    {
        return;
    }

    prop->sourceContext->setCurrentIndex(to_underlying(
        mAction->GetSourceContext()));
    prop->location->setCurrentIndex(to_underlying(
        mAction->GetLocation()));

    prop->type->setCurrentIndex(to_underlying(
        mAction->GetType()));
    prop->setMode->setCurrentIndex(to_underlying(
        mAction->GetSetMode()));

    std::unordered_map<uint32_t, int32_t> states;

    for(auto state : mAction->GetFlagStates())
    {
        states[(uint32_t)state.first] = state.second;
    }

    prop->flagStates->Load(states);
}

std::shared_ptr<objects::Action> ActionUpdateZoneFlags::Save() const
{
    return mAction;
}
