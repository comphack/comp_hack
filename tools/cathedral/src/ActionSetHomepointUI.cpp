/**
 * @file tools/cathedral/src/ActionSetHomepointUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a start event action.
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

#include "ActionSetHomepointUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionSetHomepoint.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionSetHomepoint::ActionSetHomepoint(ActionList *pList, MainWindow *pMainWindow,
    QWidget *pParent) : Action(pList, pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionSetHomepoint;
    prop->setupUi(pWidget);

    prop->zone->Bind(pMainWindow, "ZoneData", false);
    prop->spot->SetMainWindow(pMainWindow);

    ui->actionTitle->setText(tr("<b>Set Homepoint</b>"));
    ui->layoutMain->addWidget(pWidget);
}

ActionSetHomepoint::~ActionSetHomepoint()
{
    delete prop;
}

void ActionSetHomepoint::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionSetHomepoint>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->zone->SetValue(mAction->GetZoneID());
    prop->spot->SetValue(mAction->GetSpotID());
}

std::shared_ptr<objects::Action> ActionSetHomepoint::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetZoneID(prop->zone->GetValue());
    mAction->SetSpotID(prop->spot->GetValue());

    return mAction;
}
