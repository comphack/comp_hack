/**
 * @file tools/cathedral/src/ActionRunScriptUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a run script action.
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

#include "ActionRunScriptUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionRunScript.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionRunScript::ActionRunScript(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionRunScript;
    prop->setupUi(pWidget);

    ui->actionTitle->setText(tr("<b>Run Script</b>"));
    ui->layoutMain->addWidget(pWidget);
}

ActionRunScript::~ActionRunScript()
{
    delete prop;
}

void ActionRunScript::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionRunScript>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->script->SetScriptID(mAction->GetScriptID());
    prop->script->SetParams(mAction->GetParams());
}

std::shared_ptr<objects::Action> ActionRunScript::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetScriptID(prop->script->GetScriptID());

    mAction->ClearParams();
    if(!mAction->GetScriptID().IsEmpty())
    {
        // Ignore params if no script is set
        mAction->SetParams(prop->script->GetParams());
    }

    return mAction;
}
