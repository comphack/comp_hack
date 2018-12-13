/**
 * @file tools/cathedral/src/ActionUpdateCOMPUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an update COMP action.
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

#include "ActionUpdateCOMPUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionUpdateCOMP.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionUpdateCOMP::ActionUpdateCOMP(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionUpdateCOMP;
    prop->setupUi(pWidget);
    prop->addDemons->SetValueName(tr("Count:"));
    prop->addDemons->SetMinMax(0, 255);
    prop->removeDemons->SetValueName(tr("Count:"));
    prop->removeDemons->SetMinMax(0, 255);

    ui->actionTitle->setText(tr("<b>Update COMP</b>"));
    ui->actionLayout->insertWidget(2, pWidget);
}

ActionUpdateCOMP::~ActionUpdateCOMP()
{
    delete prop;
}

void ActionUpdateCOMP::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionUpdateCOMP>(act);

    if(!mAction)
    {
        return;
    }

    prop->sourceContext->setCurrentIndex(to_underlying(
        mAction->GetSourceContext()));
    prop->location->setCurrentIndex(to_underlying(
        mAction->GetLocation()));
    prop->addSlot->setValue(mAction->GetAddSlot());
    prop->unsummon->setChecked(mAction->GetUnsummon());

    std::unordered_map<uint32_t, int32_t> add, remove;

    for(auto count : mAction->GetAddDemons())
    {
        add[count.first] = count.second;
    }

    for(auto count : mAction->GetRemoveDemons())
    {
        remove[count.first] = count.second;
    }

    prop->addDemons->Load(add);
    prop->removeDemons->Load(remove);
}

std::shared_ptr<objects::Action> ActionUpdateCOMP::Save() const
{
    return mAction;
}
