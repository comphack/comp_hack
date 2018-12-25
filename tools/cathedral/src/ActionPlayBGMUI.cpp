/**
 * @file tools/cathedral/src/ActionPlayBGMUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a play BGM action.
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

#include "ActionPlayBGMUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionPlayBGM.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionPlayBGM::ActionPlayBGM(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionPlayBGM;
    prop->setupUi(pWidget);

    prop->music->Bind(pMainWindow, "CSoundData");

    ui->actionTitle->setText(tr("<b>Play BGM</b>"));
    ui->layoutMain->addWidget(pWidget);
}

ActionPlayBGM::~ActionPlayBGM()
{
    delete prop;
}

void ActionPlayBGM::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionPlayBGM>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->isStop->setChecked(mAction->GetIsStop());
    prop->music->SetValue((uint32_t)mAction->GetMusicID());
    prop->fadeInDelay->setValue(mAction->GetFadeInDelay());
    prop->unknown->setValue(mAction->GetUnknown());
}

std::shared_ptr<objects::Action> ActionPlayBGM::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetIsStop(prop->isStop->isChecked());
    mAction->SetMusicID((int32_t)prop->music->GetValue());
    mAction->SetFadeInDelay(prop->fadeInDelay->value());
    mAction->SetUnknown(prop->unknown->value());

    return mAction;
}
