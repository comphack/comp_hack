/**
 * @file tools/cathedral/src/ActionZoneInstanceUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a spawn action.
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

#include "ActionZoneInstanceUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionZoneInstance.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionZoneInstance::ActionZoneInstance(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionZoneInstance;
    prop->setupUi(pWidget);

    ui->actionTitle->setText(tr("<b>Zone Instance</b>"));
    ui->layoutMain->addWidget(pWidget);

    prop->timerExpirationEvent->SetMainWindow(pMainWindow);
}

ActionZoneInstance::~ActionZoneInstance()
{
    delete prop;
}

void ActionZoneInstance::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionZoneInstance>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->instanceID->setValue((int32_t)mAction->GetInstanceID());
    prop->mode->setCurrentIndex(to_underlying(
        mAction->GetMode()));
    prop->variantID->setValue((int32_t)mAction->GetVariantID());
    prop->timerID->setValue((int32_t)mAction->GetTimerID());
    prop->timerExpirationEvent->SetEvent(mAction
        ->GetTimerExpirationEventID());
}

std::shared_ptr<objects::Action> ActionZoneInstance::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetInstanceID((uint32_t)prop->instanceID->value());
    mAction->SetMode((objects::ActionZoneInstance::Mode_t)
        prop->mode->currentIndex());
    mAction->SetVariantID((uint32_t)prop->variantID->value());
    mAction->SetTimerID((uint32_t)prop->timerID->value());
    mAction->SetTimerExpirationEventID(prop->timerExpirationEvent->GetEvent());

    return mAction;
}
