/**
 * @file tools/cathedral/src/ActionUpdateFlagUI.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an update flag action.
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

#include "ActionUpdateFlagUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Action.h"
#include "ui_ActionUpdateFlag.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

ActionUpdateFlag::ActionUpdateFlag(ActionList *pList,
    MainWindow *pMainWindow, QWidget *pParent) : Action(pList,
    pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::ActionUpdateFlag;
    prop->setupUi(pWidget);

    prop->idSelector->hide();

    ui->actionTitle->setText(tr("<b>Update Flag</b>"));
    ui->layoutMain->addWidget(pWidget);

    connect(prop->flagType, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(FlagTypeChanged()));
}

ActionUpdateFlag::~ActionUpdateFlag()
{
    delete prop;
}

void ActionUpdateFlag::Load(const std::shared_ptr<objects::Action>& act)
{
    mAction = std::dynamic_pointer_cast<objects::ActionUpdateFlag>(act);

    if(!mAction)
    {
        return;
    }

    LoadBaseProperties(mAction);

    prop->flagType->setCurrentIndex(to_underlying(
        mAction->GetFlagType()));
    if(!prop->idNumeric->isHidden())
    {
        prop->idNumeric->setValue((int32_t)mAction->GetID());
    }
    else
    {
        prop->idSelector->SetValue(mAction->GetID());
    }

    prop->remove->setChecked(mAction->GetRemove());
}

std::shared_ptr<objects::Action> ActionUpdateFlag::Save() const
{
    if(!mAction)
    {
        return nullptr;
    }

    SaveBaseProperties(mAction);

    mAction->SetFlagType((objects::ActionUpdateFlag::FlagType_t)
        prop->flagType->currentIndex());

    if(!prop->idNumeric->isHidden())
    {
        mAction->SetID((uint16_t)prop->idNumeric->value());
    }
    else
    {
        mAction->SetID((uint16_t)prop->idSelector->GetValue());
    }

    mAction->SetRemove(prop->remove->isChecked());

    return mAction;
}

void ActionUpdateFlag::FlagTypeChanged()
{
    libcomp::String selectorType;
    switch((objects::ActionUpdateFlag::FlagType_t)
        prop->flagType->currentIndex())
    {
    case objects::ActionUpdateFlag::FlagType_t::PLUGIN:
        selectorType = "CKeyItemData";
        break;
    case objects::ActionUpdateFlag::FlagType_t::VALUABLE:
        selectorType = "CValuablesData";
        break;
    case objects::ActionUpdateFlag::FlagType_t::MAP:
    case objects::ActionUpdateFlag::FlagType_t::TIME_TRIAL:
    default:
        break;
    }

    prop->idSelector->Bind(mMainWindow, selectorType, false);

    prop->idNumeric->setHidden(!selectorType.IsEmpty());
    prop->idSelector->setHidden(selectorType.IsEmpty());

    prop->idNumeric->setValue(0);
    prop->idSelector->SetValue(0);
}
