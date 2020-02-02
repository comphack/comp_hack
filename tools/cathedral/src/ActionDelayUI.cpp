/**
 * @file tools/cathedral/src/ActionDelayUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a delay action.
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

#include "ActionDelayUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionDelay.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionDelay::ActionDelay(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionDelay;
    prop->setupUi(pWidget);

    ui->actionTitle->setText(tr("<b>Delay</b>"));
    ui->layoutMain->addWidget(pWidget);

    prop->actions->SetMainWindow(pMainWindow);
}

ActionDelay::~ActionDelay()
{
    delete prop;
}

void ActionDelay::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionDelay>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->type->setCurrentIndex(to_underlying(
        mAction->GetType()));
    prop->delayID->setValue(mAction->GetDelayID());
    prop->duration->setValue((int32_t)mAction->GetDuration());

    auto actions = mAction->GetActions();
    prop->actions->Load(actions);
}

std::shared_ptr<objects::Action> ActionDelay::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetType((objects::ActionDelay::Type_t)
        prop->type->currentIndex());
    mAction->SetDelayID(prop->delayID->value());
    mAction->SetDuration((uint32_t)prop->duration->value());

    auto actions = prop->actions->Save();
    mAction->SetActions(actions);

    return mAction;
}
