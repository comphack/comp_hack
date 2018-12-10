/**
 * @file tools/cathedral/src/ActionIDs.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a list of IDs (for use in actions).
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

#include "ActionIDs.h"

// Cathedral Includes
#include "ActionIDsItem.h"
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_ActionIDs.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <algorithm>
#include <climits>

ActionIDs::ActionIDs(QWidget *pParent) : QWidget(pParent)
{
    ui = new Ui::ActionIDs;
    ui->setupUi(this);

    connect(ui->add, SIGNAL(clicked(bool)), this, SLOT(AddNewValue()));
}

ActionIDs::~ActionIDs()
{
    delete ui;
}

void ActionIDs::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;
}

void ActionIDs::Load(const std::set<uint32_t>& values)
{
    ClearValues();

    for(auto val : values)
    {
        AddValue(new ActionIDsItem(this, val));
    }
}

std::set<uint32_t> ActionIDs::SaveSet() const
{
    std::set<uint32_t> values;

    for(auto pValue : mValues)
    {
        values.insert(pValue->GetValue());
    }

    return values;
}

void ActionIDs::Load(const std::list<uint32_t>& values)
{
    ClearValues();

    for(auto val : values)
    {
        AddValue(new ActionIDsItem(this, val));
    }
}

std::list<uint32_t> ActionIDs::SaveList() const
{
    std::list<uint32_t> values;

    for(auto pValue : mValues)
    {
        values.push_back(pValue->GetValue());
    }

    return values;
}

void ActionIDs::RemoveValue(ActionIDsItem *pValue)
{
    ui->actionMapLayout->removeWidget(pValue);

    auto it = std::find(mValues.begin(), mValues.end(), pValue);

    if(mValues.end() != it)
    {
        mValues.erase(it);
    }

    pValue->deleteLater();
}

void ActionIDs::AddNewValue()
{
    AddValue(new ActionIDsItem(this));
}

void ActionIDs::AddValue(ActionIDsItem *pValue)
{
    mValues.push_back(pValue);

    ui->actionMapLayout->insertWidget(
        ui->actionMapLayout->count() - 1, pValue);
}

void ActionIDs::ClearValues()
{
    for(auto pValue : mValues)
    {
        ui->actionMapLayout->removeWidget(pValue);

        delete pValue;
    }

    mValues.clear();
}
