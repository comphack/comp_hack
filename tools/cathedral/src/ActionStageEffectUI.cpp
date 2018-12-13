/**
 * @file tools/cathedral/src/ActionStageEffectUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a stage effect action.
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

#include "ActionStageEffectUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionStageEffect.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionStageEffect::ActionStageEffect(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionStageEffect;
    prop->setupUi(pWidget);

    ui->actionTitle->setText(tr("<b>Stage Effect</b>"));
    ui->actionLayout->insertWidget(2, pWidget);
}

ActionStageEffect::~ActionStageEffect()
{
    delete prop;
}

void ActionStageEffect::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionStageEffect>(act);

    if(!mAction)
    {
        return;
    }

    prop->sourceContext->setCurrentIndex(to_underlying(
        mAction->GetSourceContext()));
    prop->location->setCurrentIndex(to_underlying(
        mAction->GetLocation()));
    prop->messageID->lineEdit()->setText(
        QString::number(mAction->GetMessageID()));
    prop->effectType->setValue(mAction->GetEffectType());
    prop->messageValue->setValue(mAction->GetMessageValue());
    prop->includeMessage->setChecked(mAction->GetIncludeMessage());
}

std::shared_ptr<objects::Action> ActionStageEffect::Save() const
{
    return mAction;
}
