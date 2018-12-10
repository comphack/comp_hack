/**
 * @file tools/cathedral/src/ActionMap.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a map of IDs to a value (for use in actions).
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

#include "ActionMap.h"

// Cathedral Includes
#include "ActionMapItem.h"
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_ActionMap.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <algorithm>
#include <climits>

ActionMap::ActionMap(QWidget *pParent) : QWidget(pParent), mMin(INT_MIN),
    mMax(INT_MAX)
{
    ui = new Ui::ActionMap;
    ui->setupUi(this);

    connect(ui->add, SIGNAL(clicked(bool)), this, SLOT(AddNewValue()));
}

ActionMap::~ActionMap()
{
    delete ui;
}

void ActionMap::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;
}

void ActionMap::Load(const std::unordered_map<uint32_t, int32_t>& values)
{
    ClearValues();

    for(auto val : values)
    {
        AddValue(new ActionMapItem(mValueName, this, val.first, val.second));
    }
}

std::unordered_map<uint32_t, int32_t> ActionMap::Save() const
{
    std::unordered_map<uint32_t, int32_t> values;

    for(auto pValue : mValues)
    {
        values[pValue->GetKey()] = pValue->GetValue();
    }

    return values;
}

void ActionMap::RemoveValue(ActionMapItem *pValue)
{
    ui->actionMapLayout->removeWidget(pValue);

    auto it = std::find(mValues.begin(), mValues.end(), pValue);

    if(mValues.end() != it)
    {
        mValues.erase(it);
    }

    pValue->deleteLater();
}

void ActionMap::SetValueName(const QString& name)
{
    mValueName = name;
}

void ActionMap::SetMinMax(int32_t min, int32_t max)
{
    mMin = min;
    mMax = max;
}

void ActionMap::AddNewValue()
{
    AddValue(new ActionMapItem(mValueName, this));
}

void ActionMap::AddValue(ActionMapItem *pValue)
{
    pValue->SetMinMax(mMin, mMax);

    mValues.push_back(pValue);

    ui->actionMapLayout->insertWidget(
        ui->actionMapLayout->count() - 1, pValue);
}

void ActionMap::ClearValues()
{
    for(auto pValue : mValues)
    {
        ui->actionMapLayout->removeWidget(pValue);

        delete pValue;
    }

    mValues.clear();
}
